#include "test_module.h"

//***********************************************************************

void load_test_sta_cal(){

    sensor_cal test_CO_cal;
    test_CO_cal.WE0=363;
    test_CO_cal.WEe=353;
    test_CO_cal.AE0=340;
    test_CO_cal.AEe=347;
    test_CO_cal.sensit=399;

    sensor_cal test_SO2_cal;
    test_SO2_cal.WE0=374;
    test_SO2_cal.WEe=349;
    test_SO2_cal.AE0=347;
    test_SO2_cal.AEe=342;
    test_SO2_cal.sensit=378;


    sensor_cal test_NO2_cal;
    test_NO2_cal.WE0=224;
    test_NO2_cal.WEe=214;
    test_NO2_cal.AE0=240;
    test_NO2_cal.AEe=241;
    test_NO2_cal.sensit=295;

    sensor_cal test_O3_cal;
    test_O3_cal.WE0=234;
    test_O3_cal.WEe=237;
    test_O3_cal.AE0=234;
    test_O3_cal.AEe=235;
    test_O3_cal.sensit=300;
    test_O3_cal.NO2_sensit=-480;

    station_cal test_station_cal;

    test_station_cal.CO_cal=test_CO_cal;
    test_station_cal.SO2_cal=test_SO2_cal;
    test_station_cal.NO2_cal=test_NO2_cal;
    test_station_cal.O3_cal=test_O3_cal;

    test_station_cal.station_SN=1;
    test_station_cal.pos_latitude=44.425511;
    test_station_cal.pos_longitude=26.131311;

    //now load configured data to nvs

    nvs_handle aqm_handle;

    ESP_ERROR_CHECK(nvs_flash_init_partition("config_nvs"));

    esp_err_t result = nvs_open_from_partition("config_nvs", "store", NVS_READWRITE, &aqm_handle);

		switch (result)
		{
		case ESP_OK:
			
			break;

		default:
			ESP_LOGE(TAG_TEST, "Error: %s", esp_err_to_name(result));
			break;
		}

    ESP_ERROR_CHECK(nvs_set_blob(aqm_handle, "aqm_sta_cal", (void*)&test_station_cal, sizeof(station_cal)));

    nvs_commit(aqm_handle);

    nvs_close(aqm_handle);

    ESP_LOGI(TAG_TEST, "Loaded test data to nvs");    
    

}

void load_test_lte(){

    lte_cloud bitcom_cloud;
    strcpy(bitcom_cloud.path, "/backend/station/input");
    strcpy(bitcom_cloud.server, "bit.bordeac.ro");
    bitcom_cloud.port=443;

    lte_data aqm_main_lte;
    
    #define ONE_NCE
    //#define VODAFONE

#ifdef VODAFONE
    strcpy(aqm_main_lte.lte_apn, "live.vodafone.com");
    strcpy(aqm_main_lte.lte_user, "live");
    strcpy(aqm_main_lte.lte_pass, "");
#endif

#ifdef ONE_NCE 
    strcpy(aqm_main_lte.lte_apn, "iot.1nce.net");
    strcpy(aqm_main_lte.lte_user, "");
    strcpy(aqm_main_lte.lte_pass, "");
#endif

    nvs_handle aqm_handle;
    ESP_ERROR_CHECK(nvs_flash_init_partition("config_nvs"));
    esp_err_t result = nvs_open_from_partition("config_nvs", "store", NVS_READWRITE, &aqm_handle);

		switch (result)
		{
		case ESP_OK:
			ESP_LOGI(TAG_TEST, "Partition config_nvs opened successfully");
			break;

		default:
			ESP_LOGE(TAG_TEST, "Error: %s", esp_err_to_name(result));
			break;
		}

    result = nvs_set_blob(aqm_handle, "lte_config", (void*)&aqm_main_lte, sizeof(lte_data));

    switch (result)
		{
		case ESP_OK:
			ESP_LOGI(TAG_TEST, "Partitioni config_nvs opened successfully");
			break;

		default:
			ESP_LOGE(TAG_TEST, "Error: %s", esp_err_to_name(result));
			break;
		}

    nvs_commit(aqm_handle);

    result = nvs_set_blob(aqm_handle, "lte_cloud_0", (void*)&bitcom_cloud, sizeof(lte_cloud));

    switch (result)
		{
		case ESP_OK:
			ESP_LOGI(TAG_TEST, "Partitioni config_nvs opened successfully");
			break;

		default:
			ESP_LOGE(TAG_TEST, "Error: %s", esp_err_to_name(result));
			break;
		}

    nvs_commit(aqm_handle);

    nvs_close(aqm_handle);

    ESP_LOGI(TAG_TEST, "Loaded test data to nvs");    

}