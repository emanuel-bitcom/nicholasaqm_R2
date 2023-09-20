#include "humid_ctrl.h"

/*public variables definitions*/
const int heat_ctrl_flag=BIT4;
const int heat_interval_flag=BIT5;
TaskHandle_t humid_ctrl_handle;

/*private variable definitions*/
#define HUMID_TRESH_HIGH    50.0
#define HUMID_TRESH_LOW     45.0
#define DELAY_HEATING_MS    3*60*1000

#define PTC_SW 17
#define PIN_HIGH 1
#define PIN_LOW 0

/*0-off, 1-on*/
uint8_t ptc_state=0;

static const char* TAG="HUMID_CTRL";

/**
 * Controls humidity of air entering PM sensor by enabling a heater
 * - checks conditions before enabling sensor measurements
 * - if necesary turns on the heater for DELAY_HEATING_MS
 * - signals to the interval control process that ptc is heated (if neccessary)
*/
void pm_humid_ctrl_fun(void *args){
    
    /*initialize PTC control pin and set it to OFF*/
    gpio_set_direction(PTC_SW, GPIO_MODE_OUTPUT);
    gpio_set_level(PTC_SW, PIN_LOW);

    while(true){
        double humid, temp;

        xEventGroupWaitBits(heat_ctrl_evt_grp, heat_ctrl_flag, true, true, portMAX_DELAY);
        
        /*check admission air humidity*/
        hdc2021_get_temp_humid(&hdc_tube, &temp, &humid);
        /*debug*/
        ESP_LOGI(TAG, "measured humidity: %f", humid);
        if(humid>=HUMID_TRESH_HIGH){
            ptc_state=1;
        }
        else if(humid<=HUMID_TRESH_LOW){
            ptc_state=0;
        }

        /*if necessary turn on heater for required delay*/
        if(ptc_state==0){
            gpio_set_level(PTC_SW, PIN_LOW);
        } else if(ptc_state==1){
            /*debug*/
                ESP_LOGI(TAG, "heater power on and waiting 3 mins...");
            gpio_set_level(PTC_SW, PIN_HIGH);
            vTaskDelay(pdMS_TO_TICKS(DELAY_HEATING_MS));
        }

        /*signal that pre-heating is over*/
        xEventGroupSetBits(heat_ctrl_evt_grp, heat_interval_flag);
          
    }

    /*should never reach here*/
    vTaskDelete(NULL);
}

void pm_heater_off(){
    ESP_LOGI(TAG, "heater power off");
    gpio_set_level(PTC_SW, PIN_LOW);
}