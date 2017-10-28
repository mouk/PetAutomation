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
#include "freertos/timers.h"

#define CONFIG_AP_SSID "PetAutomation"
#define CONFIG_AP_PASSWORD "PetAutomation"

static const char *TAG = "WiFi";

#define WIFI_SSID CONFIG_WIFI_SSID
#define WIFI_PASS CONFIG_WIFI_PASSWORD

static esp_err_t event_handler(void *ctx, system_event_t *event);
static esp_err_t last_connection_err = ESP_OK;
static void wifi_start_soft_ap();
static void WIFI_ERROR_CHECK(esp_err_t ret);
static void create_timer_to_stop_ap(void);

void initialise_wifi(void) {
	tcpip_adapter_init();
	event_group = xEventGroupCreate();
	ESP_ERROR_CHECK(esp_event_loop_init(event_handler, NULL));
	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
	ESP_ERROR_CHECK(esp_wifi_init(&cfg));
	ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_FLASH));
	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
	ESP_ERROR_CHECK(esp_wifi_start());

	EventBits_t wifi_bits = xEventGroupWaitBits(event_group,
	STA_CONNECTED_BIT, //| STA_CONNECTION_ERR_BIT,
			false, false, pdMS_TO_TICKS(20 * 1000));

	if ((wifi_bits & STA_CONNECTED_BIT) == 0) {
		ESP_LOGI(TAG,
				"Connection to AP couldn't be established. Switching to softAP mode.")
		wifi_start_soft_ap();
	} else {
		ESP_LOGI(TAG, "Connection to AP established successfully")
	}
}
void set_wifi_sta_and_start(char s[32], char p[64]) {

	ESP_ERROR_CHECK(esp_wifi_stop());
	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
	wifi_config_t sta_config;

	memset(sta_config.sta.ssid, 0, sizeof sta_config.sta.ssid);
	memset(sta_config.sta.password, 0, sizeof sta_config.sta.password);
	strcpy((char *) sta_config.sta.ssid, s);
	strcpy((char *) sta_config.sta.password, p);

	ESP_LOGI(TAG, "New SSID: (%s), Password: (%s)", sta_config.sta.ssid,
			sta_config.sta.password);
	ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &sta_config));
	ESP_ERROR_CHECK(esp_wifi_start());
}

// AP event handler
static esp_err_t event_handler(void *ctx, system_event_t *event) {
	switch (event->event_id) {

	case SYSTEM_EVENT_AP_START:
		printf("Access point started\n");
		break;

	case SYSTEM_EVENT_AP_STACONNECTED:
		ESP_LOGI(TAG, "SYSTEM_EVENT_AP_STACONNECTED")
		;
		xEventGroupSetBits(event_group, AP_CONNECTED_BIT);
		break;

	case SYSTEM_EVENT_AP_STADISCONNECTED:
		ESP_LOGI(TAG, "SYSTEM_EVENT_AP_STADISCONNECTED")
		;
		xEventGroupClearBits(event_group, AP_CONNECTED_BIT);
		break;
	case SYSTEM_EVENT_STA_GOT_IP:
		xEventGroupSetBits(event_group, STA_CONNECTED_BIT);
		//xEventGroupClearBits(event_group, STA_CONNECTION_ERR_BIT);
		printf("got ip\n");
		printf("ip: " IPSTR "\n", IP2STR(&event->event_info.got_ip.ip_info.ip));
		printf("netmask: " IPSTR "\n",
				IP2STR(&event->event_info.got_ip.ip_info.netmask));
		printf("gw: " IPSTR "\n", IP2STR(&event->event_info.got_ip.ip_info.gw));
		printf("\n");
		break;
	case SYSTEM_EVENT_STA_START:
		ESP_LOGI(TAG, "SYSTEM_EVENT_STA_START")
		;
		last_connection_err = esp_wifi_connect();
		if (last_connection_err != ESP_OK) {
			WIFI_ERROR_CHECK(last_connection_err);
			ESP_LOGI(TAG, "esp_wifi_connect failed with error: %d",
					last_connection_err);
			//xEventGroupSetBits(event_group, STA_CONNECTION_ERR_BIT);
		}
		break;
	case SYSTEM_EVENT_STA_DISCONNECTED:
		/* This is a workaround as ESP32 WiFi libs don't currently
		 auto-reassociate. */
		ESP_LOGI(TAG, "SYSTEM_EVENT_STA_DISCONNECTED: SSID: %s reason: %u",
				(char * ) &event->event_info.disconnected.ssid,
				(unsigned char )&event->event_info.disconnected.reason)
		;
		WIFI_ERROR_CHECK(esp_wifi_connect());
		xEventGroupClearBits(event_group, STA_CONNECTED_BIT);
		break;

	default:
		break;
	}

	return ESP_OK;
}

