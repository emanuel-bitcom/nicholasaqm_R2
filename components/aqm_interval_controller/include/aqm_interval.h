#ifndef AQM_INTERVAL_H
#define AQM_INTERVAL_H

#include "aqm_global_data.h"
#include "aqm_nvs_high.h"

/*public functions*/
void init_interval_control();

/*private functions*/
static void interval_fun(void *params);


#endif