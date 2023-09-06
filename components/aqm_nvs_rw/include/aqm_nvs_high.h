#ifndef AQM_NVS_HIGH_H
#define AQM_NVS_HIGH_H

#include "aqm_nvs_keys.h"
#include "aqm_nvs_rw.h"


/*Public Functions*/
esp_err_t nvs_high_read_interval(interval_config_t *interval_out);
esp_err_t nvs_high_read_wifi(wifi_aqm_config_t *wifi_config_out);
esp_err_t nvs_high_read_config_generic(const char* key,void *data_out, size_t data_len);

esp_err_t nvs_high_write_interval(interval_config_t *interval_in);
esp_err_t nvs_high_write_config_generic(const char* key,void *data_in, size_t data_len);

#endif