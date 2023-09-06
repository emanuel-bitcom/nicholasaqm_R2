#ifndef HDC2021_H
#define HDC2021_H

#include <i2c_common.h>

/* ESP32 Specifics ------------------------*/
#define HDC2021_DEBUG_LEVEL ESP_LOG_DEBUG

/* Register Addresses ---------------------*/
#define HDC2021_TEMP_L_ADDR 0x00
#define HDC2021_TEMP_H_ADDR 0x01
#define HDC2021_HUMID_L_ADDR 0x02
#define HDC2021_HUMID_H_ADDR 0x03
#define HDC2021_STATUS_ADDR 0x04
#define HDC2021_DEV_CONFIG_ADDR 0x0E
#define HDC2021_MEAS_CONFIG_ADDR 0x0F

typedef enum hdc2021_err
{
    HDC_NO_ERR = 0,
    HDC_NO_DEVICE = 1
} hdc2021_err_t;

typedef enum
{
    humid_temp = 0,
    temp_only = 1
} hdc2021_meas_config_t;

typedef enum
{
    one_shot = 0,
    two_mins = 1,
    one_min = 2,
    ten_secs = 3,
    five_secs = 4,
    one_sec = 5,
    half_sec = 6,
    fifth_sec = 7
} hdc2021_meas_mode_t;

typedef struct hdc2021
{
    union
    {
        struct
        {
            unsigned int cc : 3;
            unsigned int meas_config : 2;
            unsigned int meas_trig : 1;
        }field;
         uint8_t word;
    } config;
    uint8_t rw_buff[4];
    uint8_t rw_buff_len;
    uint8_t dev_addr;
} hdc2021_t;

/*
 *  Function Prototypes
 */
hdc2021_err_t hdc2021_set_config(hdc2021_t *hdc_dev_struct, uint8_t device_address,hdc2021_meas_config_t measurement_config, hdc2021_meas_mode_t measurement_mode);
hdc2021_err_t hdc2021_set_data(hdc2021_t *hdc_dev_struct, uint8_t *data_buf, uint8_t data_len);
hdc2021_err_t hdc2021_send_data(hdc2021_t *hdc_dev_struct);
hdc2021_err_t hdc2021_recv_data(hdc2021_t *hdc_dev_struct, uint8_t data_len);
hdc2021_err_t hdc2021_get_temp_humid(hdc2021_t *hdc_dev_struct, double *temperature, double *humidity);


#endif