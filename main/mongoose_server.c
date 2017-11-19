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
#include "configuration.h"
#include "ap_serializer.h"

static const char *TAG = "MONGOOSE";
#define MG_LISTEN_ADDR "80"

static void http_serve_start_page(struct mg_connection *nc, int ev,
		void *ev_data);
static void http_save_wifi_credentials(struct mg_connection *nc, int ev,
		void *ev_data);
static void api_handle_lighting(struct mg_connection *nc, int ev, void *ev_data);
static void api_handle_status(struct mg_connection *nc, int ev, void *ev_data);
static void api_handle_ap_records(struct mg_connection *nc, int ev,
		void *ev_data);
static void http_serve_axios(struct mg_connection *nc, int ev, void *ev_data);
static void http_serve_vuejs(struct mg_connection *nc, int ev, void *ev_data);


static void send_not_found_and_close(struct mg_connection *nc){
	mg_send_head(nc, 404, 0, "Content-Type: text/plain");
	nc->flags |= MG_F_SEND_AND_CLOSE;
}

static void send_no_content_and_close(struct mg_connection *nc){
	mg_send_head(nc, 201, 0, "Connection: close");
	nc->flags |= MG_F_SEND_AND_CLOSE;
}


static bool is_softAP() {
	EventBits_t wifi_bits = xEventGroupWaitBits(event_group,
	AP_CONNECTED_BIT,
	false, false, 0);
	return wifi_bits & AP_CONNECTED_BIT;
}

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
		/*
		 if (strncmp("POST", hm->method.p, hm->method.len)) {

		 ESP_LOGI(TAG, "GET");
		 http_serve_start_page(nc);
		 } else {
		 ESP_LOGI(TAG, "POST");
		 http_serve_start_page(nc);
		 }
		 */
		break;
	}
	case MG_EV_SEND: {
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
	mg_register_http_endpoint(nc, "/api/aps", api_handle_ap_records);
	mg_register_http_endpoint(nc, "/api/lighting", api_handle_lighting);
	mg_register_http_endpoint(nc, "/api/status", api_handle_status);
	mg_register_http_endpoint(nc, "/assets/vue.min.js", http_serve_vuejs);
	mg_register_http_endpoint(nc, "/assets/axios.min.js", http_serve_axios);
	mg_register_http_endpoint(nc, "/", http_serve_start_page);

	mg_set_protocol_http_websocket(nc);

	/* Processing events */
	while (1) {
		mg_mgr_poll(&mgr, 1000);
	}
}
static const char *json_fmt = "HTTP/1.0 200 OK\r\n"
		"Connection: close\r\n"
		"Content-Type: application/json\r\n"
		"\r\n"
		"%s";

static const char *JSON_HEADER = "Connection: close\r\n"
		"Cache-Control: no-cache\r\n"
		"Content-Type: application/json";

static const char JS_GZIP_HEADERS[] = "Connection: close\r\n"
		"Content-Type: application/javascript\r\n"
		"Content-Encoding: gzip";
static const char HTML_HEADERS[] = "Connection: close\r\n"
		"Content-Type: text/html";


static void http_serve_integration_page(struct mg_connection *nc) {
	ESP_LOGI(TAG, "Sending the integration page");
		mg_send_head(nc, 200, softap_html_end - softap_html_start, HTML_HEADERS);
		mg_send(nc, (void *) softap_html_start,
				softap_html_end - softap_html_start);
}
static void http_serve_gziped_js(struct mg_connection *nc, const uint8_t *start, const uint8_t* end) {
	size_t len = end-start;
	mg_send_head(nc, 200, len, JS_GZIP_HEADERS);
	mg_send(nc, (void *) start, len);

}
static void http_serve_vuejs(struct mg_connection *nc, int ev,
		void *ev_data) {
	ESP_LOGI(TAG, "Sending the integration page");

	http_serve_gziped_js(nc, vue_min_js_gz_start, vue_min_js_gz_end);
}
static void http_serve_axios(struct mg_connection *nc, int ev,
		void *ev_data) {
	ESP_LOGI(TAG, "Sending the integration page");

	http_serve_gziped_js(nc, axios_min_js_gz_start, axios_min_js_gz_end);
}

