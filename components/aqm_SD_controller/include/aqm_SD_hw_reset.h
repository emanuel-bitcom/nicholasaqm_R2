#ifndef SD_HW_RESET
#define SD_HW_RESET
/**
 * Once hardware reset button is pressed, esp32 is put into deep sleep with a very 
 * small timeout, then, the wake-up cause will trigger hardware reset
*/

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "esp_sleep.h"
#include "aqm_SD_controller.h"

#define PIN_HW_RESET 35

void init_hw_reset();

#endif