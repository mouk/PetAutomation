/*
 * sensors.c
 *
 *  Created on: 28 Oct 2017
 *      Author: Moukarram Kabbash
 */

#include "esp_adc_cal.h"
#include "constants.h"
#include "sensors.h"
#include "esp_log.h"
#include "ds18b20.h"
#include "string.h"
#include <time.h>

static const char *TAG = "SENSORS";

static const gpio_num_t SENSOR_GPIO = TEMP_ONEWIRE_GPIO;
static ds18b20_addr_t addrs[MAX_SENSORS];
static esp_adc_cal_characteristics_t characteristics;
static int sensor_count;

void init_sensors(void) {
	adc1_config_width(ADC_WIDTH_12Bit);
	adc1_config_channel_atten(PHOTORESISTOR_ADC1_CHANNEL, ADC_ATTEN_0db);
	esp_adc_cal_get_characteristics(V_REF, ADC_ATTEN_0db, ADC_WIDTH_12Bit,
			&characteristics);

	gpio_pad_select_gpio(SENSOR_GPIO);
	sensor_count = ds18b20_scan_devices(SENSOR_GPIO, addrs, MAX_SENSORS);
	ESP_LOGD(TAG, "%d sensors found", sensor_count);
}

static int8_t get_brightness() {
	//Init ADC and Characteristics
	uint32_t voltage = adc1_to_voltage(PHOTORESISTOR_ADC1_CHANNEL,
			&characteristics);
	//measures showed min: 54, max: 1018 normalize to a percent
	int8_t percent = ((voltage - 54) * 100) / 964;
	ESP_LOGD(TAG, "%d mV -> %d%% percent", voltage, percent);
	return percent;
}

static esp_err_t get_temp_readings(float *temp_readings, uint8_t *count) {
	memset(temp_readings, 0, MAX_SENSORS * sizeof(float));
	bool ok = ds18b20_measure_and_read_multi(SENSOR_GPIO, addrs, sensor_count,
			temp_readings);
	if (!ok) {
		ESP_LOGW(TAG, "get_temp_readings failed");
		return ESP_FAIL;
	}

	for (uint8_t i = 0; i < sensor_count; i++) {
		ESP_LOGD(TAG, "Sensor %d: %f", i, temp_readings[i]);
	}
	*count = sensor_count;
	return ESP_OK;
}
esp_err_t get_seasors_reading(sensors_reading_t *result) {
	time(&result->timestamp);
	result->brightness = get_brightness();
	float temp_readings[MAX_SENSORS] = { 0 };
	uint8_t number_of_temp_readings;

	esp_err_t temp_ok = get_temp_readings(temp_readings,
			&number_of_temp_readings);
	if (temp_ok == ESP_OK) {
		result->temp_reading_1 = temp_readings[0];
		result->temp_reading_2 = temp_readings[1];
		result->temp_reading_3 = temp_readings[2];
		result->temp_reading_4 = temp_readings[3];
		result->temp_readings_count = number_of_temp_readings;
		return ESP_OK;
	}
	return ESP_FAIL;
}

void printout_sensors_reading(sensors_reading_t *sensors_reading) {
	char strftime_buf[64];
		struct tm timeinfo;
		localtime_r(&sensors_reading->timestamp, &timeinfo);
		strftime(strftime_buf, sizeof(strftime_buf), "%T", &timeinfo);

	ESP_LOGI(TAG,  "Time: %s Brightness :%d%%, temp sensors: %d, temp1: %u, temp2 %u",
			strftime_buf,
			sensors_reading->brightness,
			sensors_reading->temp_readings_count,
			sensors_reading->temp_reading_1,
			sensors_reading->temp_reading_2);
}
