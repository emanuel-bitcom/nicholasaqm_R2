#include "sensirion_i2c.h"
#include "i2c_common.h"

/*General includes*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "esp_system.h"
#include <esp_log.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "aqm_global_data.h"

int16_t sensirion_i2c_select_bus(uint8_t bus_idx){
    /**
     * Does nothing as there is only one i2c bus in this application
    */
   return 0;
}

void sensirion_i2c_init(void){
    /**
     * Does nothing as i2c driver is initialized globally in this application
    */
}

void sensirion_i2c_release(void){
    /**
     * Does nothing, i2c is managed globally
    */
}

int8_t sensirion_i2c_read(uint8_t address, uint8_t* data, uint16_t count){
    WAIT_I2C();
    int8_t ret = (int8_t)receive_i2c_data(address, data, (uint8_t)count);
    RELEASE_I2C();
    return ret;
}

int8_t sensirion_i2c_write(uint8_t address, const uint8_t* data,
                           uint16_t count){
    WAIT_I2C();
    int8_t ret = (int8_t)send_i2c_data(address, data, (uint8_t)count);
    RELEASE_I2C();
    return ret;
}

void sensirion_sleep_usec(uint32_t useconds){
    vTaskDelay(pdMS_TO_TICKS(useconds/1000));
}
