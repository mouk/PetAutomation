/*
 * sensors.h
 *
 *  Created on: 28 Oct 2017
 *      Author: Moukarram Kabbash
 */

#ifndef MAIN_INCLUDE_SENSORS_H_
#define MAIN_INCLUDE_SENSORS_H_
#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include "esp_err.h"

typedef struct{
	time_t timestamp;
	int8_t temp_reading_1;
	int8_t temp_reading_2;
	int8_t temp_reading_3;
	int8_t temp_reading_4;
	uint8_t temp_readings_count;
	int8_t brightness;
}sensors_reading_t;

void init_sensors(void);
esp_err_t get_seasors_reading(sensors_reading_t *result);
void printout_sensors_reading(sensors_reading_t *result);


#endif /* MAIN_INCLUDE_SENSORS_H_ */
