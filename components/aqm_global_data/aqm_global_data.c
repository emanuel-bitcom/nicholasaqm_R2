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

//****Functions to write and get current data
toxic_data current_toxic_data={
  .SEN1_A=0,
  .SEN1_W=0,
  .SEN2_A=0,
  .SEN2_W=0,
  .SEN3_W=0,
  .SEN3_A=0,
  .SEN4_A=0,
  .SEN4_W=0
};

void write_toxic_data(toxic_data in_data){
  current_toxic_data=in_data;
}
void get_current_toxic_data(toxic_data* out_data){
 if(out_data){
  *out_data=current_toxic_data;
 }
}

temp_data current_temp_data={
  .humid_env=0,
  .humid_sta=0,
  .temp_env=0,
  .temp_sta=0
};

void write_temp_data(temp_data in_data){
  current_temp_data=in_data;

}
void get_current_temp_data(temp_data* out_data){
  if(out_data){
    *out_data=current_temp_data;
  }
}

opc_data current_opc_data={
  .pmA = 0,
  .pmB = 0,
  .pmC = 0
};

void write_opc_data(opc_data in_data){
  current_opc_data = in_data;
}


void get_current_opc_data(opc_data* out_data){
  if(out_data){
    *out_data=current_opc_data;
  }
}