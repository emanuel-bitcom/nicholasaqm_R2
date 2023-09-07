#include "aqm_SD_controller.h"

static const char *TAG = "SD_DRIVER";

/**
 * Private variables
*/
char* config_names[6]={
    "interval_config.json",
    "mqtt_config.json",
    "lte_config.json",
    "lorawan_config.json",
    "wifi_config.json",
    "eth_config.json"
};

char *config_paths[2]={
    "/store/root_config/",
    "/store/config/"
};


esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = 5,
        .allocation_unit_size = 16 * 1024};

spi_bus_config_t spi_bus_config = {
    .mosi_io_num = AQM_MOSI_PIN,
    .miso_io_num = AQM_MISO_PIN,
    .sclk_io_num = AQM_SCK_PIN,
    .quadhd_io_num = -1,
    .quadwp_io_num = -1};

static const char *BASE_PATH = "/store";

esp_err_t load_calib_data_to_nvs(){
    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    ESP_ERROR_CHECK(spi_bus_initialize(host.slot, &spi_bus_config, SDSPI_DEFAULT_DMA));

    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs = SD_CS_PIN;
    slot_config.host_id = host.slot;

    sdmmc_card_t *card;
    esp_err_t mount_err=esp_vfs_fat_sdspi_mount(BASE_PATH, &host, &slot_config, &mount_config, &card);
    ESP_LOGI(TAG, "Mounting error is: %d", (int)mount_err);
    
    /**
     * in case of error or if there is no card
    */
    if(mount_err){
        esp_err_t ret_error=ESP_OK;
        if(mount_err!=ESP_FAIL)
        ret_error+=esp_vfs_fat_sdcard_unmount(BASE_PATH, card);
        ret_error+=spi_bus_free(host.slot);
        return ret_error;
    }

    sdmmc_card_print_info(stdout, card);

    /**
     * Load calibration data
    */
    station_cal SD_calib;
    cJSON *p_SD_calib=NULL;
    read_file_json("/store/calib/gps.json", &p_SD_calib);
    if(p_SD_calib){
        SD_calib.pos_latitude=cJSON_GetObjectItem(p_SD_calib,"lat")->valuedouble;
        SD_calib.pos_longitude=cJSON_GetObjectItem(p_SD_calib, "long")->valuedouble;
        free(p_SD_calib);
        ESP_LOGI(TAG, "Loaded calib gps data");
    }
    read_file_json("/store/calib/tox_mV.json", &p_SD_calib);
    if(p_SD_calib){
        char sub_key[15];
        for(uint8_t i=1; i<=4;i++){
            /*sensor type*/
            memset(SD_calib.SEN_cal[i].type, 0, 5);
            memset(sub_key,0,15);
            sprintf(sub_key,"SEN%d_t",i);
            strcpy(SD_calib.SEN_cal[i].type, cJSON_GetObjectItem(p_SD_calib,(const char*)sub_key)->valuestring); 

            /*sensor WEe*/
            memset(sub_key,0,15);
            sprintf(sub_key,"SEN%d_WEe",i);
            SD_calib.SEN_cal[i].WEe=cJSON_GetObjectItem(p_SD_calib, (const char*)sub_key)->valueint;

            /*sensor AEe*/
            memset(sub_key,0,15);
            sprintf(sub_key,"SEN%d_AEe",i);
            SD_calib.SEN_cal[i].AEe=cJSON_GetObjectItem(p_SD_calib, (const char*)sub_key)->valueint;

            /*sensor A0*/
            memset(sub_key,0,15);
            sprintf(sub_key,"SEN%d_A0",i);
            SD_calib.SEN_cal[i].AE0=cJSON_GetObjectItem(p_SD_calib, (const char*)sub_key)->valueint;

            /*sensor W0*/
            memset(sub_key,0,15);
            sprintf(sub_key,"SEN%d_W0",i);
            SD_calib.SEN_cal[i].WE0=cJSON_GetObjectItem(p_SD_calib, (const char*)sub_key)->valueint;

            /*sensor sensit 1*/
            memset(sub_key,0,15);
            sprintf(sub_key,"SEN%d_S",i);
            SD_calib.SEN_cal[i].sensit=cJSON_GetObjectItem(p_SD_calib, (const char*)sub_key)->valueint;

            /*sensor sensit2*/
            /*aka NO2 sensit*/
            memset(sub_key,0,15);
            sprintf(sub_key,"SEN%d_S2",i);
            SD_calib.SEN_cal[i].NO2_sensit=cJSON_GetObjectItem(p_SD_calib, (const char*)sub_key)->valueint;

            /*sensor gain*/
            /*it is a double value*/
            memset(sub_key,0,15);
            sprintf(sub_key,"SEN%d_G",i);
            SD_calib.SEN_cal[i].gain=cJSON_GetObjectItem(p_SD_calib,(const char*)sub_key)->valuedouble;

        }

        free(p_SD_calib);
        ESP_LOGI(TAG, "Loaded calib mV data");
    }
    
    read_file_json("/store/serial_number.json", &p_SD_calib);
    if(p_SD_calib){
        SD_calib.station_SN=cJSON_GetObjectItem(p_SD_calib, "SN")->valueint;
        free(p_SD_calib);
        ESP_LOGI(TAG, "Loaded calib SN data");
    }

    /**
     *  store calibration to nvs
    */
    nvs_store_blob(NVS_KEY_CALIB, (void*)&SD_calib, sizeof(SD_calib), NVS_CONFIGURATION);
    nvs_storage_close();
    

    esp_err_t ret_error=ESP_OK;
    ret_error+=esp_vfs_fat_sdcard_unmount(BASE_PATH, card);
    ret_error+=spi_bus_free(host.slot);
    return ret_error;
}

