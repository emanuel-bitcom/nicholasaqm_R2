#include "aqm_i2c_toxic.h"
#include "aqm_global_data.h"

static const char *TAG = "Toxic SVC";
#define TAG_TOXIC TAG

/* ADS1115 setup ------------------------------------- */
// Below uses the default values speficied by the datasheet
ads1115_t ads1_cfg = {
    .reg_cfg = ADS1115_CFG_LS_COMP_MODE_TRAD | // Comparator is traditional
               ADS1115_CFG_LS_COMP_LAT_NON |   // Comparator is non-latching
               ADS1115_CFG_LS_COMP_POL_LOW |   // Alert is active low
               ADS1115_CFG_LS_COMP_QUE_DIS |   // Compator is disabled
               ADS1115_CFG_LS_DR_1600SPS |     // No. of samples to take
               ADS1115_CFG_MS_MODE_SS,         // Mode is set to single-shot
    .dev_addr = ADS1_ADDR,
};

ads1115_t ads2_cfg = {
    .reg_cfg = ADS1115_CFG_LS_COMP_MODE_TRAD | // Comparator is traditional
               ADS1115_CFG_LS_COMP_LAT_NON |   // Comparator is non-latching
               ADS1115_CFG_LS_COMP_POL_LOW |   // Alert is active low
               ADS1115_CFG_LS_COMP_QUE_DIS |   // Compator is disabled
               ADS1115_CFG_LS_DR_1600SPS |     // No. of samples to take
               ADS1115_CFG_MS_MODE_SS,         // Mode is set to single-shot
    .dev_addr = ADS2_ADDR,
};

//timing variables
uint n_toxic_values=N_VALUES;
uint n_toxic_time=N_TIME*1000;


const int toxic_flag=BIT1;

int start_toxic_svc()
{

  // on i2c not yet initialized return
  if (!I2C_INIT_OK)
  {
    TOXIC_SERVICE_ACTIVE = 0;
    return 1;
  }

  //set timing variables from memory
    interval_config_t my_interval;
    nvs_high_read_interval(&my_interval);
    if(my_interval.repetitions)
    n_toxic_values=my_interval.repetitions;

    /*measuring time must be expressed in ms*/
    if(my_interval.measuring_time)
    n_toxic_time=my_interval.measuring_time*1000;

  // Setup ADS1115
  initiate_two_ads(&ads1_cfg, &ads2_cfg);

  // Start ADC loop
  xTaskCreatePinnedToCore(toxic_fun, "toxic_fun", 2048, NULL, 1, NULL,1);

  TOXIC_SERVICE_ACTIVE = 1;

  return 0;
}


#ifdef VERSION_PRODUCTION

