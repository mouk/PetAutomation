/* Blink Example

 This example code is in the Public Domain (or CC0 licensed, at your option.)

 Unless required by applicable law or agreed to in writing, this
 software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
 CONDITIONS OF ANY KIND, either express or implied.
 */
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "driver/gpio.h"
#include "sdkconfig.h"
#include "sntp.h"
#include "nvs_flash.h"
#include "esp_event_loop.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "constants.h"
#include "ds18b20.h"

#define HUMIDITY_GPIO CONFIG_HUMIDITY_GPIO
#define WIFI_SSID CONFIG_WIFI_SSID
#define WIFI_PASS CONFIG_WIFI_PASSWORD

static void initialise_wifi(void);
static esp_err_t event_handler(void *ctx, system_event_t *event);
static EventGroupHandle_t wifi_event_group;

static const char *TAG = "MAIN";

void humidity_task(void *pvParameter) {

	/* Configure the IOMUX register for pad BLINK_GPIO (some pads are
	 muxed to GPIO on reset already, but some default to other
	 functions and need to be switched to GPIO. Consult the
	 Technical Reference for a list of pads and their default
	 functions.)
	 */
	gpio_pad_select_gpio(HUMIDITY_GPIO);
	/* Set the GPIO as a push/pull output */
	gpio_set_direction(HUMIDITY_GPIO, GPIO_MODE_OUTPUT);
	while (1) {
		/* Blink off (output low) */
		gpio_set_level(HUMIDITY_GPIO, 0);
		vTaskDelay(1000 / portTICK_PERIOD_MS);
		/* Blink on (output high) */
		gpio_set_level(HUMIDITY_GPIO, 1);
		vTaskDelay(1000 / portTICK_PERIOD_MS);
		ESP_LOGI(TAG, "Task delayed");
	}
}

void printTemp(void *pvParameter) {
	const gpio_num_t SENSOR_GPIO = TEMP_ONEWIRE_GPIO;
	const int MAX_SENSORS = 10;
	ds18b20_addr_t addrs[MAX_SENSORS];
	int sensor_count = ds18b20_scan_devices(SENSOR_GPIO, addrs, MAX_SENSORS);
	ESP_LOGI(TAG, "%d sensors found", sensor_count);
	float temps[sensor_count];


	gpio_pad_select_gpio(SENSOR_GPIO);
	while (1) {
		 ds18b20_measure_and_read_multi(SENSOR_GPIO, addrs, sensor_count, temps);

		for (uint8_t i = 0; i < sensor_count; i++) {
			ESP_LOGI(TAG, "Sensor %d: %f", i, temps[i]);
		}
		vTaskDelay(10 * 1000 / portTICK_PERIOD_MS);
	}
}

void app_main() {
	ESP_ERROR_CHECK(nvs_flash_init());
	initialise_wifi();

	ESP_LOGI(TAG, "Starting tasks");
	/*
	 xTaskCreate(&humidity_task, "humidity_task", configMINIMAL_STACK_SIZE * 2,
	 NULL, 5, NULL);
	 xTaskCreate(&getTime, "getTime", configMINIMAL_STACK_SIZE * 5,
	 wifi_event_group, 3, NULL);
	 */
	xTaskCreate(&printTemp, "printTemp", configMINIMAL_STACK_SIZE * 10,
			wifi_event_group, 3, NULL);

//ESP_ERROR_CHECK( esp_wifi_stop() );
}

static void initialise_wifi(void) {
	tcpip_adapter_init();
	wifi_event_group = xEventGroupCreate();
	ESP_ERROR_CHECK(esp_event_loop_init(event_handler, NULL));
	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT()
	;
	ESP_ERROR_CHECK(esp_wifi_init(&cfg));
	ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
	wifi_config_t wifi_config = { .sta = { .ssid = WIFI_SSID, .password =
			WIFI_PASS, }, };

	ESP_LOGI(TAG, "Setting WiFi configuration SSID %s...", wifi_config.sta.ssid);
	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
	ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
	ESP_ERROR_CHECK(esp_wifi_start());
}

static esp_err_t event_handler(void *ctx, system_event_t *event) {
	ESP_LOGI(TAG, "Event number %d", event->event_id);
	switch (event->event_id) {
	case SYSTEM_EVENT_STA_START:
		esp_wifi_connect();
		break;
	case SYSTEM_EVENT_STA_GOT_IP:
		xEventGroupSetBits(wifi_event_group, CONNECTED_BIT);
		break;
	case SYSTEM_EVENT_STA_DISCONNECTED:
		/* This is a workaround as ESP32 WiFi libs don't currently
		 auto-reassociate. */
		esp_wifi_connect();
		xEventGroupClearBits(wifi_event_group, CONNECTED_BIT);
		break;
	default:
		break;
	}
	return ESP_OK;
}

