#include "aqm_spi_controller.h"

/*Private defines*/
#define SPI_OPC_ready 0xF3
#define SPI_OPC_busy  0x31

//timing variables
uint n_pm_values=N_VALUES;
uint n_pm_time=N_TIME*1000;

esp_err_t ret;
spi_device_handle_t spi;
spi_bus_config_t buscfg;

spi_device_interface_config_t devcfg;


TaskHandle_t opc_task_handle;

//flag that signals work done on OPC
const int opc_flag=BIT0;


spi_bus_config_t buscfg = {
    .miso_io_num = AQM_MISO_PIN,
    .mosi_io_num = AQM_MOSI_PIN,
    .sclk_io_num = AQM_SCK_PIN,
    .quadwp_io_num = -1,
    .quadhd_io_num = -1,
    .max_transfer_sz = 32};

spi_device_interface_config_t devcfg = {
    .command_bits = 0,
    .address_bits = 0,
    .dummy_bits = 0,
    .clock_speed_hz = 600 * 1000, // Clock out at 600 kHz
    .mode = 1,                    // SPI mode 1
    .spics_io_num = -1,   // CS pin
    .queue_size = 5,              // We want to be able to queue 5 transactions at a time
    .pre_cb = NULL,
    .post_cb = NULL
    };

void aqm_spi_init_raw(){
    // initialize SPI bus
    ret = spi_bus_initialize(OPC_HOST, &buscfg, SPI_DMA_CH_AUTO);
    ESP_ERROR_CHECK(ret);
}

