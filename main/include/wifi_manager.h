/*
 * wifi_manager.h
 *
 *  Created on: 20 Oct 2017
 *      Author: Moukarram Kabbash
 */

#ifndef MAIN_INCLUDE_WIFI_MANAGER_H_
#define MAIN_INCLUDE_WIFI_MANAGER_H_


#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "constants.h"


void initialise_wifi(void);
esp_err_t wifi_start_soft_ap();
esp_err_t set_wifi_sta_and_start(char *ssid, char *password);
void wifi_stop_soft_ap();

EventGroupHandle_t event_group;
#endif /* MAIN_INCLUDE_WIFI_MANAGER
_H_ */