void toxic_fun(void *arg)
{
  //variables to read data into
  unsigned long int CO_W_sum=0, SO2_W_sum=0, NO2_W_sum=0, O3_W_sum=0;
  unsigned long int CO_A_sum=0, SO2_A_sum=0, NO2_A_sum=0, O3_A_sum=0;
  uint toxic_meas_delay=n_toxic_time/n_toxic_values;

  	while(true){
	
    xEventGroupWaitBits(data_sent_evt_grp,toxic_flag, true, true, portMAX_DELAY);

    //variables to read data into
  CO_W_sum=0; SO2_W_sum=0; NO2_W_sum=0; O3_W_sum=0;
  CO_A_sum=0; SO2_A_sum=0; NO2_A_sum=0; O3_A_sum=0;

  for(int index=0; index<n_toxic_values; index++)
        {
          int16_t result_vect[4];
        
        //NO2 and SO2 data
        get_conversion(1, result_vect);
        
        NO2_W_sum+=result_vect[1];
        NO2_A_sum+=result_vect[0];
        SO2_W_sum+=result_vect[3];
        SO2_A_sum+=result_vect[2];

        get_conversion(2, result_vect);
        CO_W_sum+=result_vect[1];
        CO_A_sum+=result_vect[0];
        O3_W_sum+=result_vect[3];
        O3_A_sum+=result_vect[2];

        vTaskDelay(toxic_meas_delay/portTICK_PERIOD_MS);

        }
  
   //we average the values according to a certain function
    CO_W_sum/=n_toxic_values;
    SO2_W_sum/=n_toxic_values;
    NO2_W_sum/=n_toxic_values;
    O3_W_sum/=n_toxic_values;

    CO_A_sum/=n_toxic_values;
    SO2_A_sum/=n_toxic_values;
    NO2_A_sum/=n_toxic_values;
    O3_A_sum/=n_toxic_values;

	//put read data in structure
	toxic_data current_toxic;

	current_toxic.CO_W=CO_W_sum;
  current_toxic.SO2_W=SO2_W_sum;
  current_toxic.NO2_W=NO2_W_sum;
  current_toxic.O3_W=O3_W_sum;

  current_toxic.CO_A=CO_A_sum;
  current_toxic.SO2_A=SO2_A_sum;
  current_toxic.NO2_A=NO2_A_sum;
  current_toxic.O3_A=O3_A_sum;
	

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
			ESP_LOGI(TAG_TOXIC, "successfully opened temp_nvs");
			break;

		default:
			ESP_LOGE(TAG_TOXIC, "Error: %s", esp_err_to_name(result));
			break;
		}

		ESP_ERROR_CHECK(nvs_set_blob(aqm_handle, "aqm_toxic", (void*)&current_toxic, sizeof(toxic_data)));

		ESP_ERROR_CHECK(nvs_commit(aqm_handle));

		data_written=true;

		nvs_close(aqm_handle);

    /**
     * Debug section
    */
    ESP_LOGI(TAG_TOXIC, "Written data: working: CO=%ld SO2=%ld NO2=%ld O3=%ld",current_toxic.CO_W, current_toxic.SO2_W, current_toxic.NO2_W, current_toxic.O3_W);

		xSemaphoreGive(sensors_data_m);
		}
		else{
		printf("temp cannot write to memory\n");
		printf("Trying again in 5 seconds\n");
		vTaskDelay(5000/portTICK_PERIOD_MS);
		}
		}

	xEventGroupSetBits(sensors_evt_grp, toxic_flag);


	}


}

#endif

/**
 * ads_num takes 1 or 2 value
*/
void get_conversion(uint8_t ads_num, int16_t result[]){
  /*Wait for I2C line to be available*/
    WAIT_I2C();
    int16_t FS_mV=6144;
    int16_t conversion_result=0;
    long int conversion_intermediate=0;

    set_active_ads(ads_num);

    // Request single ended on pin AIN0
    ADS1115_request_single_ended_AIN0(); // all functions except for get_conversion_X return 'esp_err_t' for logging

    // Check conversion state - returns true if conversion is complete
    while (!ADS1115_get_conversion_state())
      vTaskDelay(pdMS_TO_TICKS(5)); // wait 5ms before check again

    // Return latest conversion value
    conversion_result=ADS1115_get_conversion();
    conversion_intermediate=conversion_result*FS_mV/32768;
    result[0] = conversion_intermediate;

    // Request single ended on pin AIN0
    ADS1115_request_single_ended_AIN1(); // all functions except for get_conversion_X return 'esp_err_t' for logging

    // Check conversion state - returns true if conversion is complete
    while (!ADS1115_get_conversion_state())
      vTaskDelay(pdMS_TO_TICKS(5)); // wait 5ms before check again

    // Return latest conversion value
    conversion_result=ADS1115_get_conversion();
    conversion_intermediate=conversion_result*FS_mV/32768;
    result[1] = conversion_intermediate;

    // Request single ended on pin AIN0
    ADS1115_request_single_ended_AIN2(); // all functions except for get_conversion_X return 'esp_err_t' for logging

    // Check conversion state - returns true if conversion is complete
    while (!ADS1115_get_conversion_state())
      vTaskDelay(pdMS_TO_TICKS(5)); // wait 5ms before check again

    // Return latest conversion value
    conversion_result=ADS1115_get_conversion();
    conversion_intermediate=conversion_result*FS_mV/32768;
    result[2] = conversion_intermediate;

    // Request single ended on pin AIN0
    ADS1115_request_single_ended_AIN3(); // all functions except for get_conversion_X return 'esp_err_t' for logging

    // Check conversion state - returns true if conversion is complete
    while (!ADS1115_get_conversion_state())
      vTaskDelay(pdMS_TO_TICKS(5)); // wait 5ms before check again

    // Return latest conversion value
    conversion_result=ADS1115_get_conversion();
    conversion_intermediate=conversion_result*FS_mV/32768;
    result[3] = conversion_intermediate;

    RELEASE_I2C();

     //add a delay for other threads to get access to i2c
    vTaskDelay(pdMS_TO_TICKS(100));
}