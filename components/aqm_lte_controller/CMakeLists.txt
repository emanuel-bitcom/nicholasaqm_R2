idf_component_register(SRCS "aqm_lte_controller.c" INCLUDE_DIRS include REQUIRES "nvs_flash" "cjson" "driver" "aqm_global_data" "aqm_nvs_rw"
EMBED_TXTFILES "cert/cert4.cer")