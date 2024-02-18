idf_component_register(SRCS "sensirion_common.c" "sensirion_i2c_idf.c" "sps_git_version.c" "sps30.c" "aqm_sps_ctrl.c"  
                       INCLUDE_DIRS include 
                       REQUIRES "i2c_common" "nvs_flash" "aqm_global_data" "driver" "aqm_nvs_rw")