idf_component_register(SRCS "main.c"
                    INCLUDE_DIRS "."
                    REQUIRES "aqm_global_data" "aqm_i2c_temp" "aqm_i2c_toxic" "aqm_spi_controller" "aqm_lte_controller" "i2c_common" "test_module" "aqm_eth_controller"
                                "aqm_wifi_interface" "aqm_SD_controller" "aqm_interval_controller" "aqm_nvs_rw" "aqm_sps_controller")

spiffs_create_partition_image(spiffs ../site FLASH_IN_PROJECT)