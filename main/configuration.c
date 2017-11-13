/*
 * configuration.c
 *
 *  Created on: 9 Nov 2017
 *      Author: Moukarram Kabbash
 */
#include "configuration.h"
#include "cJSON.h"
#include "stdbool.h"
#include "string.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "automation_logic.h"

#define MONTHS 12


static char *KEY = "config";

char *serialize_status(void) {

	sensor_actor_status_t status =get_last_status();

	cJSON *root = cJSON_CreateObject();
	if(status.sensors != NULL){
		cJSON *a = cJSON_CreateArray();
		cJSON_AddItemToArray(a, cJSON_CreateNumber(status.sensors->temp_reading_1));
		cJSON_AddItemToArray(a, cJSON_CreateNumber(status.sensors->temp_reading_2));
		cJSON_AddItemToArray(a, cJSON_CreateNumber(status.sensors->temp_reading_3));
		cJSON_AddItemToArray(a, cJSON_CreateNumber(status.sensors->temp_reading_4));
		cJSON_AddItemToObject(root, "temp_sensors", a);
		cJSON_AddNumberToObject(root, "brightness", status.sensors->brightness);
		cJSON_AddNumberToObject(root, "sensors_timestamp", status.sensors->timestamp);
	}

	if(status.actors != NULL){

		cJSON *a = cJSON_CreateArray();
		cJSON_AddItemToArray(a, cJSON_CreateNumber(status.actors->relay_state_1));
		cJSON_AddItemToArray(a, cJSON_CreateNumber(status.actors->relay_state_2));
		cJSON_AddItemToArray(a, cJSON_CreateNumber(status.actors->relay_state_3));
		cJSON_AddItemToArray(a, cJSON_CreateNumber(status.actors->relay_state_4));
		cJSON_AddItemToObject(root, "relays", a);
	}
	char* json_unformatted;
	json_unformatted = cJSON_PrintUnformatted(root);
	cJSON_Delete(root);
	return json_unformatted;
}

month_config_t *get_configuration(void) {
	static month_config_t configs[MONTHS] = { { 0, 0, 0, 6 }, //jan
			{ 0, 0, 0, 6 }, //feb
			{ 1, 10, 14, 6 }, //March
			{ 1, 9, 15, 6 }, //april
			{ 1, 9, 16, 6 }, //may
			{ 1, 8, 17, 6 }, //june
			{ 1, 8, 18, 6 }, //july
			{ 1, 8, 18, 6 }, //August
			{ 1, 9, 17, 6 }, //September
			{ 1, 9, 15, 6 }, //October
			{ 1, 10, 13, 6 }, //November
			{ 0, 0, 0, 6 },	 //December
			};
	return configs;
}

char *serialize_configuration(void) {
	cJSON *a = cJSON_CreateArray();
	month_config_t *configs = get_configuration();
	for (int m = 0; a && m < MONTHS; m++) {
		cJSON *line = cJSON_CreateObject();
		cJSON_AddNumberToObject(line, "active", configs[m].active);
		cJSON_AddNumberToObject(line, "start", configs[m].start);
		cJSON_AddNumberToObject(line, "end", configs[m].end);
		cJSON_AddNumberToObject(line, "temp_threshold",
				configs[m].temp_threshold);
		cJSON_AddItemToArray(a, line);
	}
	char* json_unformatted;
	json_unformatted = cJSON_PrintUnformatted(a);
	cJSON_Delete(a);
	return json_unformatted;
}

esp_err_t persist_config() {
	char * buffer = serialize_configuration();
	size_t size = strlen(buffer);

	nvs_handle h;
	esp_err_t ret;

	if (nvs_open(KEY, NVS_READWRITE, &h) != ESP_OK)
		return ESP_FAIL;

	if (!size) {
		ret = nvs_erase_all(h);
		nvs_close(h);
		return ret;
	}

	ret = nvs_set_blob(h, KEY, buffer, size);
	free(buffer);
	nvs_close(h);
	return ret;
}
esp_err_t update_configuration_from_json(const char * buffer, const size_t len) {

	month_config_t *configs = get_configuration();
	cJSON * root = cJSON_Parse(buffer);

	printf("Root of type %d", root->type);
	//TODO check the data for actual length
	for (int mon = 0; mon < MONTHS; mon++) {
		cJSON * line = cJSON_GetArrayItem(root, mon);
		if (line) {
			configs[mon].active = cJSON_GetObjectItem(line, "active")->valueint;
			configs[mon].start = cJSON_GetObjectItem(line, "start")->valueint;
			configs[mon].end = cJSON_GetObjectItem(line, "end")->valueint;
			configs[mon].temp_threshold = cJSON_GetObjectItem(line,
					"temp_threshold")->valueint;
			printf("New threshold: %d", configs[mon].temp_threshold);
		}

	}
	cJSON_Delete(root);
	return ESP_OK;
}
esp_err_t init_configuration() {

	nvs_handle h;
	size_t len = 0;
	esp_err_t ret;

	if ((ret = nvs_open(KEY, NVS_READONLY, &h)) != ESP_OK)
		return ret;

	ret = nvs_get_blob(h, KEY, (void*)NULL, &len);
	if ((ret != ESP_OK) || !len) {
		ESP_LOGD("CONFIG", "First nvs_get_blob returned %x", ret);
		nvs_close(h);
		return ret;
	}
	char* buffer = (char*) malloc(len + 1);
	if (!buffer)
		return nvs_close(h), false;
	ret = nvs_get_blob(h, KEY, buffer, &len);
	if (ret != ESP_OK) {
		ESP_LOGD("CONFIG", "First nvs_get_blob returned %x", ret);
		free(buffer);
		nvs_close(h);
		return ret;
	}
	buffer[len] = 0x00;
	ret = update_configuration_from_json(buffer, len);
	free(buffer);
	nvs_close(h);

	return ESP_OK;
}
