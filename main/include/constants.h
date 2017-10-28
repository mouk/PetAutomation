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

// Event group
#define STA_CONNECTED_BIT BIT0
#define AP_CONNECTED_BIT BIT1
//#define STA_CONNECTION_ERR_BIT BIT2
#define TEMP_ONEWIRE_GPIO 23
#define SOFTAP_TIMEOUT_IN_S (5 *60)

#endif /* MAIN_CONSTANTS_H_ */
