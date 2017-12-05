#ifndef __TEMP_AND_RH_H
#define __TEMP_AND_RH_H


#include "stm32f4xx_hal.h"
#include "stm32f4xx_it.h"

#define TEMP_HUMI_ADDR  0x40
#define TEMP_HUMI_READ	0x81
#define TEMP_HUMI_WRITE 0x80
#define CMD_TEMP		0xE3
#define CMD_HUMI		0xE5
#define SOFT_RESET		0xFE

void init_temp_humi_sensor(I2C_HandleTypeDef *hi2c);
double get_temp_data(I2C_HandleTypeDef *hi2c);
double get_humi_data(I2C_HandleTypeDef *hi2c);
void us_delay1_int_count(volatile unsigned int nTime);

#endif
