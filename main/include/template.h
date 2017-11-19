static const char *REDIRECT =
		"<html><head><meta http-equiv=\"refresh\" content=\"20;url=http://www.google.com/\" /></head>"
		"<body><h1>Connecting to %s. You will be redirected in 20 seconds...</h1></body></html>";

extern const uint8_t config_html_start[] asm("_binary_config_html_start");
extern const uint8_t config_html_end[] asm("_binary_config_html_end");

extern const uint8_t softap_html_start[] asm("_binary_softap_html_start");
extern const uint8_t softap_html_end[] asm("_binary_softap_html_end");

extern const uint8_t vue_min_js_gz_start[] asm("_binary_vue_min_js_gz_start");
extern const uint8_t vue_min_js_gz_end[] asm("_binary_vue_min_js_gz_end");

extern const uint8_t axios_min_js_gz_start[] asm("_binary_axios_min_js_gz_start");
extern const uint8_t axios_min_js_gz_end[] asm("_binary_axios_min_js_gz_end");
