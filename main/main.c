#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "driver/gpio.h"
#include "sdkconfig.h"
#include "sntp.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "mongoose_server.h"
#include "wifi_manager.h"
#include "sensors.h"
#include "actors.h"
#include "automation_logic.h"
#include <time.h>
#include "configuration.h"

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


//#define V_REF_TO_GPIO  //Remove comment on define to route v_ref to GPIO

void print_system_information() {
	ESP_LOGI(TAG,  "SDK version: %s", esp_get_idf_version());
	while (1) {
		ESP_LOGI(TAG,  "Free heap size: %u bytes", esp_get_free_heap_size());
		vTaskDelay(pdMS_TO_TICKS(1000 * 60));
	}
}

void sensors_actors_main_task(void *pvParameter) {
	init_sensors();

	while (1) {
		sensors_reading_t sensors_reading;
		esp_err_t sensors_ok = get_seasors_reading(&sensors_reading);
		if(sensors_ok == ESP_OK){
			printout_sensors_reading(&sensors_reading);

			actors_state_t actors_state;
			esp_err_t result = process_sensors_reading(&sensors_reading, &actors_state);
			if(result == ESP_OK){
				 printout_actors_state(&actors_state);
				 result = apply_actors_state(&actors_state);
				 ESP_LOGD(TAG,  "Result of apply actors state: %d", result);
			}

		}else{
			ESP_LOGW(TAG,  "Couldn't get sensors reading");
		}
		//Wait for the next minute
		vTaskDelay(pdMS_TO_TICKS(1000 * 60));
	}
}

void app_main() {
	ESP_ERROR_CHECK(nvs_flash_init());
	init_configuration();
	initialise_wifi();
	ESP_LOGI(TAG, "Starting tasks");
	xTaskCreate(&print_system_information, "print_system_information",
				1024 * 2, NULL, 1, NULL);

	xTaskCreate(&http_serve, "http_server", 2048 * 10, NULL, 5, NULL);


	xTaskCreate(&get_time, "get_time", configMINIMAL_STACK_SIZE * 5,
			event_group, 3, NULL);

	xTaskCreate(&blink_builtin_led, "blink_builtin_led",
			configMINIMAL_STACK_SIZE, event_group, 3, NULL);
	 xTaskCreate(&sensors_actors_main_task, "sensors_actors_main_task", 1024 * 2,
	 event_group, 3, NULL);

	/*

	 xTaskCreate(&printTemp, "printTemp", configMINIMAL_STACK_SIZE * 10,
	 event_group, 3, NULL);
	 */
}

