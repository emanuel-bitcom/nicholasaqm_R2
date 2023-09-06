#ifndef I2C_COMMON_H
#define I2C_COMMON_H

#include <stdio.h>

#include <esp_log.h>
#include <driver/i2c.h>

#define SDA_IO (21) /*!< gpio number for I2C master data  */
#define SCL_IO (22) /*!< gpio number for I2C master clock */

#define FREQ_HZ (100000)   /*!< I2C master clock frequency */
#define TX_BUF_DISABLE (0) /*!< I2C master doesn't need buffer */
#define RX_BUF_DISABLE (0) /*!< I2C master doesn't need buffer */

#define I2C_NUM I2C_NUM_0               /*!< I2C number */
#define I2C_MODE I2C_MODE_MASTER        /*!< I2C mode to act as */
#define I2C_RX_BUF_STATE RX_BUF_DISABLE /*!< I2C set rx buffer status */
#define I2C_TX_BUF_STATE TX_BUF_DISABLE /*!< I2C set tx buffer status */
#define I2C_INTR_ALOC_FLAG (0)          /*!< I2C set interrupt allocation flag */

/*
Connected devices addresses
*/
#define ADS1_ADDR 0x48
#define ADS2_ADDR 0x49

#define HDC_EXT_ADDR 0x40
#define HDC_TUBE_ADDR 0x41

int init_i2c_driver();
esp_err_t send_i2c_data(uint8_t _dev_addr, uint8_t *_buf, uint8_t _buf_len);
esp_err_t receive_i2c_data(uint8_t _dev_addr, uint8_t *_buf, uint8_t data_len);
esp_err_t send_receive_i2c_data(uint8_t _dev_addr, uint8_t *_tx_buf, uint8_t _tx_buf_len, 
                                uint8_t *_rx_buf, uint8_t _rx_buf_len);


#endif