esp_err_t load_config_data_to_nvs(){
    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    ESP_ERROR_CHECK(spi_bus_initialize(host.slot, &spi_bus_config, SDSPI_DEFAULT_DMA));

    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs = SD_CS_PIN;
    slot_config.host_id = host.slot;

    sdmmc_card_t *card;
    esp_err_t mount_err=esp_vfs_fat_sdspi_mount(BASE_PATH, &host, &slot_config, &mount_config, &card);
    ESP_LOGI(TAG, "Mounting error is: %d", (int)mount_err);
    
    /**
     * in case of error or if there is no card
    */
    if(mount_err){
        esp_err_t ret_error=ESP_OK;
        if(mount_err!=ESP_FAIL)
        ret_error+=esp_vfs_fat_sdcard_unmount(BASE_PATH, card);
        ret_error+=spi_bus_free(host.slot);
        return ret_error;
    }

    sdmmc_card_print_info(stdout, card);

    /**
     * Load configuration data by category
    */
    interval_config_t SD_interval_config;
    memset(&SD_interval_config,0,sizeof(SD_interval_config));
    cJSON *p_SD_config=NULL;

    /*interval configuration*/
    read_file_json("/store/config/interval_config.json", &p_SD_config);
    if(p_SD_config){
        SD_interval_config.interval=cJSON_GetObjectItem(p_SD_config, "interval")->valueint;
        SD_interval_config.measuring_time=cJSON_GetObjectItem(p_SD_config, "measuring_time")->valueint;
        SD_interval_config.repetitions=cJSON_GetObjectItem(p_SD_config, "repetitions")->valueint;
        free(p_SD_config);
        ESP_LOGI(TAG, "Loaded interval config data");
    }

    /*mqtt configuration*/
    mqtt_conn_config_t SD_mqtt_config;
    memset(&SD_mqtt_config,0,sizeof(SD_mqtt_config));
    read_file_json("/store/config/mqtt_config.json", &p_SD_config);
    if(p_SD_config){
         strcat(SD_mqtt_config.url, (const char*)cJSON_GetObjectItem(p_SD_config, "URL")->valuestring);
         strcat(SD_mqtt_config.username, (const char*)cJSON_GetObjectItem(p_SD_config, "uname")->valuestring);
         strcat(SD_mqtt_config.password, (const char*)cJSON_GetObjectItem(p_SD_config, "pwd")->valuestring);
         SD_mqtt_config.port=cJSON_GetObjectItem(p_SD_config,"port")->valueint;
         free(p_SD_config);
         ESP_LOGI(TAG, "Loaded mqtt config data");
    }

    /*lte configuration*/
    lte_data SD_lte_config;
    memset(&SD_lte_config,0,sizeof(SD_lte_config));
    read_file_json("/store/config/lte_config.json", &p_SD_config);
    if(p_SD_config){
         strcat(SD_lte_config.lte_apn, (const char*)cJSON_GetObjectItem(p_SD_config, "APN")->valuestring);
         strcat(SD_lte_config.lte_user, (const char*)cJSON_GetObjectItem(p_SD_config, "uname")->valuestring);
         strcat(SD_lte_config.lte_pass, (const char*)cJSON_GetObjectItem(p_SD_config, "pwd")->valuestring);
         free(p_SD_config);
         ESP_LOGI(TAG, "Loaded lte config data");
    }

    /*lorawan configuration*/
    lorawan_config_t SD_lwan_config;
    memset(&SD_lwan_config, 0, sizeof(SD_lwan_config));
    read_file_json("/store/config/lorawan_config.json", &p_SD_config);
    if(p_SD_config){
         strcat(SD_lwan_config.AppEUI, (const char*)cJSON_GetObjectItem(p_SD_config, "AppEUI")->valuestring);
         strcat(SD_lwan_config.AppKEY, (const char*)cJSON_GetObjectItem(p_SD_config, "AppKEY")->valuestring);
         free(p_SD_config);
         ESP_LOGI(TAG, "Loaded lorawan config data");
    }

    /*ethernet mac configuration*/
    eth_config_t SD_eth_config;
    memset(&SD_eth_config,0,sizeof(SD_eth_config));
    read_file_json("/store/config/eth_config.json", &p_SD_config);
    if(p_SD_config){
        strcat(SD_eth_config.mac, (const char*)cJSON_GetObjectItem(p_SD_config,"mac")->valuestring);
        free(p_SD_config);
        ESP_LOGI(TAG, "Loaded ethernet config data");
    }

    /*wifi configuration*/
    wifi_aqm_config_t SD_wifi_config;
    memset(&SD_wifi_config,0, sizeof(SD_wifi_config));
    read_file_json("/store/config/wifi_config.json", &p_SD_config);
    if(p_SD_config){
        cJSON *Wifi_json = cJSON_GetObjectItem(p_SD_config, "Wifi");
        if(Wifi_json){
            strcat(SD_wifi_config.wifi_ssid,(const char*)cJSON_GetObjectItem(Wifi_json, "SSID")->valuestring);
            strcat(SD_wifi_config.wifi_pwd,(const char*)cJSON_GetObjectItem(Wifi_json,"pwd")->valuestring);
        }
        cJSON *Web_json = cJSON_GetObjectItem(p_SD_config,"Website");
        if(Web_json){
            strcat(SD_wifi_config.web_uname, (const char*)cJSON_GetObjectItem(Web_json,"uname")->valuestring);
            strcat(SD_wifi_config.web_pwd, (const char*)cJSON_GetObjectItem(Web_json,"pwd")->valuestring);
        }
        SD_wifi_config.timeout=cJSON_GetObjectItem(p_SD_config, "timeout")->valueint;
        free(p_SD_config);
        ESP_LOGI(TAG, "Loaded wifi config data");
    }

    /**
    *  store configuration to nvs
    */
    nvs_store_blob(NVS_KEY_CONFIG_MQTT, (void*)&SD_mqtt_config, sizeof(SD_mqtt_config),NVS_CONFIGURATION);
    nvs_store_blob(NVS_KEY_CONFIG_INTERVAL, (void*)&SD_interval_config, sizeof(SD_interval_config), NVS_CONFIGURATION);
    nvs_store_blob(NVS_KEY_CONFIG_LTE, (void*)&SD_lte_config, sizeof(SD_lte_config), NVS_CONFIGURATION);
    nvs_store_blob(NVS_KEY_CONFIG_LORAWAN, (void*)&SD_lwan_config, sizeof(SD_lwan_config), NVS_CONFIGURATION);
    nvs_store_blob(NVS_KEY_CONFIG_WIFI,(void*)&SD_wifi_config, sizeof(SD_wifi_config), NVS_CONFIGURATION);
    nvs_store_blob(NVS_KEY_CONFIG_ETHERNET, (void*)&SD_eth_config, sizeof(SD_eth_config), NVS_CONFIGURATION);
    nvs_storage_close();

    esp_err_t ret_error=ESP_OK;
    ret_error+=esp_vfs_fat_sdcard_unmount(BASE_PATH, card);
    ret_error+=spi_bus_free(host.slot);
    return ret_error;
}


