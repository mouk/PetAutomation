/*
 * automation_logic.h
 *
 *  Created on: 29 Oct 2017
 *      Author: Moukarram Kabbash
 */

#ifndef MAIN_INCLUDE_AUTOMATION_LOGIC_H_
#define MAIN_INCLUDE_AUTOMATION_LOGIC_H_


#include "sensors.h"
#include "actors.h"
#include "esp_err.h"

#define PA_TIME_NOT_SET        50

esp_err_t process_sensors_reading(sensors_reading_t *sensors, actors_state_t *actors) ;


#endif /* MAIN_INCLUDE_AUTOMATION_LOGIC_H_ */
