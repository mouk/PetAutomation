/*
 * constants.h
 *
 *  Created on: 8 Oct 2017
 *      Author: Moukarram Kabbash
 */
#include <stdint.h>
#ifndef MAIN_CONSTANTS_H_
#define MAIN_CONSTANTS_H_


/* Are we connected to the AP with an IP? */
#define HUMIDITY_GPIO CONFIG_HUMIDITY_GPIO
#define LED_BUILTIN 2

// Event group
#define STA_CONNECTED_BIT BIT0
#define AP_CONNECTED_BIT BIT1
//#define STA_CONNECTION_ERR_BIT BIT2
#define TEMP_ONEWIRE_GPIO 23
#define SOFTAP_TIMEOUT_IN_S (5 *60)
#define V_REF   1100
#define PHOTORESISTOR_ADC1_CHANNEL (ADC1_CHANNEL_6)      //GPIO 34
#define MAX_SENSORS 4

#endif /* MAIN_CONSTANTS_H_ */
