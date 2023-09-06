#ifndef AQM_WIFI_INTERFACE
#define AQM_WIFI_INTERFACE

#include <stdio.h>
#include "esp_system.h"
#include <stdlib.h>
#include "esp_log.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "string.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_types.h"
#include "freertos/event_groups.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "cJSON.h"
#include "esp_sleep.h"
#include "connect.h"
#include "esp_http_server.h"
#include "mdns.h"
#include "esp_spiffs.h"
#include "esp_wifi.h"
#include <aqm_global_data.h>
#include <aqm_nvs_high.h>
#include <aqm_nvs_rw.h>
#include <aqm_nvs_keys.h>

#if CONFIG_FREERTOS_UNICORE
static const BaseType_t app_cpu_wifi = 0;
#else
static const BaseType_t app_cpu_wifi = 1;
#endif

#define TAG_WIFI "WIFI"

//***********************************************************************


void init_wifi_interface();
void main_wifi_fun(void *arg);
void start_mdns_service();
static void init_server();
void deinit_wifi_interface();

/**
 * server handlers
*/
static esp_err_t on_default_url(httpd_req_t *req);
static esp_err_t on_check_uname_url(httpd_req_t *req);
static esp_err_t on_interval_config_url(httpd_req_t *req);
static esp_err_t on_mqtt_config_url(httpd_req_t *req);
static esp_err_t on_lte_config_url(httpd_req_t *req);
static esp_err_t on_lorawan_config_url(httpd_req_t *req);
static esp_err_t on_finish_config_url(httpd_req_t *req);

/**
 * auxiliary functions
*/
static esp_err_t send_spiffs_resource(char* path, httpd_req_t *req);
static char* get_value_from_post_string(const char* key, const char* post_str);

#endif