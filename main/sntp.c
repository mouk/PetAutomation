/* LwIP SNTP example

 This example code is in the Public Domain (or CC0 licensed, at your option.)

 Unless required by applicable law or agreed to in writing, this
 software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
 CONDITIONS OF ANY KIND, either express or implied.
 */
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_attr.h"
#include "esp_sleep.h"
#include "lwip/err.h"
#include "apps/sntp/sntp.h"
#include "constants.h"

static const char *TAG = "SNTP";

static void obtain_time(EventGroupHandle_t wifi_event_group);
static void initialize_sntp(void);
uint8_t has_internet_connection(EventGroupHandle_t wifi_event_group) ;

void get_time(EventGroupHandle_t wifi_event_group) {
	obtain_time(wifi_event_group);

	setenv("TZ", "CET-1CEST,M3.5.0,M10.5.0", 1);

	char strftime_buf[64];
	time_t now;
	struct tm timeinfo;
	time(&now);
	localtime_r(&now, &timeinfo);
	strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
	ESP_LOGI(TAG, "The current date/time in central europe is: %s", strftime_buf);

	ESP_LOGI(TAG, "Time was set. Deleting task");

	vTaskDelete(NULL);
}

static void obtain_time(EventGroupHandle_t wifi_event_group) {
	while (!has_internet_connection(wifi_event_group)) {
		vTaskDelay(200 / portTICK_PERIOD_MS);
	}

	initialize_sntp();

	// wait for time to be set
	time_t now = 0;
	struct tm timeinfo = { 0 };
	int retry = 0;
	const int retry_count = 10;
	while (timeinfo.tm_year < (2016 - 1900) && ++retry < retry_count) {
		ESP_LOGI(TAG, "Cannot set time. No Internet connection.");
		ESP_LOGI(TAG, "Waiting for system time to be set... (%d/%d)", retry,
				retry_count);
		vTaskDelay(2000 / portTICK_PERIOD_MS);
		time(&now);
		localtime_r(&now, &timeinfo);
	}
}

uint8_t has_internet_connection(EventGroupHandle_t wifi_event_group) {
	EventBits_t set = xEventGroupWaitBits(wifi_event_group, STA_CONNECTED_BIT,
	false, true, portMAX_DELAY);
	return ((STA_CONNECTED_BIT & set) != 0);
}

static void initialize_sntp(void) {
	ESP_LOGI(TAG, "Initializing SNTP...");
	sntp_setoperatingmode(SNTP_OPMODE_POLL);
	sntp_setservername(0, "pool.ntp.org");
	sntp_init();
}

