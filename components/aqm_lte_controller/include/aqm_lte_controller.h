#ifndef AQM_LTE_CONTROLLER
#define AQM_LTE_CONTROLLER

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
#include "aqm_global_data.h"
#include "aqm_nvs_high.h"
#include "aqm_nvs_keys.h"


#define TAG1 "UART"

//pins of Quectel board
#define LTE_RES 25
#define LTE_POW 26
#define LTE_WKP 27
#define LTE_TX 33
#define LTE_RX 5

//global settings
#define MAX_SUPPORTED_SERVERS 5

#define RX_BUF_SIZE 1024
#define TX_BUF_SIZE 1024
#define PATTERN_LEN 3
#define LTE_SERIAL_TIMEOUT 1000
#define LTE_SERIAL_PERIPHERAL UART_NUM_1

/**
 * LTE serial macro
*/
#define SEND_AT(T) lte_serial_write(T, strlen(T))

/**
 * MQTT connection data
*/
typedef struct mqtt_config{
    char broker_ip[25];
    uint port;
    uint keepalive;
    char username[30];
    char password[30];
    char topic[60];
    uint8_t qos;
}mqtt_config_t;

/*global variables*/
extern const int lte_flag;
extern const int data_sent_done;

void init_modem(const char* apn, const char* user, const char* pass);

static void init_https();
static void init_mqtt(mqtt_config_t *config, uint client_idx);
static void publish_on_mqtt(mqtt_config_t *config, uint client_idx, char *message);
static void disconnect_mqtt(uint client_idx);

static void send_post_request(lte_cloud target_server, char* body);

static void power_on();
static void power_off();


static void lte_serial_write(const char* data_string, size_t data_len);
static void init_modem_params(const char* apn, const char* user, const char* pass);

static void await_serial();
static void await_long_serial();

/*public functions*/
void setup_lte();
void upload_certificate();
void build_body_as_JSON(char **result_json);
void aqm_mqtt_lte_send_data();

#endif