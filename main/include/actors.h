/*
 * actors.h
 *
 *  Created on: 29 Oct 2017
 *      Author: Moukarram Kabbash
 */

#ifndef MAIN_INCLUDE_ACTORS_H_
#define MAIN_INCLUDE_ACTORS_H_
#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"
#include "esp_log.h"

typedef struct{
	bool relay_state_1;
	bool relay_state_2;
	bool relay_state_3;
	bool relay_state_4;
	uint8_t relays_count;
}actors_state_t;

void init_actors(void);
esp_err_t apply_actors_state(actors_state_t *state);
void printout_actors_state(actors_state_t *actors);

#endif /* MAIN_INCLUDE_ACTORS_H_ */
