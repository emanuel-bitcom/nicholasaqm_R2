#include "aqm_sps_ctrl.h"
#include "sps30.h"

#include <stdio.h>
#include "aqm_global_data.h"
#include <string.h>
#include "esp_system.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "aqm_nvs_high.h"

/*public variables-------------------------------------------------*/
//flag that signals work done on SPS
const int sps_flag=BIT0;
//task related variable
TaskHandle_t sps_task_handle;

/*private variables------------------------------------------------*/
//debug var
static const char* TAG= "SPS30";

//timing variables
static unsigned int n_pm_values=N_VALUES;
static unsigned int n_pm_time=N_TIME*1000;

static void sps_main_fun(void* params){
    /*important variables for measuring----------------------------------*/
    //pma is pm1.0
    //pmb is pm4.0
    //pmc is pm10
    //we read pm values for N_PM_VALUES times during N_PM_TIME
    double pma_sum=0, pmb_sum=0, pmc_sum=0; //sum of values
    
    unsigned int sps_meas_delay=n_pm_time/n_pm_values;
    /*sampling period must not be lower than 1s oor higher than 3s*/
    if(sps_meas_delay<1000) sps_meas_delay=1000;
    if(sps_meas_delay>3000) sps_meas_delay=3000;

    struct sps30_measurement m;
    int16_t ret;
    
    /* Busy loop for initialization, because the main loop 
     * does not work without
     * a sensor.
     */
    while (sps30_probe() != 0) {
        ESP_LOGE(TAG,"SPS sensor probing failed");
        sensirion_sleep_usec(1000000); /* wait 1s */
    }
    ESP_LOGI(TAG,"SPS sensor probing successful");
    
    ret = sps30_start_measurement();
    if (ret < 0)
        ESP_LOGE(TAG,"error starting measurement");
    ESP_LOGI(TAG,"measurements started");
   
   /*infinite process loop--------------------------------*/
    while(true){
        /*wait part of the loop--------------------------*/
        xEventGroupWaitBits(data_sent_evt_grp, sps_flag,true, true, portMAX_DELAY);

        /*measuring part of the loop----------------------*/
        unsigned int n_read_ok=0;

        for(int index=0; index<n_pm_values; index++){
           vTaskDelay(pdMS_TO_TICKS(sps_meas_delay));
                ret = sps30_read_measurement(&m);
            if (ret < 0) {
                ESP_LOGE(TAG,"error reading measurement");

            } else {
                ESP_LOGI(TAG,"measured values:\n"
                    "\t%0.2f pm1.0\n"
                    "\t%0.2f pm2.5\n"
                    "\t%0.2f pm4.0\n"
                    "\t%0.2f pm10.0\n"
                    "\t%0.2f nc0.5\n"
                    "\t%0.2f nc1.0\n"
                    "\t%0.2f nc2.5\n"
                    "\t%0.2f nc4.5\n"
                    "\t%0.2f nc10.0\n"
                    "\t%0.2f typical particle size\n\n",
                    m.mc_1p0, m.mc_2p5, m.mc_4p0, m.mc_10p0, m.nc_0p5, m.nc_1p0,
                    m.nc_2p5, m.nc_4p0, m.nc_10p0, m.typical_particle_size);

                    //add to sum for later average
                    pma_sum+=m.mc_1p0;
                    pmb_sum+=m.mc_4p0;
                    pmc_sum+=m.mc_10p0;
                    n_read_ok++;
            }
        }

        /*storage part of the process------------------------------*/
        //we average the values according to a certain function
        pma_sum=pma_sum/n_read_ok;
        pmb_sum=pmb_sum/n_read_ok;
        pmc_sum=pmc_sum/n_read_ok;

        //put read data in structure
        opc_data current_sps;
        current_sps.pmA=pma_sum;
        current_sps.pmB=pmb_sum;
        current_sps.pmC=pmc_sum;

        //we write the obtained values in temp_nvs
        //we write the flag to event group
        bool data_written=false;

        while(!data_written){

            nvs_handle aqm_handle;

            if(xSemaphoreTake(sensors_data_m,0)==pdTRUE){

            ESP_ERROR_CHECK(nvs_flash_init_partition("temp_nvs"));

            esp_err_t result = nvs_open_from_partition("temp_nvs", "store", NVS_READWRITE, &aqm_handle);

            switch (result)
            {
            case ESP_OK:
                
                break;

            default:
                ESP_LOGE(TAG, "Error: %s", esp_err_to_name(result));
                break;
            }

            ESP_ERROR_CHECK(nvs_set_blob(aqm_handle, "aqm_opc", (void*)&current_sps, sizeof(opc_data)));

            ESP_ERROR_CHECK(nvs_commit(aqm_handle));

            data_written=true;

            nvs_close(aqm_handle);


            xSemaphoreGive(sensors_data_m);
            }
            else{
            ESP_LOGE(TAG,"SPS cannot write to memory");
            ESP_LOGW(TAG,"Trying again in 5 seconds");
            vTaskDelay(5000/portTICK_PERIOD_MS);
            }
            }

	    xEventGroupSetBits(sensors_evt_grp, sps_flag);
    }
}

void aqm_sps_init(){
    /*set timing variables from memory------------------*/

    /*interval between measurements*/
    interval_config_t my_interval;
    nvs_high_read_interval(&my_interval);
    if(my_interval.repetitions)
    n_pm_values=my_interval.repetitions;

    /*measuring time must be expressed in ms*/
    if(my_interval.measuring_time)
    n_pm_time=my_interval.measuring_time*60*1000;

    xTaskCreatePinnedToCore(
        sps_main_fun,
        "SPS main function",
        5000,
        NULL,
        1,
        &sps_task_handle,
        1
    );

    vTaskDelay(1000 / portTICK_PERIOD_MS);

}