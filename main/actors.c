/*
 * actors.c
 *
 *  Created on: 29 Oct 2017
 *      Author: Moukarram Kabbash
 */


#include "actors.h"


static const char *TAG = "ACTORS";

esp_err_t apply_actors_state(actors_state_t *state){
	return ESP_OK;
}


void printout_actors_state(actors_state_t *actors) {
	ESP_LOGI(TAG,
			"actors_state relay1: %d, relay2: %d",
			actors->relay_state_1,
			actors->relay_state_2);
}
