/*
 * wifi_manager.c
 *
 *  Created on: 20 Oct 2017
 *      Author: Moukarram Kabbash
 */
#include <string.h>
#include "wifi_manager.h"
#include "esp_event_loop.h"
#include "esp_wifi.h"

#include "esp_log.h"

#define CONFIG_AP_SSID "PetAutomation"
#define CONFIG_AP_PASSWORD "PetAutomation"

static const char *TAG = "WiFi";


#define WIFI_SSID CONFIG_WIFI_SSID
#define WIFI_PASS CONFIG_WIFI_PASSWORD


static esp_err_t event_handler(void *ctx, system_event_t *event);


void initialise_wifi(void) {
	tcpip_adapter_init();
	event_group = xEventGroupCreate();
	ESP_ERROR_CHECK(esp_event_loop_init(event_handler, NULL));
	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT()	;
	ESP_ERROR_CHECK(esp_wifi_init(&cfg));
	ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
	wifi_config_t wifi_config = { .sta = { .ssid = WIFI_SSID, .password =
	WIFI_PASS, }, };

	ESP_LOGI(TAG, "Setting WiFi configuration SSID %s...", wifi_config.sta.ssid);
	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
	ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
	ESP_ERROR_CHECK(esp_wifi_start());
}

esp_err_t set_wifi_sta_and_start(char *ssid, char *password){
	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));

	wifi_config_t sta_config;
	ESP_ERROR_CHECK(esp_wifi_get_config(WIFI_IF_STA, &sta_config));
	ESP_LOGI(TAG, "Old SSID: %s, Password: %s", sta_config.sta.ssid, sta_config.sta.password);

	memset(sta_config.sta.ssid, 0, sizeof sta_config.sta.ssid);
	strncpy((char *)sta_config.sta.ssid, ssid, strlen(ssid));
	memset(sta_config.sta.password, 0, sizeof sta_config.sta.password);
	strncpy((char *)sta_config.sta.password, password, strlen(password));
	ESP_LOGI(TAG, "New SSID: %s, Password: %s", sta_config.sta.ssid, sta_config.sta.password);

	ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &sta_config));
	return esp_wifi_connect();
}
// AP event handler
static esp_err_t event_handler(void *ctx, system_event_t *event) {
	switch (event->event_id) {

	case SYSTEM_EVENT_AP_START:
		printf("Access point started\n");
		break;

	case SYSTEM_EVENT_AP_STACONNECTED:
		ESP_LOGI(TAG, "SYSTEM_EVENT_AP_STACONNECTED")		;
		xEventGroupSetBits(event_group, AP_CONNECTED_BIT);
		break;

	case SYSTEM_EVENT_AP_STADISCONNECTED:
		ESP_LOGI(TAG, "SYSTEM_EVENT_AP_STADISCONNECTED");
		xEventGroupClearBits(event_group, AP_CONNECTED_BIT);
		break;
	case SYSTEM_EVENT_STA_GOT_IP:
		xEventGroupSetBits(event_group, STA_CONNECTED_BIT);
		printf("got ip\n");
		printf("ip: " IPSTR "\n", IP2STR(&event->event_info.got_ip.ip_info.ip));
		printf("netmask: " IPSTR "\n",
				IP2STR(&event->event_info.got_ip.ip_info.netmask));
		printf("gw: " IPSTR "\n", IP2STR(&event->event_info.got_ip.ip_info.gw));
		printf("\n");
		break;
	case SYSTEM_EVENT_STA_START:
		ESP_LOGI(TAG, "SYSTEM_EVENT_STA_START");
		//esp_wifi_connect();
		break;
	case SYSTEM_EVENT_STA_DISCONNECTED:
		/* This is a workaround as ESP32 WiFi libs don't currently
		 auto-reassociate. */
		ESP_LOGI(TAG, "SYSTEM_EVENT_STA_DISCONNECTED");
		//esp_wifi_connect();
		xEventGroupClearBits(event_group, STA_CONNECTED_BIT);
		break;

	default:
		break;
	}

	return ESP_OK;
}

esp_err_t wifi_start_soft_ap() {
	event_group = xEventGroupCreate();

	tcpip_adapter_init();

	tcpip_adapter_ip_info_t ipInfo;
	IP4_ADDR(&ipInfo.ip, 192, 168, 1, 99);
	IP4_ADDR(&ipInfo.gw, 192, 168, 1, 1);
	IP4_ADDR(&ipInfo.netmask, 255, 255, 255, 0);
	ESP_ERROR_CHECK(tcpip_adapter_dhcps_stop(TCPIP_ADAPTER_IF_AP));
	ESP_ERROR_CHECK(tcpip_adapter_set_ip_info(TCPIP_ADAPTER_IF_AP, &ipInfo));
	ESP_ERROR_CHECK(tcpip_adapter_dhcps_start(TCPIP_ADAPTER_IF_AP));

	ESP_ERROR_CHECK(esp_event_loop_init(event_handler, NULL));
	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
	ESP_ERROR_CHECK(esp_wifi_init(&cfg));
	ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
	wifi_config_t apConfig = { .ap = { .ssid = CONFIG_AP_SSID, .ssid_len = 0,
			.password = CONFIG_AP_PASSWORD, .channel = 0, .authmode =
					WIFI_AUTH_WPA2_PSK, .max_connection = 4, .beacon_interval =
					100 } };
	ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &apConfig));
	ESP_ERROR_CHECK(esp_wifi_start());

	ESP_LOGI(TAG, "Starting access point, SSID=%s\n", CONFIG_AP_SSID);

	return ESP_OK;
}

void wifi_stop_soft_ap() {
	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
}
