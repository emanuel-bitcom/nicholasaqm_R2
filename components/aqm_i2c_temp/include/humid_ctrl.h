#ifndef HUMID_CTRL
#define HUMID_CTRL

#include "hdc2021.h"
#include <aqm_global_data.h>
#include <stdio.h>
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/*public variables*/
extern hdc2021_t hdc_tube;
extern const int heat_ctrl_flag;
extern TaskHandle_t humid_ctrl_handle;

/*public functions*/
void pm_humid_ctrl_fun(void *args);
void pm_heater_off();


#endif