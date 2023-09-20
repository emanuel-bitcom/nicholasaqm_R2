#ifndef AQM_GLOBAL_DATA_H
#define AQM_GLOBAL_DATA_H

/**
 * software version
*/
//#define VERSION_TEST
#define VERSION_PRODUCTION
 

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "esp_system.h"
#include <esp_log.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/event_groups.h"

/*Global Variables*/
#define N_VALUES 20
#define N_TIME 10 //default reading time (in s) used for computing delay between measurements
#define N_INTERVAL 5 //default interval (in min) between readings
#define WIFI_TIMEOUT 3 //default wifi timeout (int min) 

#define TOXIC_SENSORS_N 4

/*Global TCP/IP related flags*/
extern _Bool is_netif_init;
extern _Bool is_eventloop_init;
extern const int mqtt_flag;

//flags
extern const int toxic_flag;
extern const int temp_flag;
extern const int opc_flag;
extern const int wifi_interface_done;
extern const int heat_ctrl_flag;
extern const int heat_interval_flag;

/*Global timing flag*/
extern const int interval_flag;

/*
Global I2C flags
*/
extern short I2C_INIT_OK;

/*
Global Toxic flags
*/
extern short TOXIC_SERVICE_ACTIVE;

/**
 * Global Temperature Humidity flags
*/
extern short TEMP_SERVICE_ACTIVE;

/**
 * Global Mutexes and other semaphores
*/
extern SemaphoreHandle_t sensors_data_m;
extern SemaphoreHandle_t i2c_bus_m;
extern EventGroupHandle_t sensors_evt_grp;
extern EventGroupHandle_t data_sent_evt_grp;
extern EventGroupHandle_t comm_available_evt_grp;
extern EventGroupHandle_t heat_ctrl_evt_grp;

/**
 * SPI Bus pins
*/
#define AQM_SCK_PIN 18
#define AQM_MISO_PIN 19
#define AQM_MOSI_PIN 23
#define AQM_SPI_HOST HSPI_HOST

//sensors data structures
//***********************************************************************

typedef struct toxic_data_struct{
	  unsigned long SEN1_W;
    unsigned long SEN2_W;
    unsigned long SEN3_W;
    unsigned long SEN4_W;

    unsigned long SEN1_A;
    unsigned long SEN2_A;
    unsigned long SEN3_A;
    unsigned long SEN4_A;
}toxic_data;

//temp and humid placed together
typedef struct temp_data_struct{
	double temp_env;
  double temp_sta;
  int humid_env;
  int humid_sta;
}temp_data;


typedef struct opc_data_struct{
	  float pmA;
    float pmB;
    float pmC;
}opc_data;

//***********************************************************************

//***********************************************************************
//sensors calibration data structures

typedef struct sensor_cal_struct{
  char type[5];
  uint16_t WE0;
  uint16_t WEe;
  uint16_t AE0;
  uint16_t AEe;
  uint16_t sensit;
  int16_t NO2_sensit;
  float gain;
}sensor_cal;



typedef struct station_cal_struct{
  uint16_t station_SN;

  float pos_latitude;
  float pos_longitude;

  //first position is empty as sensors number begin with 1 
  sensor_cal SEN_cal[TOXIC_SENSORS_N+1];

}station_cal;

//*************************************************************************
//LTE Structs

typedef struct lte_cloud_struct{
  char server[40];
  uint16_t port;
  char path[50];
}lte_cloud;

typedef struct lte_data_struct{
	char lte_apn[30];
  char lte_user[30];
  char lte_pass[30];
}lte_data;

//*************************************************************************
//MQTT struct

typedef struct mqtt_conn_config
{
  char url[50];
  unsigned int port;
  char username[30];
  char password[30];
}mqtt_conn_config_t;


//**************************************************************************
//Interval measurement configuration

typedef struct interval_config{
  int interval;         //in min
  int repetitions;      //no.
  int measuring_time;    //in min
}interval_config_t;

//**************************************************************************
//Lorawan measurement configuration

typedef struct lorawan_config{
  char DevEUI[17];    
  char AppEUI[17];    
  char AppKEY[33];    
}lorawan_config_t;

//**************************************************************************
//ETH mac configuration

typedef struct eth_config{
  char mac[20];
}eth_config_t;

//**************************************************************************
//Wifi configuration 

typedef struct wifi_aqn_config{
  char wifi_ssid[50];
  char wifi_pwd[20];
  char web_uname[50];
  char web_pwd[20];
  int timeout;
}wifi_aqm_config_t;

//**************************************************************************

/**
 * Global configurations
*/
#define  USE_LTE
#define USE_ETH
#undef USE_T_TUBE_AS_T_STA



void wait_for_mutex(SemaphoreHandle_t *mutex, uint wait_delay);
void release_mutex(SemaphoreHandle_t *mutex);
void init_global_mutexes();


/*Global macros functions*/
#define WAIT_I2C() {wait_for_mutex(&i2c_bus_m, 200);}
#define RELEASE_I2C() {release_mutex(&i2c_bus_m);}
#define DELAY_MS(T)  (vTaskDelay(pdMS_TO_TICKS(T)))

#endif
