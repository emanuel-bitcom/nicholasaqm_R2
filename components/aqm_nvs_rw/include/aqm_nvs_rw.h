#ifndef AQM_RW_H
#define AQM_RW_H

#include "aqm_global_data.h"
#include "nvs.h"
#include "nvs_flash.h"

/*public variables*/
typedef enum {
    NVS_TEMPORARY = 0,
    NVS_CONFIGURATION
}nvs_scope_t;

/*public functions*/
esp_err_t nvs_init_partition(nvs_scope_t nvs_scope);
esp_err_t nvs_store_blob(const char* key, const void* data, size_t data_size, nvs_scope_t nvs_scope);
esp_err_t nvs_retrieve_blob(const char* key,void * out_blob_value, size_t rec_data_len, nvs_scope_t nvs_scope);
void nvs_storage_close();

/*private functions*/
static _Bool is_init(nvs_scope_t nvs_scope);


#endif
