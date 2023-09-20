#include "aqm_interval.h"

/*private variables*/
long int measurements_interval=N_INTERVAL*60*1000;
long int wifi_timeout=WIFI_TIMEOUT*60*1000;
TaskHandle_t interval_task_handle;
const char* TAG="INTERVAL CONTROL";

void init_interval_control(){
    /*Get interval data from nvs*/
    //set timing variables from memory
    interval_config_t my_interval;
    nvs_high_read_interval(&my_interval);
    if(my_interval.interval)
    measurements_interval=(my_interval.interval)*60*1000;

    /*Get wifi timeout from nvs memory*/
    wifi_aqm_config_t my_wifi_config={.timeout=0};
    nvs_high_read_wifi(&my_wifi_config);
    if(my_wifi_config.timeout)
    wifi_timeout=my_wifi_config.timeout*60*1000;

    xTaskCreatePinnedToCore(
        interval_fun,
        "interval control task",
        2000,
        NULL,
        3,
        &interval_task_handle,
        1
    );
}

static void interval_fun(void *params){

    //this function also implements the timeout
    xEventGroupWaitBits(sensors_evt_grp, wifi_interface_done, false, true, wifi_timeout/portTICK_PERIOD_MS);

    /*If wifi interface is still open, close it*/
    EventBits_t uxResult = xEventGroupGetBits(sensors_evt_grp);
    if(!(uxResult & wifi_interface_done)){
        ESP_LOGW(TAG, "Forcing WiFi interface close");
        xEventGroupSetBits(sensors_evt_grp, wifi_interface_done);
    }

    while(true){
        ESP_LOGI(TAG, "Starting heater if necessary");
        xEventGroupSetBits(heat_ctrl_evt_grp, heat_ctrl_flag);

        ESP_LOGI(TAG, "Waiting for preheating if neccessary (timeout: 4 mins)");
        xEventGroupWaitBits(heat_ctrl_evt_grp, heat_interval_flag, true, true, pdMS_TO_TICKS(4*60*1000));

        ESP_LOGI(TAG, "Starting Sensors readings");
        xEventGroupSetBits(data_sent_evt_grp, 0xff);
        vTaskDelay(measurements_interval/portTICK_PERIOD_MS);
    }

    /*should never reach here*/
    vTaskDelete(interval_task_handle);
}