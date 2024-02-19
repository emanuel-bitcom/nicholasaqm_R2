#ifndef _AQM_SPI_CONTROLLER_H
#define _AQM_SPI_CONTROLLER_H

#include <stdio.h>
#include "aqm_global_data.h"
#include <string.h>
#include "esp_system.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "aqm_nvs_high.h"

#define TAG_SPI "SPI_OPC"

extern esp_err_t ret;
extern spi_device_handle_t spi;
extern spi_bus_config_t buscfg;



#define OPC_CS_PIN 16
#define OPC_HOST HSPI_HOST

extern spi_device_interface_config_t devcfg;

void aqm_spi_init();
void aqm_spi_init_raw();

extern int opc_power_ON();
extern int opc_power_OFF();


void cs_high();
void cs_low();

void read_pm(float *PM1, float *PM2_5, float *PM10);
void read_opc(uint8_t command, uint8_t *result);
void read_opc_bytes(uint8_t n_bytes, uint8_t *my_tx_buffer, uint8_t *my_rx_buffer);
int get_OPC_rdy_response(uint8_t cmd_byte);
void read_opc_n_async(uint8_t n_bytes, uint8_t *my_tx_buffer, uint8_t *my_rx_buffer);
void read_opc_n_bytes(uint8_t n_bytes, uint8_t *my_tx_buffer, uint8_t *my_rx_buffer);
void timer_callback(void *args);
void opc_main_fun(void *params);

extern void opc_deinit();

#endif