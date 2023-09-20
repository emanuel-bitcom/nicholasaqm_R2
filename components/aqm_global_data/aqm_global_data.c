#include "aqm_global_data.h"

/*
definitions of global variables
*/
short I2C_INIT_OK=0;
short TOXIC_SERVICE_ACTIVE=0;
short TEMP_SERVICE_ACTIVE=0;

/*Global TCP/IP related flags*/
_Bool is_netif_init=0;
_Bool is_eventloop_init=0;
const int mqtt_flag=BIT7;

/**
 * Global Mutexes and other semaphores
*/
SemaphoreHandle_t sensors_data_m;
SemaphoreHandle_t i2c_bus_m;
EventGroupHandle_t sensors_evt_grp;
EventGroupHandle_t data_sent_evt_grp;
EventGroupHandle_t comm_available_evt_grp;
EventGroupHandle_t heat_ctrl_evt_grp;

void init_global_mutexes(){
    //create mutexes
    i2c_bus_m=xSemaphoreCreateMutex();
    sensors_data_m=xSemaphoreCreateMutex();
  
    sensors_evt_grp=xEventGroupCreate();
    data_sent_evt_grp=xEventGroupCreate();
    comm_available_evt_grp=xEventGroupCreate();
    heat_ctrl_evt_grp=xEventGroupCreate();

}

void wait_for_mutex(SemaphoreHandle_t *mutex, uint wait_delay){
  while(xSemaphoreTake(*mutex, 0)!=pdTRUE){
    vTaskDelay(wait_delay/portTICK_PERIOD_MS);
  }
}

void release_mutex(SemaphoreHandle_t *mutex){
  xSemaphoreGive(*mutex);
}