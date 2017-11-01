/*
 * automation_logic.c
 *
 *  Created on: 29 Oct 2017
 *      Author: Moukarram Kabbash
 */
#include "automation_logic.h"
#include <time.h>
#include "esp_log.h"

struct {
	int month;
	int start;
	int end;
	int min_temperature;
} month_config_t;
esp_err_t process_sensors_reading(sensors_reading_t *sensors, actors_state_t *actors) {

	struct tm timeinfo = { 0 };
	localtime_r(&sensors->timestamp, &timeinfo);
	if (timeinfo.tm_year < (2016 - 1900)) {
		return PA_TIME_NOT_SET;
	}
	int hour = timeinfo.tm_hour;
	int mon = timeinfo.tm_mon;
	//TODO the configuration should be moved to some kind of storage
	const int configs[][3] = { { 0, 0, 6 }, //jan
			{ 0, 0, 6 }, //feb
			{ 10, 14, 6 }, //March
			{ 9, 15, 6 }, //april
			{ 9, 16, 6 },//may
			{ 8, 17, 6 }, //june
			{ 8, 18, 6 }, //july
			{ 8, 18, 6 }, //August
			{ 9, 17, 6 }, //September
			{ 9, 15, 6 }, //October
			{ 10, 13, 6 },//November
			{ 0, 0, 6 },	 //December
			};
	bool relay1_eligable = hour >= configs[mon][0] && hour < configs[mon][1];
	actors->relay_state_1 = relay1_eligable && sensors->temp_reading_1 < 30
			&& sensors->brightness < 60;
	actors->relay_state_2 = !actors->relay_state_1
			&& sensors->temp_reading_2 <= configs[mon][3];

	return ESP_OK;
}
