#include <stdio.h>

#include <esp_log.h>
#include <driver/i2c.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "esp_heap_caps.h"

/*
Application Specific includes
*/
#include <aqm_global_data.h>
#include <aqm_i2c_toxic.h>
#include <aqm_i2c_temp.h>
#include <aqm_spi_controller.h>
#include <i2c_common.h>
#include <aqm_lte_controller.h>
#include <test_module.h>
#include <aqm_eth_controller.h>
#include <aqm_wifi_interface.h>
#include <aqm_SD_controller.h>
#include <aqm_interval.h>
#include <aqm_SD_hw_reset.h>

static const char *TAG = "Main";

/*DEBUG DEFINES*/
#define TEST_NO_WIFI_INTERFACE 0

void app_main(void)
{
  init_global_mutexes();

  // load a test configuration to have something in nvs
  // load_test_lte();
  // load_test_sta_cal();

  /**
   * Initialize hw reset structure
   * Check for hw reset event
  */
  init_hw_reset();

  /**
   * load data from SD card
   */
  load_calib_data_to_nvs();
  load_config_data_to_nvs();

  init_interval_control();

  /*possibility to run without a wifi config interface                                                                                                                                                                                                                                                         */
  #if TEST_NO_WIFI_INTERFACE
  xEventGroupSetBits(sensors_evt_grp, wifi_interface_done);
  #else
  init_wifi_interface();

  xEventGroupWaitBits(sensors_evt_grp, wifi_interface_done, false, true, portMAX_DELAY);
  deinit_wifi_interface();
  vTaskDelay(500);
  store_config_data_to_SD();
  #endif

  if (!I2C_INIT_OK)
    init_i2c_driver();

  if (start_toxic_svc())
  {
    ESP_LOGE(TAG, "Start Toxic Failed");
  };
  if (start_temp_svc())
  {
    ESP_LOGE(TAG, "Start Temp Failed");
  };
  aqm_spi_init();

  #ifdef USE_ETH
    init_ETH();
  #endif

  #ifdef USE_LTE
    setup_lte();
  #endif

  while (true)
  {
    // wait for sensors data to be ready
    xEventGroupWaitBits(sensors_evt_grp, opc_flag | toxic_flag | temp_flag, true, true, portMAX_DELAY);

    /*after sensors measurements, close heating ptc*/
    pm_heater_off();

    /* For debug purposes, print available heap memory */
    ESP_LOGW(TAG, "************************Memory*DEBUG************************************");
    ESP_LOGI(TAG, "xPortGetFreeHeapSize %ld = DRAM", (long int)xPortGetFreeHeapSize());

    int DRam = heap_caps_get_free_size(MALLOC_CAP_8BIT);
    int IRam = heap_caps_get_free_size(MALLOC_CAP_32BIT) - heap_caps_get_free_size(MALLOC_CAP_8BIT);

    ESP_LOGI(TAG, "DRAM \t\t %d", DRam);
    ESP_LOGI(TAG, "IRam \t\t %d", IRam);
    int free = heap_caps_get_largest_free_block(MALLOC_CAP_8BIT);
    ESP_LOGI(TAG, "free = %d", free);

    int stackmem = uxTaskGetStackHighWaterMark(NULL);
    ESP_LOGI(TAG, "stack space = %d", stackmem);
    ESP_LOGW(TAG, "*************************************************************************");

    ESP_LOGI(TAG, "Received sensors readings, now transmitting");

    /*Test if there is a potential LTE connection*/
    short is_lte=false;
    short is_eth=false;
    EventBits_t active_comm_bits = xEventGroupGetBits(comm_available_evt_grp);
    if((active_comm_bits&lte_flag)!=0)is_lte=true;
    if((active_comm_bits&eth_flag)!=0)is_eth=true;

    /*If there is lte available, but no ethernet, transmit on lte*/
    if((!is_eth)&&is_lte){

#ifdef USE_LTE
    aqm_mqtt_lte_send_data();
#endif

    }
    else{

#ifdef USE_ETH
    aqm_mqtt_eth_publish_data();
#endif

    }


  }
}
