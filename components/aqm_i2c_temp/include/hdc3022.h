#ifndef HDC3022_H
#define HDC3022_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <i2c_common.h>


/*public types*/
typedef enum hdc3022_err
{
    HDC3022_NO_ERR = 0,
    HDC3022_NO_DEVICE = 1
} hdc3022_err_t;

/*public functions*/
hdc3022_err_t hdc3022_configure_autonomous();
hdc3022_err_t hdc3022_get_temp_humid(double *temperature, double *humidity);


#endif