static void wifi_start_soft_ap() {
	ESP_ERROR_CHECK(esp_wifi_stop());

	tcpip_adapter_ip_info_t ipInfo;
	IP4_ADDR(&ipInfo.ip, 192, 168, 1, 99);
	IP4_ADDR(&ipInfo.gw, 192, 168, 1, 1);
	IP4_ADDR(&ipInfo.netmask, 255, 255, 255, 0);
	ESP_ERROR_CHECK(tcpip_adapter_dhcps_stop(TCPIP_ADAPTER_IF_AP));
	ESP_ERROR_CHECK(tcpip_adapter_set_ip_info(TCPIP_ADAPTER_IF_AP, &ipInfo));
	ESP_ERROR_CHECK(tcpip_adapter_dhcps_start(TCPIP_ADAPTER_IF_AP));

	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
	wifi_config_t apConfig = { .ap = { .ssid = CONFIG_AP_SSID, .ssid_len = 0,
			.password = CONFIG_AP_PASSWORD, .channel = 0, .authmode =
					WIFI_AUTH_WPA2_PSK, .max_connection = 4, .beacon_interval =
					100 } };
	ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &apConfig));
	ESP_ERROR_CHECK(esp_wifi_start());

	ESP_LOGI(TAG, "Starting access point, SSID=%s\n", CONFIG_AP_SSID);
	create_timer_to_stop_ap();
}

void wifi_stop_soft_ap() {
	wifi_mode_t wifi_mode;
	ESP_ERROR_CHECK(esp_wifi_get_mode(&wifi_mode));
	wifi_mode &= (!WIFI_MODE_AP);
	ESP_ERROR_CHECK(esp_wifi_set_mode(wifi_mode));
}

void wifi_stop_sta() {
	wifi_mode_t wifi_mode;
	ESP_ERROR_CHECK(esp_wifi_get_mode(&wifi_mode));
	wifi_mode &= (!WIFI_MODE_STA);
	ESP_ERROR_CHECK(esp_wifi_set_mode(wifi_mode));
}
static void v_stop_ap_callback(TimerHandle_t xTimer) {
	ESP_LOGI(TAG, "Time expired");
	configASSERT(xTimer);
	xTimerStop(xTimer, 0);
	wifi_stop_soft_ap();
}
static TimerHandle_t ap_timer;
static void create_timer_to_stop_ap(void) {
	ESP_LOGI(TAG, "Creating timer to stop softAP");
	ap_timer = xTimerCreate("StopAPTimer", pdMS_TO_TICKS(1000 * SOFTAP_TIMEOUT_IN_S),
	pdFALSE, (void *) 0, v_stop_ap_callback);
	if (ap_timer == NULL) {
		ESP_LOGI(TAG, "Couldn't create timer for stopping ap");
	} else {
		if ( xTimerStart( ap_timer, 0 ) != pdPASS) {
			ESP_LOGI(TAG, "Couldn't activate timer for stopping ap");
		}
	}
}

static char *errorNumberToString(esp_err_t error) {
	switch (error) {
	case ESP_OK:
		return "OK";
	case ESP_FAIL:
		return "Fail";
	case ESP_ERR_NO_MEM:
		return "No memory";
	case ESP_ERR_INVALID_ARG:
		return "Invalid argument";
	case ESP_ERR_INVALID_SIZE:
		return "Invalid size";
	case ESP_ERR_INVALID_STATE:
		return "Invalid state";
	case ESP_ERR_NOT_FOUND:
		return "Not found";
	case ESP_ERR_NOT_SUPPORTED:
		return "Not supported";
	case ESP_ERR_TIMEOUT:
		return "Timeout";
	case ESP_ERR_WIFI_NOT_INIT:
		return "ESP_ERR_WIFI_NOT_INIT";
	case ESP_ERR_WIFI_NOT_STARTED:
		return "ESP_ERR_WIFI_NOT_START";
	case ESP_ERR_WIFI_IF:
		return "ESP_ERR_WIFI_IF";
	case ESP_ERR_WIFI_MODE:
		return "ESP_ERR_WIFI_MODE";
	case ESP_ERR_WIFI_STATE:
		return "ESP_ERR_WIFI_STATE";
	case ESP_ERR_WIFI_CONN:
		return "ESP_ERR_WIFI_CONN";
	case ESP_ERR_WIFI_NVS:
		return "ESP_ERR_WIFI_NVS";
	case ESP_ERR_WIFI_MAC:
		return "ESP_ERR_WIFI_MAC";
	case ESP_ERR_WIFI_SSID:
		return "ESP_ERR_WIFI_SSID";
	case ESP_ERR_WIFI_PASSWORD:
		return "ESP_ERR_WIFI_PASSWORD";
	case ESP_ERR_WIFI_TIMEOUT:
		return "ESP_ERR_WIFI_TIMEOUT";
	case ESP_ERR_WIFI_WAKE_FAIL:
		return "ESP_ERR_WIFI_WAKE_FAIL";
	}
	return "Unknown ESP_ERR error";
}
static void WIFI_ERROR_CHECK(esp_err_t ret) {
	ESP_LOGI(TAG, "WIFI error %s", errorNumberToString(ret));
}
