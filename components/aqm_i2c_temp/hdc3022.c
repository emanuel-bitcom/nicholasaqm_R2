#include "hdc3022.h"

/*private variables*/
#define HDC3022_I2C_ADDR 0x44
static char* TAG="HDC3022";
/**
 * corresponds to 1 meas per 2 secs
 * corresponds to low poser mode 1
*/
#define MEAS_AUT_H 0x21     
#define MEAS_AUT_L 0x30

#define MEAS_READ_H 0xE0
#define MEAS_READ_L 0x00

#define RES_H 0x30
#define RES_L 0xA2

#define CMD_LEN 2
uint8_t cmd_buff[2];

/*assume a maximum of 10 bytes received*/
#define T_H_REC_LEN 6
uint8_t receive_buff[10];

hdc3022_err_t hdc3022_configure_autonomous(){
    cmd_buff[0]=MEAS_AUT_H;
    cmd_buff[1]=MEAS_AUT_L;
    esp_err_t ret_err = send_i2c_data(HDC3022_I2C_ADDR, cmd_buff, CMD_LEN);
    if(ret_err!=ESP_OK){
        ESP_LOGE(TAG, "NO DEVICE FOUND");
        ESP_LOGE(TAG, "%s",esp_err_to_name(ret_err));
        return HDC3022_NO_DEVICE;
    }
    ESP_LOGI(TAG, "RESET SUCCESS");

    vTaskDelay(pdMS_TO_TICKS(100));
    
    
    cmd_buff[0]=MEAS_AUT_H;
    cmd_buff[1]=MEAS_AUT_L;

    ret_err = send_i2c_data(HDC3022_I2C_ADDR, cmd_buff, CMD_LEN);
    if(ret_err!=ESP_OK){
        ESP_LOGE(TAG, "NO DEVICE FOUND");
        ESP_LOGE(TAG, "%s",esp_err_to_name(ret_err));
        return HDC3022_NO_DEVICE;
    }
    ESP_LOGI(TAG, "SET SUCCESS");
    return HDC3022_NO_ERR;
}

hdc3022_err_t hdc3022_get_temp_humid(double *temperature, double *humidity){
    cmd_buff[0]=MEAS_READ_H;
    cmd_buff[1]=MEAS_READ_L;
    memset(receive_buff,0,10);

    esp_err_t ret_err = send_receive_i2c_data(HDC3022_I2C_ADDR, cmd_buff, CMD_LEN, receive_buff, T_H_REC_LEN);
    //esp_err_t ret_err=send_i2c_data(HDC3022_I2C_ADDR, cmd_buff, 2);
    //ret_err=receive_i2c_data(HDC3022_I2C_ADDR, receive_buff, 6);

     if(ret_err!=ESP_OK){
        ESP_LOGE(TAG, "NO DEVICE FOUND");
        return HDC3022_NO_DEVICE;
    }
    
    uint16_t TEMPERATURE = (receive_buff[0]<<8) | receive_buff[1];
    uint16_t HUMIDITY = (receive_buff[3]<<8)| receive_buff[4];

    /*process data to get final results*/
    *temperature=TEMPERATURE*175.0/65535-45;
    *humidity=HUMIDITY*100.0/65535;

    return HDC3022_NO_ERR;
}