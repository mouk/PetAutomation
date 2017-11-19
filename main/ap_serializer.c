/*
 * ap_serializer.c
 *
 *  Created on: 19 Nov 2017
 *      Author: Moukarram Kabbash
 */

#include "wifi_manager.h"
#include "cJSON.h"
#include "esp_log.h"
#include <string.h>
const static char *TAG = "SERLIZER";

typedef struct {
	uint8_t ssid[33];
	int8_t strength;
} ap_data_t;

/*
 * Function:  ap_get_rssi_rating
 * --------------------
 * Get a rating for the wifi signal strength:
 *    accoring to https://www.metageek.com/training/resources/understanding-rssi.html
 *
 *  rssi: the strength of the signal in dBm
 *
 *  returns: 	1 Amazing
 *  				2 Very Good
 *  				3 Ok
 *  				4 Not Good
 *  				5 Unusable
 */
static int8_t ap_get_rssi_rating(int8_t rssi){
	if(rssi > -67)
		return 1;
	if(rssi > -70)
		return 2;
	if(rssi > -80)
		return 3;
	if(rssi > -90)
		return 4;
	return 5;
}
static ap_data_t *ap_simplify_aps(uint16_t *ap_num) {
	uint16_t all_ap_num;
	wifi_ap_record_t * aps = get_ap_records(&all_ap_num);

	ap_data_t *ap_data = malloc(sizeof(ap_data_t) * all_ap_num);

	uint16_t ap_found = 0;
	for (int i = 0; i < all_ap_num; i++) {
		bool better_found = false;

		for (int other = 0; other < all_ap_num; other++) {
			if (strcmp((char *) aps[i].ssid, (char *) aps[other].ssid) == 0) {
				if (aps[i].rssi < aps[other].rssi){
					better_found = true;
					continue;
				}
			}
		}

		if(better_found)
			continue;


		ap_data_t ap = { 0 };
		ap.strength = ap_get_rssi_rating(aps[i].rssi);
		memcpy(ap.ssid, aps[i].ssid, 33);
		ap_data[ap_found++] = ap;
	}

	ESP_LOGI(TAG, "%u ssids turned to %u", all_ap_num, ap_found);
	*ap_num = ap_found;

	return ap_data;
}
char *wifi_serialize_scanned_ap() {
	uint16_t ap_num;
	ap_data_t * aps = ap_simplify_aps(&ap_num);

	cJSON *root = cJSON_CreateArray();
	for (int i = 0; i < ap_num; i++) {
		cJSON *ap = cJSON_CreateObject();
		cJSON_AddStringToObject(ap, "ssid", (char * )aps[i].ssid);
		cJSON_AddNumberToObject(ap, "strength", aps[i].strength);
		cJSON_AddItemToArray(root, ap);
	}
	char* json_unformatted;
	json_unformatted = cJSON_PrintUnformatted(root);
	cJSON_Delete(root);
	return json_unformatted;
}
