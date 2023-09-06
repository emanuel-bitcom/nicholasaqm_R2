#ifndef TEST_MODULE_H
#define TEST_MODULE_H

#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "esp_types.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "aqm_global_data.h"

#define MAX_SUPPORTED_SERVERS 10

#define TAG_TEST "test"


void load_test_sta_cal();

void load_test_lte();


#endif