esp_err_t store_config_data_to_SD(){

    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    ESP_ERROR_CHECK(spi_bus_initialize(host.slot, &spi_bus_config, SDSPI_DEFAULT_DMA));

    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs = SD_CS_PIN;
    slot_config.host_id = host.slot;

    sdmmc_card_t *card;
    esp_err_t mount_err=esp_vfs_fat_sdspi_mount(BASE_PATH, &host, &slot_config, &mount_config, &card);
    ESP_LOGI(TAG, "Mounting error is: %d", (int)mount_err);
    
    /**
     * in case of error or if there is no card
    */
    if(mount_err){
        esp_err_t ret_error=ESP_OK;
        if(mount_err!=ESP_FAIL)
        ret_error+=esp_vfs_fat_sdcard_unmount(BASE_PATH, card);
        ret_error+=spi_bus_free(host.slot);
        return ret_error;
    }

    sdmmc_card_print_info(stdout, card);

    lorawan_config_t SD_lwan_config;
    lte_data SD_lte_config;
    mqtt_conn_config_t SD_mqtt_config;
    interval_config_t SD_interval_config;

    /*Open Config data from nvs*/
    nvs_retrieve_blob(NVS_KEY_CONFIG_MQTT, (void*)&SD_mqtt_config, sizeof(SD_mqtt_config),NVS_CONFIGURATION);
    nvs_retrieve_blob(NVS_KEY_CONFIG_INTERVAL, (void*)&SD_interval_config, sizeof(SD_interval_config), NVS_CONFIGURATION);
    nvs_retrieve_blob(NVS_KEY_CONFIG_LTE, (void*)&SD_lte_config, sizeof(SD_lte_config), NVS_CONFIGURATION);
    nvs_retrieve_blob(NVS_KEY_CONFIG_LORAWAN, (void*)&SD_lwan_config, sizeof(SD_lwan_config), NVS_CONFIGURATION);
    nvs_storage_close();

    /**
     * Write data to sd card by each category
    */

   /*interval*/
   cJSON *p_SD_config=NULL;
   p_SD_config = cJSON_CreateObject();
   if(p_SD_config){
   cJSON_AddNumberToObject(p_SD_config, "interval", SD_interval_config.interval);
   cJSON_AddNumberToObject(p_SD_config, "repetitions", SD_interval_config.repetitions);
   cJSON_AddNumberToObject(p_SD_config, "measuring_time", SD_interval_config.measuring_time);
   write_file_json("/store/config/interval_config.json", p_SD_config);
   free(p_SD_config);
   }

   p_SD_config=NULL;
   p_SD_config = cJSON_CreateObject();
   if(p_SD_config){
   cJSON_AddStringToObject(p_SD_config, "URL", SD_mqtt_config.url);
   cJSON_AddNumberToObject(p_SD_config, "port", SD_mqtt_config.port);
   cJSON_AddStringToObject(p_SD_config, "uname", SD_mqtt_config.username);
   cJSON_AddStringToObject(p_SD_config, "pwd", SD_mqtt_config.password);
   write_file_json("/store/config/mqtt_config.json", p_SD_config);
   free(p_SD_config);
   }

   p_SD_config=NULL;
   p_SD_config = cJSON_CreateObject();
   if(p_SD_config){
   cJSON_AddStringToObject(p_SD_config, "APN", SD_lte_config.lte_apn);
   cJSON_AddStringToObject(p_SD_config, "uname", SD_lte_config.lte_user);
   cJSON_AddStringToObject(p_SD_config, "pwd", SD_lte_config.lte_pass);
   write_file_json("/store/config/lte_config.json", p_SD_config);
   free(p_SD_config);
   }

   p_SD_config=NULL;
   p_SD_config = cJSON_CreateObject();
   if(p_SD_config){
   cJSON_AddStringToObject(p_SD_config, "DevEUI", SD_lwan_config.DevEUI);
   cJSON_AddStringToObject(p_SD_config, "AppEUI", SD_lwan_config.AppEUI);
   cJSON_AddStringToObject(p_SD_config, "AppKEY", SD_lwan_config.AppKEY);
   write_file_json("/store/config/lorawan_config.json", p_SD_config);
   free(p_SD_config);
   }



    /**
     * Unmount SD card
    */
    esp_err_t ret_error=ESP_OK;
    ret_error+=esp_vfs_fat_sdcard_unmount(BASE_PATH, card);
    ret_error+=spi_bus_free(host.slot);
    return ret_error;

    
}

