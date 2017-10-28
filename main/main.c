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
#include "esp_log.h"
#include "esp_adc_cal.h"
#include "constants.h"
#include "ds18b20.h"
#include "mongoose_server.h"
#include "wifi_manager.h"

#define HUMIDITY_GPIO CONFIG_HUMIDITY_GPIO
#define LED_BUILTIN 2

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

void blink_builtin_led(void *pvParameter) {
	gpio_pad_select_gpio(LED_BUILTIN);
	/* Set the GPIO as a push/pull output */
	gpio_set_direction(LED_BUILTIN, GPIO_MODE_OUTPUT);
	while (1) {
		gpio_set_level(LED_BUILTIN, 1);
		vTaskDelay(500 / portTICK_PERIOD_MS);
		gpio_set_level(LED_BUILTIN, 0);
		vTaskDelay(2000 / portTICK_PERIOD_MS);
	}
}

#define V_REF   1100
#define ADC1_TEST_CHANNEL (ADC1_CHANNEL_6)      //GPIO 34
//#define V_REF_TO_GPIO  //Remove comment on define to route v_ref to GPIO

void get_light() {
	//Init ADC and Characteristics
	esp_adc_cal_characteristics_t characteristics;
	adc1_config_width(ADC_WIDTH_12Bit);
	adc1_config_channel_atten(ADC1_TEST_CHANNEL, ADC_ATTEN_0db);
	esp_adc_cal_get_characteristics(V_REF, ADC_ATTEN_0db, ADC_WIDTH_12Bit,
			&characteristics);
	uint32_t voltage;
	while (1) {
		voltage = adc1_to_voltage(ADC1_TEST_CHANNEL, &characteristics);
		//measures showed min: 54, max: 1018 normalize to a percent
		int percent = ((voltage - 54) * 100) / 964;
		ESP_LOGI(TAG, "%d mV -> %d%% percent", voltage, percent);
		vTaskDelay(pdMS_TO_TICKS(1000));
	}
}

void print_system_information() {
	ESP_LOGI(TAG,  "SDK version: %s", esp_get_idf_version());
	while (1) {
		ESP_LOGI(TAG,  "Free heap size: %u bytes", esp_get_free_heap_size());
		vTaskDelay(pdMS_TO_TICKS(1000 * 10));
	}
}

void app_main() {
	ESP_ERROR_CHECK(nvs_flash_init());
	initialise_wifi();
	ESP_LOGI(TAG, "Starting tasks");
	xTaskCreate(&print_system_information, "print_system_information",
				1024 * 2, NULL, 1, NULL);
	//wifi_start_soft_ap();


	xTaskCreate(&get_time, "get_time", configMINIMAL_STACK_SIZE * 5,
			event_group, 3, NULL);

	/*
	 xTaskCreate(&get_light, "get_light", configMINIMAL_STACK_SIZE * 2,
	 event_group, 3, NULL);

	 xTaskCreate(&printTemp, "printTemp", configMINIMAL_STACK_SIZE * 10,
	 event_group, 3, NULL);
	 */
	xTaskCreate(&blink_builtin_led, "blink_builtin_led",
			configMINIMAL_STACK_SIZE, event_group, 3, NULL);

	xTaskCreate(&http_serve, "http_server", 2048 * 10, NULL, 5, NULL);

//ESP_ERROR_CHECK( esp_wifi_stop() );
}