static void http_get_config(struct mg_connection *nc, int ev, void *ev_data) {
	ESP_LOGI(TAG, "Sending the config page");
	mg_send_head(nc, 200, config_html_end - config_html_start, HTML_HEADERS);
	mg_send(nc, (void *) config_html_start,
			config_html_end - config_html_start);
}
static void http_serve_start_page(struct mg_connection *nc, int ev,
		void *ev_data) {
	if (is_softAP()) {
		http_serve_integration_page(nc);
	} else {
		http_get_config(nc, ev, ev_data);
	}
}

static void api_handle_ap_records(struct mg_connection *nc, int ev,
		void *ev_data) {
	ESP_LOGI(TAG, "hanlding api_handle_ap_records")
	char *response = wifi_serialize_scanned_ap();
	size_t len = strlen(response);
	mg_send_head(nc, 200, len, JSON_HEADER);
	mg_send(nc, (void *) response, len);
	free(response);
}

static void api_handle_status(struct mg_connection *nc, int ev, void *ev_data) {
	ESP_LOGD(TAG, "api_handle_lighting");
	struct http_message *hm = (struct http_message *) ev_data;
	if (strncmp("GET", hm->method.p, hm->method.len)) {
		//accept only GET
		send_not_found_and_close(nc);
		return;
	}
	char *json = serialize_status();
	mg_printf(nc, json_fmt, json);
	nc->flags |= MG_F_SEND_AND_CLOSE;
	free(json);

}
static void api_handle_lighting(struct mg_connection *nc, int ev, void *ev_data) {
	ESP_LOGD(TAG, "api_handle_lighting");
	struct http_message *hm = (struct http_message *) ev_data;
	printf("HTTP request: (%.*s) %.*s\n", (int) hm->method.len, hm->method.p,
			(int) hm->uri.len, hm->uri.p);
	if (strncmp("GET", hm->method.p, hm->method.len) == 0) {
		ESP_LOGD(TAG, "GET api_handle_lighting");

		char *json_unformatted = serialize_configuration();
		mg_printf(nc, json_fmt, json_unformatted);
		nc->flags |= MG_F_SEND_AND_CLOSE;
		free(json_unformatted);
	}
	if (strncmp("POST", hm->method.p, hm->method.len) == 0) {
		printf("POST api_handle_lighting");

		ESP_ERROR_CHECK(
				update_configuration_from_json(hm->body.p, hm->body.len));
		ESP_ERROR_CHECK(persist_config());
		send_no_content_and_close(nc);

	} else {
		ESP_LOGD(TAG, "Else api_handle_lighting");
		nc->flags |= MG_F_SEND_AND_CLOSE;
	}
}
static void http_save_wifi_credentials(struct mg_connection *nc, int ev,
		void *ev_data) {

	char ssid[32], password[64];
	struct http_message *hm = (struct http_message *) ev_data;

	//mg_get_http_var(const struct mg_str *buf, const char *name, char *dst, size_t dst_len)
	mg_get_http_var(&hm->body, "s", ssid, sizeof(ssid));
	mg_get_http_var(&hm->body, "p", password, sizeof(password));
	ESP_LOGI(TAG, "SSID: %s, Password: %s", ssid, password);

	size_t content_length = sizeof(REDIRECT);

	mg_send_head(nc, 200, content_length, HTML_HEADERS);
	mg_send(nc, (void *) REDIRECT, content_length);

	nc->flags |= MG_F_SEND_AND_CLOSE;
	ESP_LOGI(TAG, "Waiting few second");
	ets_delay_us(300);

	ESP_LOGI(TAG, "Switching to new AP");
	set_wifi_sta_and_start(ssid, password);

	ESP_LOGI(TAG, "set_wifi_sta_and_start existed");
	ESP_LOGI(TAG, "Waiting for STA_CONNECTED_BIT | STA_CONNECTION_ERR_BIT");
	EventBits_t wifi_bits = xEventGroupWaitBits(event_group,
	STA_CONNECTED_BIT /*| STA_CONNECTION_ERR_BIT*/,
	false, false, pdMS_TO_TICKS(10 * 1000));
	ESP_LOGI(TAG,
			"Waiting for STA_CONNECTED_BIT | STA_CONNECTION_ERR_BIT returned: %u",
			wifi_bits);
}

