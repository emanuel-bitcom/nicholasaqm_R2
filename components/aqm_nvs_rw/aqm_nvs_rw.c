#include "aqm_nvs_rw.h"

/*private defs*/
#define N_PART 2
#define NAMESPACE "store"

/*private variables*/
static const char *TAG = "NVS_DRIVER";
nvs_handle my_nvs_handle;
_Bool partition_init[N_PART] = {0, 0};
char *partition_names[N_PART] = {
    "temp_nvs",
    "config_nvs"};

esp_err_t nvs_init_partition(nvs_scope_t nvs_scope)
{
    /*if there is already a partition opened, return*/
    uint8_t sum = 0;
    for (int i = 0; i < N_PART; sum += partition_init[i], i++)
        ;
    if (sum != 0)
    {
        ESP_LOGE(TAG, "A partition is already opened");
        return ESP_ERR_INVALID_STATE;
    }

    esp_err_t rez = nvs_flash_init_partition(partition_names[nvs_scope]);

    switch (rez)
    {
    case ESP_OK:
        ESP_LOGI(TAG, "Partition %s init OK", partition_names[nvs_scope]);
        break;

    default:
        ESP_LOGE(TAG, "Could not init partition");
        return ESP_FAIL;
        break;
    }

    esp_err_t result = nvs_open_from_partition(partition_names[nvs_scope], NAMESPACE, NVS_READWRITE, &my_nvs_handle);
    switch (result)
    {
    case ESP_OK:
        ESP_LOGI(TAG, "Partition %s opened successfully", partition_names[nvs_scope]);
        partition_init[nvs_scope] = 1;
        break;

    default:
        ESP_LOGE(TAG, "Error: %s", esp_err_to_name(result));
        break;
    }

    return result;
}

esp_err_t nvs_store_blob(const char *key, const void *data, size_t data_size, nvs_scope_t nvs_scope)
{
    /*check if required partition is initialized*/
    int tries;
    for (tries = 0; (!is_init(nvs_scope)) && (tries < 5); tries++)
    {
        ESP_LOGW(TAG, "Required partition is not opened; trying to open");
        nvs_init_partition(nvs_scope);
        vTaskDelay(10);
    }
    if (tries == 5)
    {
        ESP_LOGE(TAG, "Could not open %s partition", partition_names[nvs_scope]);
        return ESP_ERR_NOT_FOUND;
    }

    esp_err_t result = nvs_set_blob(my_nvs_handle, key, data, data_size);

    switch (result)
    {
    case ESP_OK:
        ESP_LOGI(TAG, "Blob set successfully");
        break;

    default:
        ESP_LOGE(TAG, "Blob not set - Error: %s", esp_err_to_name(result));
        break;
    }

    nvs_commit(my_nvs_handle);

    return result;
}

esp_err_t nvs_retrieve_blob(const char *key, void *out_blob_value, size_t rec_data_len,nvs_scope_t nvs_scope)
{
    /*check if required partition is initialized*/
    int tries;
    for (tries = 0; (!is_init(nvs_scope)) && tries < 5; tries++)
    {
        ESP_LOGW(TAG, "Required partition is not opened; trying to open");
        nvs_init_partition(nvs_scope);
        vTaskDelay(10);
    }
    if (tries == 5)
    {
        ESP_LOGE(TAG, "Could not open %s partition", partition_names[nvs_scope]);
        return ESP_ERR_NOT_FOUND;
    }

    esp_err_t result = nvs_get_blob(my_nvs_handle, key, out_blob_value, &rec_data_len);

    switch (result)
    {
    case ESP_OK:
        ESP_LOGI(TAG, "Blob retrieved successfully");
        break;
    default:
        ESP_LOGE(TAG, "Blob not retrieved - Error: %s", esp_err_to_name(result));
        break;
    }

    return result;
}

static _Bool is_init(nvs_scope_t nvs_scope)
{
    return partition_init[nvs_scope];
}

void nvs_storage_close(){
    for(int i=0; i<N_PART; partition_init[i++]=0);
    nvs_close(my_nvs_handle);
}