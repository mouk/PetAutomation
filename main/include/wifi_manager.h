/*
 * wifi_manager.h
 *
 *  Created on: 20 Oct 2017
 *      Author: Moukarram Kabbash
 */

#ifndef MAIN_INCLUDE_WIFI_MANAGER_H_
#define MAIN_INCLUDE_WIFI_MANAGER_H_

#define MAX_AP_NUM 25

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "constants.h"
#include "esp_wifi_types.h"


void initialise_wifi(void);
void set_wifi_sta_and_start(char *ssid, char *password);
void wifi_stop_soft_ap_async();
void wifi_stop_sta();
wifi_ap_record_t *get_ap_records(uint16_t *ap_num);

EventGroupHandle_t event_group;
#endif /* MAIN_INCLUDE_WIFI_MANAGER
_H_ */
