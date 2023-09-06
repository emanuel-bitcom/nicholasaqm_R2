#include "aqm_nvs_high.h"

esp_err_t nvs_high_read_interval(interval_config_t *interval_out)
{
    wait_for_mutex(&sensors_data_m, 2000);
    esp_err_t ret_val=ESP_FAIL;
    ret_val=nvs_retrieve_blob(NVS_KEY_CONFIG_INTERVAL, (void*)interval_out, sizeof(interval_config_t),NVS_CONFIGURATION);
    nvs_storage_close();
    release_mutex(&sensors_data_m);

    return ret_val;
}

esp_err_t nvs_high_read_wifi(wifi_aqm_config_t *wifi_config_out)
{
    wait_for_mutex(&sensors_data_m, 2000);
    esp_err_t ret_val = ESP_FAIL;
    ret_val = nvs_retrieve_blob(NVS_KEY_CONFIG_WIFI, (void *)wifi_config_out, sizeof(wifi_aqm_config_t), NVS_CONFIGURATION);
    nvs_storage_close();
    release_mutex(&sensors_data_m);

    return ret_val;
}

esp_err_t nvs_high_read_config_generic(const char* key,void *data_out, size_t data_len){
    wait_for_mutex(&sensors_data_m, 2000);
    esp_err_t ret_val = ESP_FAIL;
    ret_val = nvs_retrieve_blob(key, data_out, data_len, NVS_CONFIGURATION);
    nvs_storage_close();
    release_mutex(&sensors_data_m);

    return ret_val;
}

esp_err_t nvs_high_write_interval(interval_config_t *interval_in){
    wait_for_mutex(&sensors_data_m, 2000);
    esp_err_t ret_val = ESP_FAIL;
    ret_val= nvs_store_blob(NVS_KEY_CONFIG_INTERVAL, (void*)interval_in, sizeof(interval_config_t),NVS_CONFIGURATION);
    nvs_storage_close();
    release_mutex(&sensors_data_m);

    return ret_val;
}

esp_err_t nvs_high_write_config_generic(const char* key,void *data_in, size_t data_len){
    wait_for_mutex(&sensors_data_m, 2000);
    esp_err_t ret_val = ESP_FAIL;
    ret_val= nvs_store_blob(key, data_in, data_len,NVS_CONFIGURATION);
    nvs_storage_close();
    release_mutex(&sensors_data_m);

    return ret_val;
}