#ifndef AQM_I2C_TOXIC_H
#define AQM_I2C_TOXIC_H

#include "i2c_common.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "nvs.h"
#include "nvs_flash.h"
#include "esp_system.h"
#include "ads1115.h"

#include "aqm_nvs_high.h"

void toxic_fun(void *arg); // FreeRTOS task for ADC loop
int start_toxic_svc(); //function to be called to start measurements
void get_conversion(uint8_t ads_num, int16_t result[]);


#endif