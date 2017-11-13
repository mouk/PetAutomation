/*
 * configuration.h
 *
 *  Created on: 9 Nov 2017
 *      Author: Moukarram Kabbash
 */

#ifndef MAIN_INCLUDE_CONFIGURATION_H_
#define MAIN_INCLUDE_CONFIGURATION_H_

#include "stdint.h"
#include "esp_err.h"

typedef struct {
	uint8_t active;
	uint8_t start;
	uint8_t end;
	int8_t temp_threshold;
}month_config_t;
esp_err_t init_configuration() ;
month_config_t *get_configuration(void);
char *serialize_configuration(void);
esp_err_t update_configuration_from_json(const char * buffer, const size_t len);
esp_err_t persist_config();
char *serialize_status(void);

#endif /* MAIN_INCLUDE_CONFIGURATION_H_ */