void write_file_json(char*path, cJSON *p_file_json){
    ESP_LOGI(TAG, "writing json to file %s", path);
    FILE *file = fopen(path, "w");

    if(!file)
    {
        ESP_LOGE(TAG, "File not found or cannot be written");
        return;
    }

    char* j_string;
    j_string=NULL;
    j_string=cJSON_Print(p_file_json);
    if(j_string){
        fputs((const char*)j_string, file);
        free(j_string);
    }

    fclose(file);
}

void read_file_json(char *path, cJSON **p_file_json)
{
    ESP_LOGI(TAG, "reading file %s", path);
    FILE *file = fopen(path, "r");
    if(!file)
    {
        ESP_LOGE(TAG, "File not found or cannot be read");
        return;
    }

    ESP_LOGI(TAG, "file contains: ");

    /*file buffer holds the whole file content*/
    char file_buffer[FILE_MAX_LEN];
    memset(file_buffer,0,FILE_MAX_LEN);

    char buffer[CHUNCK_LEN];
    memset(buffer, 0, CHUNCK_LEN);
    int read_len=0;
    _Bool error_encountered=0;
    while(fgets(buffer, CHUNCK_LEN-1, file) !=NULL){
        printf("%s", buffer);
        if(read_len<FILE_MAX_LEN)
            strcat(file_buffer, (const char*)buffer);
        else
            {
                ESP_LOGE(TAG, "File too large");
                error_encountered=1;
                break;
            }
        memset(buffer, 0, CHUNCK_LEN);
        read_len+=CHUNCK_LEN;
    }

    fclose(file);

    /**
     * if the file was too large
    */
   if(error_encountered)return;

    /**
     * get json from buffer
    */
    cJSON *pt_file_json=cJSON_Parse(file_buffer);
    *p_file_json=pt_file_json;
}

