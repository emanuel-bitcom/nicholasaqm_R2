#ifndef AQM_I2C_TEMP_H
#define AQM_I2C_TEMP_H

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <aqm_global_data.h>
#include "hdc2021.h"
#include "hdc3022.h"
#include "humid_ctrl.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "aqm_nvs_high.h"
#include "humid_ctrl.h"


int start_temp_svc();
extern void pm_heater_off();
void temp_fun(void* params);

#endif