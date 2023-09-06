#ifndef AQM_SD_CONTROLLER_H
#define AQM_SD_CONTROLLER_H

#include <aqm_global_data.h>
#include "driver/gpio.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "cJSON.h"
#include <aqm_nvs_keys.h>
#include <aqm_nvs_rw.h>

#define FILE_MAX_LEN 1400
#define CHUNCK_LEN 20
#define SD_CS_PIN 32

/**
 * data is stored in another folder than the factory one ("root_config")
 * "config"
*/
esp_err_t store_config_data_to_SD();

/**
 * Each SD card must have root_config and config folders duplicate folders
 * Each SD card must have root_calib and calib duplicate folders
 * Root directories are only used on factory reset.
*/
esp_err_t load_calib_data_to_nvs();
esp_err_t load_config_data_to_nvs();

/**
 * Hardware reset function/s copies the data from root_config to config
 * from SD card
*/
esp_err_t load_root_to_config();

void write_file(char *path, char *content);
void write_file_json(char *path, cJSON *p_file_json);
void read_file_json(char *path, cJSON **p_file_json);

#endif