#include "aqm_i2c_temp.h"

TaskHandle_t temp_task_handle;
static const char *TAG = "TEMP_SVC";

// timing variables
unsigned int n_temp_values = N_VALUES;
unsigned int n_temp_time = N_TIME*1000;

// Connected HDCs
hdc2021_t hdc_external = {
    .dev_addr = HDC_EXT_ADDR};

hdc2021_t hdc_tube = {
    .dev_addr = HDC_TUBE_ADDR};

// Flag that signals finishing work
const int temp_flag = BIT2;

int start_temp_svc()
{

    // on i2c not yet initialized return
    if (!I2C_INIT_OK)
    {
        TEMP_SERVICE_ACTIVE = 0;
        return 1;
    }

    //set timing variables from memory
    interval_config_t my_interval;
    nvs_high_read_interval(&my_interval);
    if(my_interval.repetitions)
    n_temp_values=my_interval.repetitions;

    /*measuring time must be expressed in ms*/
    if(my_interval.measuring_time)
    n_temp_time=my_interval.measuring_time*1000; 

    /**
     *Wait 10 ms before any comm request
     */
    vTaskDelay(10 / 1);
    WAIT_I2C();
    hdc2021_set_config(&hdc_external, HDC_EXT_ADDR, humid_temp, one_sec);
    hdc2021_set_config(&hdc_tube, HDC_TUBE_ADDR, humid_temp, one_sec);
    hdc3022_configure_autonomous();
    RELEASE_I2C();

    /**
     * create the task that measures temperature and humidity
     */
    xTaskCreatePinnedToCore(
        temp_fun,
        "temperature main function",
        3000,
        NULL,
        2,
        &temp_task_handle,
        1);

    xTaskCreatePinnedToCore(
        pm_humid_ctrl_fun,
        "humidity control function",
        2000,
        NULL,
        3,
        &humid_ctrl_handle,
        1
    );

    TEMP_SERVICE_ACTIVE = 1;

    /**
     * On success return 0
     */
    return 0;
}

#if defined VERSION_PRODUCTION
void temp_fun(void *params)
{
    // variables to read data into
    double temp_read;
    double temp_env_sum = 0;
    double temp_sta_sum = 0;
    double humid_read;
    int humid_env_sum = 0;
    int humid_sta_sum = 0;
    unsigned int temp_meas_delay = n_temp_time / n_temp_values;
    /*temp meas delay must not be lower than 2500 ms*/
    if(temp_meas_delay<2500)temp_meas_delay=2500;

    /**
     * wait 50 ms before any measurement request
     */
    vTaskDelay(50 / 1);

    while(true){

    xEventGroupWaitBits(data_sent_evt_grp, temp_flag, true, true, portMAX_DELAY); 

    temp_env_sum = 0;
    temp_sta_sum = 0;
    humid_env_sum = 0;
    humid_sta_sum = 0;

    for (int index = 0; index < n_temp_values; index++)
    {
        WAIT_I2C();

        // read the actual temperature
        hdc2021_get_temp_humid(&hdc_external, &temp_read, &humid_read);
        temp_env_sum += temp_read;
        humid_env_sum += (int)humid_read;
        temp_read=0.0;
        humid_read=0.0;
#ifdef USE_T_TUBE_AS_T_STA
#define hdc_sta hdc_tube
        hdc2021_get_temp_humid(&hdc_sta, &temp_read, &humid_read);
#else
        hdc3022_get_temp_humid(&temp_read, &humid_read);
#endif
        temp_sta_sum += temp_read;
        humid_sta_sum += (int)humid_read;
        
        RELEASE_I2C();

        vTaskDelay(pdMS_TO_TICKS(temp_meas_delay));
    }
    temp_env_sum /= n_temp_values;
    temp_sta_sum /= n_temp_values;
    humid_env_sum /= n_temp_values;
    humid_sta_sum /= n_temp_values;

    // put read data in structure
    temp_data current_temp;
    current_temp.temp_env = temp_env_sum;
    current_temp.temp_sta = temp_sta_sum;
    current_temp.humid_env = humid_env_sum;
    current_temp.humid_sta = humid_sta_sum;

    // we write the obtained values in temp_nvs
    // we write the flag to event group
    bool data_written = false;

    /*maximum number of tries to access memory*/
    int timeout_cycles=20;

    while ((!data_written)&&(timeout_cycles--))
    {
        nvs_handle aqm_handle;

        if (xSemaphoreTake(sensors_data_m, 0) == pdTRUE)
        {

            ESP_ERROR_CHECK(nvs_flash_init_partition("temp_nvs"));

            esp_err_t result = nvs_open_from_partition("temp_nvs", "store", NVS_READWRITE, &aqm_handle);

            switch (result)
            {
            case ESP_OK:
                ESP_LOGI(TAG, "Successfully opened temp nvs to write temperature data");
                break;

            default:
                ESP_LOGE(TAG, "Error: %s", esp_err_to_name(result));
                break;
            }

            ESP_ERROR_CHECK(nvs_set_blob(aqm_handle, "aqm_temperature", (void *)&current_temp, sizeof(temp_data)));
            ESP_ERROR_CHECK(nvs_commit(aqm_handle));

            data_written = true;

            nvs_close(aqm_handle);

            xSemaphoreGive(sensors_data_m);
        }
        else
        {
            printf("temp cannot write to memory\n");
            printf("Trying again in 1 second\n");
            vTaskDelay(1000);
        }
    }

    if(!timeout_cycles){
        ESP_LOGE(TAG, "Temperature process failed to store data");
    }
    xEventGroupSetBits(sensors_evt_grp, temp_flag);

    
    }

    /**
     * Should never reach here
     */
    vTaskDelete(NULL);
}
#endif

