#include "hdc2021.h"

hdc2021_err_t hdc2021_set_config(hdc2021_t *hdc_dev_struct, uint8_t device_address, hdc2021_meas_config_t measurement_config, hdc2021_meas_mode_t measurement_mode)
{
    hdc_dev_struct->config.field.meas_config = measurement_config;
    hdc_dev_struct->config.field.meas_trig = 1; // this will start measurement
    hdc_dev_struct->config.field.cc = measurement_mode;
    hdc_dev_struct->dev_addr = device_address;

    /*
     *set device Config Via I2C
     */
    uint8_t dev_config_reg = (hdc_dev_struct->config.field.cc) << 4;
    uint8_t data_buffer[2] = {HDC2021_DEV_CONFIG_ADDR, dev_config_reg};
    // set and send data
    hdc2021_set_data(hdc_dev_struct, data_buffer, 2);
    hdc2021_send_data(hdc_dev_struct);

    /**
     * Set Measurement Mode via I2C
     */
    uint8_t meas_mode_reg = 0x00 | ((hdc_dev_struct->config.field.meas_config) << 1) | (hdc_dev_struct->config.field.meas_trig);
    uint8_t meas_data_buffer[2] = {HDC2021_MEAS_CONFIG_ADDR, meas_mode_reg};
    // set and send data
    hdc2021_set_data(hdc_dev_struct, meas_data_buffer, 2);
    hdc2021_send_data(hdc_dev_struct);

    return HDC_NO_ERR;
}

hdc2021_err_t hdc2021_get_temp_humid(hdc2021_t *hdc_dev_struct, double *temperature, double *humidity)
{
    uint8_t reg_address=HDC2021_TEMP_L_ADDR;
    //set data to access first register
    //receive measured data
     esp_err_t my_error = send_receive_i2c_data(hdc_dev_struct->dev_addr, &reg_address, 1, hdc_dev_struct->rw_buff, 4);

    /**
     * process data
    */
    uint16_t TEMPERATURE=(hdc_dev_struct->rw_buff[0]) | ((hdc_dev_struct->rw_buff[1])<<8);
    *temperature=TEMPERATURE*165.0/65536-40;
    uint16_t HUMIDITY=(hdc_dev_struct->rw_buff[2]) | ((hdc_dev_struct->rw_buff[3])<<8);
    *humidity=HUMIDITY*100.0/65536;

    return (hdc2021_err_t)my_error;
}


hdc2021_err_t hdc2021_set_data(hdc2021_t *hdc_dev_struct, uint8_t *data_buf, uint8_t data_len)
{
    hdc_dev_struct->rw_buff_len = data_len;
    for (uint8_t i = 0; i < data_len; i++)
    {
        hdc_dev_struct->rw_buff[i] = data_buf[i];
    }
    return HDC_NO_ERR;
}

hdc2021_err_t hdc2021_send_data(hdc2021_t *hdc_dev_struct)
{
    // send the data via i2c
    esp_err_t my_error = send_i2c_data(hdc_dev_struct->dev_addr, hdc_dev_struct->rw_buff, hdc_dev_struct->rw_buff_len);
    if (my_error != ESP_OK)
    {
        return HDC_NO_DEVICE;
    }
    return HDC_NO_ERR;
}

hdc2021_err_t hdc2021_recv_data(hdc2021_t *hdc_dev_struct, uint8_t data_len)
{
    esp_err_t my_error = receive_i2c_data(hdc_dev_struct->dev_addr, hdc_dev_struct->rw_buff, data_len);
    if (my_error != ESP_OK)
    {
        return HDC_NO_DEVICE;
    }
    hdc_dev_struct->rw_buff_len = data_len;
    return HDC_NO_ERR;
}