void write_file(char *path, char *content)
{
    ESP_LOGI(TAG, "Writing \"%s\" to %s", content, path);
    FILE *file = fopen(path, "w");
    fputs(content, file);
    fclose(file);
}


esp_err_t load_root_to_config(){
    ESP_LOGW(TAG, "Root Config will be loaded to User config");

    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    ESP_ERROR_CHECK(spi_bus_initialize(host.slot, &spi_bus_config, SDSPI_DEFAULT_DMA));

    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs = SD_CS_PIN;
    slot_config.host_id = host.slot;

    sdmmc_card_t *card;
    esp_err_t mount_err=esp_vfs_fat_sdspi_mount(BASE_PATH, &host, &slot_config, &mount_config, &card);
    ESP_LOGI(TAG, "Mounting error is: %d", (int)mount_err);
    
    /**
     * in case of error or if there is no card
    */
    if(mount_err){
        esp_err_t ret_error=ESP_OK;
        if(mount_err!=ESP_FAIL)
        ret_error+=esp_vfs_fat_sdcard_unmount(BASE_PATH, card);
        ret_error+=spi_bus_free(host.slot);
        return ret_error;
    }

    /**
     * Copy from root to user config folders
    */
 
    char path[100];
    for(int i=0; i<4; i++){
        memset(path,0,100);
        sprintf(path,"%s%s", config_paths[0],config_names[i]);
        FILE *file_from=fopen(path,"r");
        
        memset(path,0,100);
        sprintf(path,"%s%s", config_paths[1],config_names[i]);
        FILE *file_to=fopen(path,"w");

        if(file_from&&file_to){
            /*Copy from one file to another*/
            int c=fgetc(file_from);
            while(c!=EOF){
                fputc(c,file_to);
                c=fgetc(file_from);
            }

            ESP_LOGI(TAG, "Successfully copied %s", config_names[i]);
        }

        if(file_from)fclose(file_from);
        if(file_to)fclose(file_to);

        }
        
    
     /**
     * Unmount SD card
    */
    esp_err_t ret_error=ESP_OK;
    ret_error+=esp_vfs_fat_sdcard_unmount(BASE_PATH, card);
    ret_error+=spi_bus_free(host.slot);
    return ret_error;

    return ESP_OK;

}