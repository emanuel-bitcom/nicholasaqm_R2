#ifndef AQM_ETH_CONTROLLER_H
#define AQM_ETH_CONTROLLER_H

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stddef.h>
#include "sdkconfig.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"

#include "esp_netif.h"
#include "esp_eth.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "driver/gpio.h"
#include "esp_eth_enc28j60.h"
#include "driver/spi_master.h"
#include <mqtt_client.h>

#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"

#include "aqm_global_data.h"
#include "aqm_lte_controller.h"
#include "aqm_nvs_high.h"
#include "aqm_nvs_keys.h"

/*Public Variables*/
#define ETH_CS_PIN 15
#define ETH_INT_PIN 14

extern const int eth_flag;


/*Public Functions*/
void aqm_mqtt_eth_publish_data();
void init_ETH();

/*Private functions*/
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data);
static void mqtt_app_start(void);
static void mqtt_app_stop(void);


#endif