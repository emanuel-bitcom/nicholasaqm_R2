#include "i2c_common.h"
#include "aqm_global_data.h"

static const char *TAG = "I2C";

/* i2c setup ----------------------------------------- */
// Config profile for espressif I2C lib
i2c_config_t i2c_cfg = {
    .mode = I2C_MODE_MASTER,
    .sda_io_num = SDA_IO,
    .scl_io_num = SCL_IO,
    .sda_pullup_en = GPIO_PULLUP_DISABLE,
    .scl_pullup_en = GPIO_PULLUP_DISABLE,
    .master.clk_speed = FREQ_HZ,
};

esp_err_t init_i2c_driver()
{
    // Setup I2C
    i2c_param_config(I2C_NUM, &i2c_cfg);
    esp_err_t ret = ESP_OK;
    if ((ret = i2c_driver_install(I2C_NUM, I2C_MODE, I2C_RX_BUF_STATE, I2C_TX_BUF_STATE, I2C_INTR_ALOC_FLAG)) == ESP_OK)
        I2C_INIT_OK = 1;
    else
        I2C_INIT_OK = 0;

    return ret;
}

esp_err_t send_i2c_data(uint8_t _dev_addr, uint8_t *_buf, uint8_t _buf_len)
{
    i2c_cmd_handle_t cmd_handle = i2c_cmd_link_create();
    i2c_master_start(cmd_handle);
    i2c_master_write_byte(cmd_handle, (_dev_addr << 1) | I2C_MASTER_WRITE, true);
    for (uint8_t index = 0; (_buf_len--) > 0; i2c_master_write_byte(cmd_handle, _buf[index++], true))
        ;
    i2c_master_stop(cmd_handle);

    //launch the created command
    esp_err_t my_error=i2c_master_cmd_begin(I2C_NUM, cmd_handle, 1000);
    i2c_cmd_link_delete(cmd_handle);

    return my_error;
}

esp_err_t receive_i2c_data(uint8_t _dev_addr, uint8_t *_buf, uint8_t data_len)
{
    i2c_cmd_handle_t cmd_handle = i2c_cmd_link_create();
    i2c_master_start(cmd_handle);
    i2c_master_write_byte(cmd_handle, (_dev_addr << 1) | I2C_MASTER_READ, true);
    i2c_master_read(cmd_handle, _buf, data_len, I2C_MASTER_ACK);
    i2c_master_stop(cmd_handle);

    //launch the created command
    esp_err_t my_error=i2c_master_cmd_begin(I2C_NUM, cmd_handle, 1000);
    i2c_cmd_link_delete(cmd_handle);

    return my_error;
}

esp_err_t send_receive_i2c_data(uint8_t _dev_addr, uint8_t *_tx_buf, uint8_t _tx_buf_len, 
                                uint8_t *_rx_buf, uint8_t _rx_buf_len)
{
    i2c_cmd_handle_t cmd_handle = i2c_cmd_link_create();
    i2c_master_start(cmd_handle);
    i2c_master_write_byte(cmd_handle, (_dev_addr << 1) | I2C_MASTER_WRITE, true);
    for (uint8_t index = 0; (_tx_buf_len--) > 0; i2c_master_write_byte(cmd_handle, _tx_buf[index++], true))
        ;
    i2c_master_start(cmd_handle);
    i2c_master_write_byte(cmd_handle, (_dev_addr << 1) | I2C_MASTER_READ, true);
    uint8_t index = 0;
    for (index = 0; (_rx_buf_len--) > 1; i2c_master_read_byte(cmd_handle, &_rx_buf[index++], I2C_MASTER_ACK));
         ;
    i2c_master_read_byte(cmd_handle, &_rx_buf[index++], I2C_MASTER_NACK);


    i2c_master_stop(cmd_handle);

    //launch the created command
    esp_err_t my_error=i2c_master_cmd_begin(I2C_NUM, cmd_handle, 1000);
    i2c_cmd_link_delete(cmd_handle);

    return my_error;
}