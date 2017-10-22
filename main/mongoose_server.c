/*
 * mongoose_server.c
 *
 *  Created on: 20 Oct 2017
 *      Author: Moukarram Kabbash
 */

#include <string.h>

#include "freertos/FreeRTOS.h"

#include "esp_event.h"
#include "esp_event_loop.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "nvs_flash.h"
#include "mongoose.h"
#include "mongoose_server.h"
#include "template.h"
#include "esp_log.h"
#include "wifi_manager.h"

static const char *TAG = "MONGOOSE";
#define MG_LISTEN_ADDR "80"

void http_serve_start_page(struct mg_connection *nc);
void http_save_wifi_credentials(struct mg_connection *nc, int ev, void *ev_data);

static void mg_ev_handler(struct mg_connection *nc, int ev, void *p) {

	switch (ev) {
	case MG_EV_ACCEPT: {
		char addr[32];
		mg_sock_addr_to_str(&nc->sa, addr, sizeof(addr),
		MG_SOCK_STRINGIFY_IP | MG_SOCK_STRINGIFY_PORT);
		printf("Connection %p from %s\n", nc, addr);
		break;
	}
	case MG_EV_HTTP_REQUEST: {
		char addr[32];
		struct http_message *hm = (struct http_message *) p;
		mg_sock_addr_to_str(&nc->sa, addr, sizeof(addr),
		MG_SOCK_STRINGIFY_IP | MG_SOCK_STRINGIFY_PORT);
		printf("HTTP request from %s: (%.*s) %.*s\n", addr,
				(int) hm->method.len, hm->method.p, (int) hm->uri.len,
				hm->uri.p);
		if (strncmp("POST", hm->method.p, hm->method.len)) {

			ESP_LOGI(TAG, "GET");
			http_serve_start_page(nc);
		} else {
			ESP_LOGI(TAG, "POST");
			http_serve_start_page(nc);
		}
		break;
	}
	case MG_EV_SEND:{
		printf("MG_EV_SEND %p\n", nc);
		break;
	}
	case MG_EV_CLOSE: {
		printf("Connection %p closed\n", nc);
		break;
	}
	}
}

void http_serve(void *pvParameters) {

	/* Starting Mongoose */
	struct mg_mgr mgr;
	struct mg_connection *nc;

	printf("Starting web-server on port %s\n", MG_LISTEN_ADDR);

	mg_mgr_init(&mgr, NULL);

	nc = mg_bind(&mgr, MG_LISTEN_ADDR, mg_ev_handler);
	if (nc == NULL) {
		printf("Error setting up listener!\n");
		return;
	}
	mg_register_http_endpoint(nc, "/wifisave", http_save_wifi_credentials);
	mg_set_protocol_http_websocket(nc);

	/* Processing events */
	while (1) {
		mg_mgr_poll(&mgr, 1000);
	}
}

void http_serve_start_page(struct mg_connection *nc) {
	//mg_printf_http_chunk(nc, "Error: %s", msg);
	/* Send empty chunk, the end of response */
	mg_printf(nc, "%s",
			"HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n");
	mg_printf_http_chunk(nc, "%s %s %s %s %s %s %s", HTTP_HEAD, HTTP_STYLE,
			HTTP_HEAD_END, HTTP_FORM_START,
			//HTTP_FORM_PARAM,
			HTTP_FORM_END, HTTP_SAVED, HTTP_END);
	mg_send_http_chunk(nc, "", 0);
}

void http_save_wifi_credentials(struct mg_connection *nc, int ev, void *ev_data) {

	char ssid[32], password[64];
	struct http_message *hm = (struct http_message *) ev_data;

	//mg_get_http_var(const struct mg_str *buf, const char *name, char *dst, size_t dst_len)
	mg_get_http_var(&hm->body, "s", ssid, sizeof(ssid));
	mg_get_http_var(&hm->body, "p", password, sizeof(password));
	ESP_LOGI(TAG, "SSID: %s, Password: %s", ssid, password);

	static const char *redirect_fmt =
			"HTTP/1.0 200 OK\r\n"
					"Connection: close\r\n"
					"Content-Type: text/html\r\n"
					"\r\n"
					"<html><head><meta http-equiv=\"refresh\" content=\"20;url=http://www.google.com/\" /></head>"
					"<body><h1>Connecting to %s. You will be redirected in 20 seconds...</h1></body></html>";
	mg_printf(nc, redirect_fmt, ssid);

	nc->flags |= MG_F_SEND_AND_CLOSE;
	ESP_LOGI(TAG, "Waiting few second");
	ets_delay_us(300);

	ESP_LOGI(TAG, "Switching to new AP");
	set_wifi_sta_and_start(ssid, password);
	EventBits_t result = xEventGroupWaitBits(event_group, STA_CONNECTED_BIT,
	false, true, (30 * 1000) / portTICK_PERIOD_MS);
	ESP_LOGI(TAG, "Connecting result %d", result);

	if (result & STA_CONNECTED_BIT) {
		ESP_LOGI(TAG, "Closing soft ap");
		wifi_stop_soft_ap();
	}
}