void aqm_spi_init()
{   

    //set timing variables from memory
    interval_config_t my_interval;
    nvs_high_read_interval(&my_interval);
    if(my_interval.repetitions)
    n_pm_values=my_interval.repetitions;

    /*measuring time must be expressed in ms*/
    if(my_interval.measuring_time)
    n_pm_time=my_interval.measuring_time*60*1000; 

    // initialize OPC N3
    gpio_set_direction(OPC_CS_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(OPC_CS_PIN, 1);

    // initialize SPI bus
    ret = spi_bus_initialize(OPC_HOST, &buscfg, SPI_DMA_CH_AUTO);
    ESP_ERROR_CHECK(ret);

    // Attach OPC to the SPI bus
    ret = spi_bus_add_device(OPC_HOST, &devcfg, &spi);
    ESP_ERROR_CHECK(ret);

    xTaskCreatePinnedToCore(
        opc_main_fun,
        "OPC main function",
        5000,
        NULL,
        1,
        &opc_task_handle,
        1
    );


    vTaskDelay(1000 / portTICK_PERIOD_MS);
}

// return 0 on success
// return <>0 on fail
extern int opc_power_ON()
{ 
    uint8_t result=0x00;
    
    /*Run a cleaning sequence*************************************************************/

    //Fan DAC ON
    get_OPC_rdy_response(0x03);
    read_opc(0x03, &result);
    cs_high();

    //wait more time such that fan can be turned on
    DELAY_MS(10000);

    /*END OF Run a cleaning sequence*************************************************************/

    /*Run fan setting*************************************************************/

    //Fan DAC ON
    get_OPC_rdy_response(0x03);
    read_opc(0x03, &result);
    cs_high();

    //wait more time such that fan can be turned on
    DELAY_MS(5000);

    /*END OF Run fan setting sequence*************************************************************/
    
    //No way to check if power is successful so assume success
    //power was successfull so return 0
    return 0;
}

// return <>0 on fail
// return 0 on success
extern int opc_power_OFF()
{
    /*
    code to power off fan and laser
    */
   uint8_t result=0x00;

    //Peripherals off
    get_OPC_rdy_response(0x03);
    read_opc(0x00, &result);
    cs_high();
    DELAY_MS(10);

    //No way to check power off result
    //power was successfull so return 0
    return 0;
}

void cs_high()
{
    
    gpio_set_level(OPC_CS_PIN, 1);
    spi_device_release_bus(spi);
}

void cs_low()
{
    spi_device_acquire_bus(spi, portMAX_DELAY);
    gpio_set_level(OPC_CS_PIN, 0);
}

#define N_BUFF_LEN 64
#define N_PM_POS   50
#define N_SPL_POS  44

void read_pm(float *PM1, float *PM2_5, float *PM10){

       if (!get_OPC_rdy_response(0x30)){
         ESP_LOGI(TAG_SPI, "Begin reading hystogram");
        uint8_t opch_tx_buffer[N_BUFF_LEN];
        memset(opch_tx_buffer, 0x30, N_BUFF_LEN);
        uint8_t opch_rx_buffer[N_BUFF_LEN];
        memset(opch_rx_buffer, 0x00, N_BUFF_LEN);
        read_opc_n_async(N_BUFF_LEN, opch_tx_buffer, opch_rx_buffer);
        
        
        cs_high();
        DELAY_MS(10);

#define IS_INVERSED false
#define INDEX(A) (IS_INVERSED?(3-A):(A))

        uint8_t PM_A_vect[4];
        PM_A_vect[INDEX(0)]=opch_rx_buffer[N_PM_POS + 0];
        PM_A_vect[INDEX(1)]=opch_rx_buffer[N_PM_POS + 1];
        PM_A_vect[INDEX(2)]=opch_rx_buffer[N_PM_POS + 2];
        PM_A_vect[INDEX(3)]=opch_rx_buffer[N_PM_POS + 3];

        uint8_t PM_B_vect[4];
        PM_B_vect[INDEX(0)]=opch_rx_buffer[N_PM_POS + 4];
        PM_B_vect[INDEX(1)]=opch_rx_buffer[N_PM_POS + 5];
        PM_B_vect[INDEX(2)]=opch_rx_buffer[N_PM_POS + 6];
        PM_B_vect[INDEX(3)]=opch_rx_buffer[N_PM_POS + 7];

        uint8_t PM_C_vect[4];
        PM_C_vect[INDEX(0)]=opch_rx_buffer[N_PM_POS + 8];
        PM_C_vect[INDEX(1)]=opch_rx_buffer[N_PM_POS + 9];
        PM_C_vect[INDEX(2)]=opch_rx_buffer[N_PM_POS + 10];
        PM_C_vect[INDEX(3)]=opch_rx_buffer[N_PM_POS + 11];

        float *PM_A_value=(float *)&PM_A_vect[0];
        ESP_LOGW(TAG_SPI, "PMA read value is: %f", *PM_A_value);

        float *PM_B_value=(float *)&PM_B_vect[0];
        ESP_LOGW(TAG_SPI, "PMB read value is: %f", *PM_B_value);

        float *PM_C_value=(float *)&PM_C_vect[0];
        ESP_LOGW(TAG_SPI, "PMC read value is: %f", *PM_C_value);

        uint8_t SPL_int[4];
        SPL_int[0]=opch_rx_buffer[N_SPL_POS + 0];
        SPL_int[1]=opch_rx_buffer[N_SPL_POS + 1];
        SPL_int[2]=opch_rx_buffer[N_SPL_POS + 2];
        SPL_int[3]=opch_rx_buffer[N_SPL_POS + 3];
        ESP_LOGW(TAG_SPI, "Sampling Period: %f s", *((float*)&SPL_int[0]));

        *PM1=*PM_A_value;
        *PM2_5=*PM_B_value;
        *PM10=*PM_C_value;

    }
}



void read_opc(uint8_t command, uint8_t *result)
{
    spi_transaction_t t;
    memset(&t, 0, sizeof(spi_transaction_t));
    t.flags = SPI_TRANS_USE_RXDATA | SPI_TRANS_USE_TXDATA; //|SPI_TRANS_CS_KEEP_ACTIVE;
    t.cmd=0x00;
    t.addr=0x00;
    t.length = 8;
    t.rxlength = 8;
    t.tx_data[0] = command;

    /*use interrupt*/
    spi_device_polling_transmit(spi, &t);
    //spi_device_polling_transmit(spi, &t);

    *result = t.rx_data[0];
    //ESP_LOGI(TAG_SPI, "OPC response: %x", t.rx_data[0]);
}

void read_opc_bytes(uint8_t n_bytes, uint8_t *my_tx_buffer, uint8_t *my_rx_buffer)
{
    spi_transaction_t t;
    memset(&t, 0, sizeof(spi_transaction_t));
    t.length = 2 * n_bytes;
    t.rxlength = n_bytes;
    t.tx_buffer = my_tx_buffer;
    t.rx_buffer = my_rx_buffer;
    spi_device_polling_transmit(spi, &t);
}


// returns 0 on success
// returns 1 if command timeout
int get_OPC_rdy_response(uint8_t cmd_byte)
{
    uint8_t response = 0x00;

    /*have a timeout in case no sensor is connected*/
    uint timeout_cycles=100;

    do{
        cs_low();
        uint8_t tries=0;
        do{
            read_opc(cmd_byte, &response);
            if(response!=SPI_OPC_ready) vTaskDelay(pdMS_TO_TICKS(1));
        }while((tries++<20)&&(response!=SPI_OPC_ready));

        if(response!= SPI_OPC_ready)
        {
            cs_high();

            if(response==SPI_OPC_busy)
            {
                ESP_LOGE(TAG_SPI, "Waiting 2s for OPC comms timeout");
                vTaskDelay(pdMS_TO_TICKS(2000));
            }
            else
            {
                ESP_LOGE(TAG_SPI, "General OPC-SPI fail");
                vTaskDelay(pdMS_TO_TICKS(6000));
            }
        }

    }while((response!=SPI_OPC_ready)&&(--timeout_cycles));
   
    if(response==SPI_OPC_ready){
        /*the exit was on correct response*/
        return 0;
    }    

    return 1; // default command timeout
}

void read_opc_n_async(uint8_t n_bytes, uint8_t *my_tx_buffer, uint8_t *my_rx_buffer){
    uint16_t index=0;

    for(index=0; index<n_bytes; index++){
        read_opc(my_tx_buffer[index], &my_rx_buffer[index]);
        DELAY_MS(1);
    }
}


void opc_main_fun(void *params){

    //pma is pm1.0
    //pmb is pm2.5
    //pmc is pm10
    //we read pm values for N_PM_VALUES times during N_PM_TIME
    float pma, pmb, pmc; //momentary values
    double pma_sum=0, pmb_sum=0, pmc_sum=0; //sum of values
    
    uint opc_meas_delay=n_pm_time/n_pm_values;
    /*sampling period must not be lower than 2s oor higher than 30s*/
    if(opc_meas_delay<2000) opc_meas_delay=2000;
    if(opc_meas_delay>30000) opc_meas_delay=30000;

    while(true){

    xEventGroupWaitBits(data_sent_evt_grp, opc_flag,true, true, portMAX_DELAY);    
    
    pma_sum=0.0; pmb_sum=0.0; pmc_sum=0.0; //sum of values init to 0

        // power ON
    if (opc_power_ON())
    {
        while(true){
            ESP_LOGE(TAG_SPI, "Could not communicate with opc");
            vTaskDelay(5000/portTICK_PERIOD_MS);
            if(!opc_power_ON()){
                ESP_LOGI(TAG_SPI, "opc comm established");
                break;
            }
        }
    }
	
    /*Discard first set of readings over an unknown period*/
    DELAY_MS(opc_meas_delay);
    read_pm(&pma, &pmb, &pmc);
    ESP_LOGE(TAG_SPI,"Discarded data is: PMA: %f; PMB: %f; PMC: %f;\n", pma, pmb, pmc);
    //DELAY_MS(opc_meas_delay);

    /*Perform good reeadings*/
    for(int index=0; index<n_pm_values; index++)
    {
    DELAY_MS(opc_meas_delay);
    read_pm(&pma, &pmb, &pmc);
    ESP_LOGI(TAG_SPI,"Data read is: PMA: %f; PMB: %f; PMC: %f;\n", pma, pmb, pmc);

    pma_sum+=pma;
    pmb_sum+=pmb;
    pmc_sum+=pmc;
    }

    //power off sensor until next measurements
    opc_power_OFF();

    //we average the values according to a certain function
    pma_sum=pma_sum/n_pm_values;
    pmb_sum=pmb_sum/n_pm_values;
    pmc_sum=pmc_sum/n_pm_values;

    //put read data in structure
    opc_data current_opc;
    current_opc.pmA=pma_sum;
    current_opc.pmB=pmb_sum;
    current_opc.pmC=pmc_sum;

    //we write the obtained values in temp_nvs
	//we write the flag to event group
	bool data_written=false;

    while(!data_written){


		if(xSemaphoreTake(sensors_data_m,0)==pdTRUE){

        write_opc_data(current_opc);

		data_written=true;

		xSemaphoreGive(sensors_data_m);
		}
		else{
		printf("OPC cannot write to memory\n");
		printf("Trying again in 5 seconds\n");
		vTaskDelay(5000/portTICK_PERIOD_MS);
		}
		}

	xEventGroupSetBits(sensors_evt_grp, opc_flag);

	
	}

}


extern void opc_deinit(){
  vTaskDelete(opc_task_handle);
  ESP_LOGW(TAG_SPI,"deleted opc task\n");
}