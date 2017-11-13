/*
 * automation_logic.c
 *
 *  Created on: 29 Oct 2017
 *      Author: Moukarram Kabbash
 */
#include "automation_logic.h"
#include <time.h>
#include "esp_log.h"
#include "configuration.h"

static sensors_reading_t *last_sensors;
static actors_state_t *last_actors;

esp_err_t process_sensors_reading(sensors_reading_t *sensors, actors_state_t *actors) {

	struct tm timeinfo = { 0 };
	localtime_r(&sensors->timestamp, &timeinfo);
	if (timeinfo.tm_year < (2016 - 1900)) {
		return PA_TIME_NOT_SET;
	}
	int hour = timeinfo.tm_hour;
	int mon = timeinfo.tm_mon;
	//TODO the configuration should be moved to some kind of storage
	month_config_t *configs = get_configuration();
	bool relay1_eligable = hour >= configs[mon].start && hour < configs[mon].end;
	actors->relay_state_1 = relay1_eligable && sensors->temp_reading_1 < 30
			&& sensors->brightness < 60;
	actors->relay_state_2 = !actors->relay_state_1
			&& sensors->temp_reading_2 <= configs[mon].temp_threshold;

	last_sensors = sensors;
	last_actors = actors;

	return ESP_OK;
}

sensor_actor_status_t get_last_status(){
	sensor_actor_status_t ret = {last_sensors, last_actors};
	return ret;
}
