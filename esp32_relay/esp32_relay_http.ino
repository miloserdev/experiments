
#ifdef __cplusplus
extern "C" {
#endif

uint8_t temprature_sens_read();
//uint8_t g_phyFuns;

#ifdef __cplusplus
}
#endif

#define R1 32
#define R2 33
#define R3 25
#define R4 26
#define MIN(a, b)(((a) < (b)) ? (a) : (b))
#define MAX(a, b)(((a) > (b)) ? (a) : (b))

// 
#include "RTCDS1307.h"
//
#include "Update.h"
//#include "WiFi.h"
#include <stdio.h>
#include <esp_err.h>
#include <esp_system.h>
#include <esp_event.h>
#include <esp_event_loop.h>
#include <esp_event_legacy.h>
#include <esp_intr_alloc.h>

static const char *TAG = "relay_array_1";
#include <esp_wifi.h>
#include <esp_wifi_types.h>
#include <tcpip_adapter.h>
#define WIFI_SSID "Keenetic-6193"
#define WIFI_PASSWORD "DNj6KdZT"
#include <esp_http_client.h>
#include <esp_http_server.h>
#define PORT 8081
#include <esp_partition.h>
#include <esp_ota_ops.h>   	// get running partition
#define OTA_BUF_SIZE 1024
#include <esp_sntp.h>
#define SNTP_SERVER "pool.ntp.org"
//
#define SNTP_SYNC_INTERVAL ((60 * 60) * 1000) * 12	// 12 hours;

#define SNTP_SYNC_INTERVAL (((60 * 60) * 1000) *24) * 7	// 7 days;
#include "JSON.h"
#include "cjson/cJSON.h"
#include <SD.h>
#include <Wire.h>
#include "RTClib.h"

RTC_DS1307 RTC;
DateTime _now;
#include <TaskScheduler.h>

Scheduler runner;

//bool connected = false;
//wl_status_t wifi_status;

//uint32_t reconnect_time = 5;	//seconds
//uint32_t reconnect_time_prev = 0;
//uint32_t reconnect_time_max = 120;

httpd_handle_t server = NULL;

cJSON * buffer;
cJSON * sched_cmd;
uint32_t scheduled_time;
bool scheduled_isset = false;

int turn_on_hour = 17;
int turn_on_minute = 1;

int turn_off_hour = 1;
int turn_off_minute = 1;

uint8_t year, month, weekday, day, hour, minute, second;
uint32_t __unixtime__;	//seconds!
bool period = 0;




char *json_type(cJSON *json)
{
	if (json == NULL || cJSON_IsInvalid(json))
	{
		return "undefined";
	}
	else if (cJSON_IsBool(json))
	{
		return "boolean";
	}
	else if (cJSON_IsNull(json))
	{
		return "null";	// TODO: should this return "object" to be more JS like?
	}
	else if (cJSON_IsNumber(json))
	{
		return "number";
	}
	else if (cJSON_IsString(json))
	{
		return "string";
	}
	else if (cJSON_IsArray(json))
	{
		return "array";	// TODO: should this return "object" to be more JS like?
	}
	else if (cJSON_IsObject(json))
	{
		return "object";
	}
	else
	{
		return "unknown";
	}
}

uint32_t get_time()
{
	_now = RTC.now();

	year = _now.year();
	month = _now.month();
	weekday = _now.dayOfTheWeek();
	day = _now.day();
	hour = _now.hour();
	minute = _now.minute();
	second = _now.second();
	__unixtime__ = _now.unixtime();

	/*
	    Serial.print("unixtime=");
	    printf(_now.unixtime());
	    Serial.print("-");
	    Serial.print(day);
	    Serial.print(" of ");
	    Serial.print(month);
	    Serial.print(" ");
	    Serial.print(hour);
	    Serial.print(":");
	    Serial.print(minute);
	    Serial.print(":");
	    Serial.print(second);
	    printf(" ");

	    Serial.print(">");
	    Serial.print(day);
	    Serial.print("/");
	    Serial.print(month);
	    Serial.print("/");
	    Serial.print(year);
	    Serial.print("-");
	    Serial.print(hour);
	    Serial.print(":");
	    Serial.print(minute);
	    Serial.print(":");
	    Serial.print(second);
	    printf(";");
	*/

	printf("time -> %d \n", __unixtime__);
	return __unixtime__;
}

//Task get_time_task(1000, TASK_FOREVER, []() { 	get_time(); }, &runner, true, NULL, NULL);

void sntp_update_rtc(struct timeval *tv)
{
	printf("SNTP RTC update event \n");
	struct DateTime dt = DateTime(tv->tv_sec);
	RTC.adjust(dt);
}

void update_time_sntp()
{
	setenv("TZ", "Europe/Moscow", 1);
	tzset();

	if (sntp_enabled())
	{
		sntp_stop();
	}

	sntp_setoperatingmode(SNTP_OPMODE_POLL);
	sntp_servermode_dhcp(1);
	sntp_setservername(0, SNTP_SERVER);
	sntp_set_time_sync_notification_cb(&sntp_update_rtc);
	sntp_set_sync_mode(SNTP_SYNC_MODE_SMOOTH);
	//    sntp_set_sync_interval(10000);	//deprecated
	sntp_init();

	printf("SNTP try fetch time");

	//  time_t now = 0;
	//  struct tm timeinfo = { 0 };

	uint32_t retry = 0;
	uint32_t retries = 10;
	while (sntp_get_sync_status() == SNTP_SYNC_STATUS_RESET && ++retry < retries)
	{
		printf(".");
	}

	printf("SNTP fetch end \n");

	//    time(&now);
	//    localtime_r(&now, &timeinfo);

	/*
	    Serial.print("Now: ");
	    printf(now);
	    Serial.print("TM_MDAY: ");
	    printf(timeinfo.tm_sec);
	    printf(timeinfo.tm_min);
	    printf(timeinfo.tm_hour);
	    printf(timeinfo.tm_mday);
	    printf(timeinfo.tm_mon);
	    printf(timeinfo.tm_year);
	    printf(timeinfo.tm_wday);

	    printf("");
	*/

}

Task get_sntp_task(SNTP_SYNC_INTERVAL, TASK_FOREVER, []()
{
	update_time_sntp();
}, &runner, true, NULL, NULL);

esp_err_t get_time_handler(httpd_req_t *req)
{
	printf("get_time_handler start \n");
	char buf[16];
	sprintf(buf, "%lu", __unixtime__);
	httpd_resp_send(req, buf, HTTPD_RESP_USE_STRLEN);
	printf("get_time_handler end \n");
	return ESP_OK;
}

esp_err_t get_handler(httpd_req_t *req)
{
	char *buf = "pong";
	// cJSON *buf_val = cJSON_CreateObject();
	// cJSON_AddStringToObject(buf_val, "pong", ret_);
	printf("get method uri: %s \n", req->uri);
	httpd_resp_send(req, buf, HTTPD_RESP_USE_STRLEN);
	return ESP_OK;
}

esp_err_t post_handler(httpd_req_t *req)
{
	printf("post_handler start \n");
	char content[512];
	size_t recv_size = MIN(req->content_len, sizeof(content));

	int ret = httpd_req_recv(req, content, recv_size);
	if (ret <= 0)
	{
		if (ret == HTTPD_SOCK_ERR_TIMEOUT)
		{
			httpd_resp_send_408(req);
		}

		return ESP_FAIL;
	}

	printf("buffer: %.*s \n", req->content_len, content);
	buffer = cJSON_Parse((const char *) content);
	const char *resp = (const char *) exec_packet(buffer);

	httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);

	printf("post_handler end \n");

	return ESP_OK;
}

/*
static const char static_html[] PROGMEM =
    "<!DOCTYPE html>"
    "<html lang='en'>"
    "<head>"
    "    <meta charset='utf-8'>"
    "    <meta name='viewport' content='width=device-width,initial-scale=1'/>"
    "</head>"
    "<body>"
    "<form method='POST' action='' enctype='multipart/form-data'>"
    "     Firmware:<br>"
    "    <input type='file' accept='.bin,.bin.gz' name='firmware'>"
    "    <input type='submit' value='Update Firmware'>"
    "</form>"
    "<form method='POST' action='' enctype='multipart/form-data'>"
    "     FileSystem:<br>"
    "    <input type='file' accept='.bin,.bin.gz,.image' name='filesystem'>"
    "    <input type='submit' value='Update FileSystem'>"
    "</form>"
    "</body>"
    "</html>)";
esp_err_t get_update_handler(httpd_req_t* req)
{
    Serial.print("get method uri: ");
    printf(req->uri);

    httpd_resp_set_hdr(req, "Connection", "close");
    httpd_resp_send(req, static_html, HTTPD_RESP_USE_STRLEN);
    httpd_resp_set_status(req, "200 Ok");
    return ESP_OK;
}

*/

//pure ESP variant	// encryption type multipart/form-data	//temporary solutiuon
// thanks to @kimata from rabbit-note.com
// curl 192.168.1.73:8081/update/ --no-buffer --data-binary @./sketch_relay_new.ino.bin
static esp_err_t http_handle_ota(httpd_req_t *req)
{
	printf("http_handle_ota start \n");

	const esp_partition_t * part;
	esp_ota_handle_t handle;
	char buf[OTA_BUF_SIZE];
	int total_size;
	int recv_size;
	int remain;
	uint8_t percent;

	printf(TAG, "Start to update firmware. \n");

	ESP_ERROR_CHECK(httpd_resp_set_type(req, "text/plain"));
	ESP_ERROR_CHECK(httpd_resp_sendstr_chunk(req, "Start to update firmware.\n"));

	part = esp_ota_get_next_update_partition(NULL);
	// part_info_show("Target", part);

	total_size = req->content_len;

	printf(TAG, "Sent size: %d KB. \n", total_size / 1024);

	ESP_ERROR_CHECK(httpd_resp_sendstr_chunk(req, "0        20        40        60        80       100%\n"));
	ESP_ERROR_CHECK(httpd_resp_sendstr_chunk(req, "|---------+---------+---------+---------+---------+\n"));
	ESP_ERROR_CHECK(httpd_resp_sendstr_chunk(req, "*"));

	ESP_ERROR_CHECK(esp_ota_begin(part, total_size, &handle));
	remain = total_size;
	percent = 2;
	while (remain > 0)
	{
		if (remain < sizeof(buf))
		{
			recv_size = remain;
		}
		else
		{
			recv_size = sizeof(buf);
		}

		recv_size = httpd_req_recv(req, buf, recv_size);
		if (recv_size <= 0)
		{
			if (recv_size == HTTPD_SOCK_ERR_TIMEOUT)
			{
				continue;
			}

			httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR,
				"Failed to receive firmware.");
			return ESP_FAIL;
		}

		ESP_ERROR_CHECK(esp_ota_write(handle, buf, recv_size));

		remain -= recv_size;
		if (remain < (total_size *(100 - percent) / 100))
		{
			httpd_resp_sendstr_chunk(req, "*");
			percent += 2;
		}
	}

	ESP_ERROR_CHECK(esp_ota_end(handle));
	ESP_ERROR_CHECK(esp_ota_set_boot_partition(part));
	printf(TAG, "Finished writing firmware. \n");

	httpd_resp_sendstr_chunk(req, "*\nOK\n");
	httpd_resp_sendstr_chunk(req, NULL);

	esp_restart();

	// xTaskCreate(restart_task, "restart_task", 1024, NULL, 10, NULL);

	return ESP_OK;
}

httpd_uri_t uri_get = { .uri = "/ping",
	.method = HTTP_GET,
	.handler = &get_handler,
	.user_ctx = NULL
};

httpd_uri_t uri_post = { .uri = "/",
	.method = HTTP_POST,
	.handler = &post_handler,
	.user_ctx = NULL
};

httpd_uri_t uri_get_time = { .uri = "/time",
	.method = HTTP_GET,
	.handler = &get_time_handler,
	.user_ctx = NULL
};

/*
httpd_uri_t uri_get_update = { .uri = "/update",
    .method = HTTP_GET,
    .handler = &get_update_handler,
    .user_ctx = NULL
};

*/

httpd_uri_t uri_post_update = { .uri = "/update/",
	.method = HTTP_POST,
	.handler = &http_handle_ota,
	.user_ctx = NULL
};

httpd_handle_t start_webserver(void)
{
	printf("Starting web server \n");
	httpd_config_t config = HTTPD_DEFAULT_CONFIG();
	config.server_port = PORT;

	server = NULL;

	if (httpd_start(&server, &config) == ESP_OK)
	{
		httpd_register_uri_handler(server, &uri_get);
		httpd_register_uri_handler(server, &uri_post);
		//  httpd_register_uri_handler(server, &uri_get_update);
		httpd_register_uri_handler(server, &uri_post_update);
		httpd_register_uri_handler(server, &uri_get_time);
		printf("All handlers in register \n");
	}

	return server;
}

void stop_webserver(httpd_handle_t server)
{
	printf("stop_webserver start \n");
	if (server)
	{
		httpd_stop(server);
	}

	printf("stop_webserver end \n");
}






const char *gateway_addr = "http://192.168.1.69:8884/";

static const char *settings =
"["
"{\"id\":\"relay_1\",\"pin\":32,\"type\":\"ANALOG\",\"pinmode\":\"OUTPUT\",\"defval\":\"HIGH\",\"invert\":true},"
"{\"id\":\"relay_2\",\"pin\":33,\"type\":\"ANALOG\",\"pinmode\":\"OUTPUT\",\"defval\":\"HIGH\",\"invert\":true},"
"{\"id\":\"relay_3\",\"pin\":25,\"type\":\"ANALOG\",\"pinmode\":\"OUTPUT\",\"defval\":\"LOW\",\"invert\":true},"
"{\"id\":\"relay_4\",\"pin\":26,\"type\":\"ANALOG\",\"pinmode\":\"OUTPUT\",\"defval\":\"HIGH\",\"invert\":true}"
"]";


int parse_lohi (char *lohi)
{
    if ( strcmp("LOW", lohi) == 0 ) return 0x0;
    else if ( strcmp("HIGH", lohi) == 0 ) return 0x1;
    else return 0xDEAD;
}

int parse_type (char *type)
{
    if ( strcmp("ANALOG", type) == 0 ) return 0x1;
    else if ( strcmp("OUTPUT", type) == 0 ) return 0x2;
    else return 0xDEAD;
}

int parse_pinmode (char *pinmode)
{
    if ( strcmp("INPUT", pinmode) == 0 ) return 0x01;
    else if ( strcmp("OUTPUT", pinmode) == 0 ) return 0x02;
    else if ( strcmp("PULLUP", pinmode) == 0 ) return 0x04;
    else if ( strcmp("INPUT_PULLUP", pinmode) == 0 ) return 0x05;
    else if ( strcmp("PULLDOWN", pinmode) == 0 ) return 0x08;
    else if ( strcmp("INPUT_PULLDOWN", pinmode) == 0 ) return 0x09;
    else if ( strcmp("OPEN_DRAIN", pinmode) == 0 ) return 0x10;
    else if ( strcmp("OUTPUT_OPEN_DRAIN", pinmode) == 0 ) return 0x12;
    else if ( strcmp("SPECIAL", pinmode) == 0 ) return 0xF0;
    else if ( strcmp("FUNCTION_1", pinmode) == 0 ) return 0x00;
    else if ( strcmp("FUNCTION_2", pinmode) == 0 ) return 0x20;
    else if ( strcmp("FUNCTION_3", pinmode) == 0 ) return 0x40;
    else if ( strcmp("FUNCTION_4", pinmode) == 0 ) return 0x60;
    else if ( strcmp("FUNCTION_5", pinmode) == 0 ) return 0x70;
    else if ( strcmp("FUNCTION_6", pinmode) == 0 ) return 0xA0;
    else if ( strcmp("ANALOG", pinmode) == 0 ) return 0xC0;
    else return 0xDEAD;
}

int settings_init () {
	cJSON * buf_val;
    cJSON *sets = cJSON_Parse(settings);
    int buf_size = cJSON_GetArraySize(sets);
	printf("settings_init start \n");
    printf("settings length %d \n", buf_size);

	// USE STRCMP

	if (sets == NULL || cJSON_IsInvalid(sets))
	{
		printf("parser malfunction \n");
	}

	if (buf_size < 0)
	{
        printf("data misfunction \n");
	}

	for (uint32_t i = 0; i < buf_size; i++)
	{
		const cJSON *data = cJSON_GetArrayItem(sets, i);
        char *data_type = json_type( (cJSON *) data);
        char *data_tmp = cJSON_Print(data);

        char *id_ = cJSON_GetObjectItemCaseSensitive(data, "id")->valuestring;
        char *type_ = cJSON_GetObjectItemCaseSensitive(data, "type")->valuestring;
        int pin_ = cJSON_GetObjectItemCaseSensitive(data, "pin")->valueint;
        char *pinmode_ = cJSON_GetObjectItemCaseSensitive(data, "pinmode")->valuestring;
        char *defval_ = cJSON_GetObjectItemCaseSensitive(data, "defval")->valuestring;

        if (pin_ && pinmode_) pinMode(pin_, parse_pinmode(pinmode_));
        if (pin_ && defval_) digitalWrite(pin_, parse_lohi(defval_));
        /*{
            switch ( parse_type(type_) ) {
                case 0x1: {
                    analogWrite(pin_, parse_lohi(defval_));
                    break;
                }
                case 0x2: {
                    digitalWrite(pin_, parse_lohi(defval_));
                    break;
                }
            }
        }*/

        printf("id_: %s pin_: %i type: %i pinmode_ : %i defval: %i\n", id_, pin_, parse_type(type_), parse_pinmode(pinmode_), parse_lohi(defval_));

    }
    printf("settings_init end \n");
    return -1;
}





#define MAX_HTTP_RECV_BUFFER 512
#define MAX_HTTP_OUTPUT_BUFFER 2048


esp_err_t _http_event_handler(esp_http_client_event_t *evt)
{
    static char *output_buffer;  // Buffer to store response of http request from event handler
    static int output_len;       // Stores number of bytes read
    switch(evt->event_id) {
        case HTTP_EVENT_ERROR:
            printf("HTTP_EVENT_ERROR\n");
            break;
        case HTTP_EVENT_ON_CONNECTED:
            printf("HTTP_EVENT_ON_CONNECTED\n");
            break;
        case HTTP_EVENT_HEADER_SENT:
            printf("HTTP_EVENT_HEADER_SENT\n");
            break;
        case HTTP_EVENT_ON_HEADER:
            printf("HTTP_EVENT_ON_HEADER, key=%s, value=%s \n", evt->header_key, evt->header_value);
            break;
        case HTTP_EVENT_ON_DATA:
            printf("HTTP_EVENT_ON_DATA, len=%d \n", evt->data_len);
            /*
             *  Check for chunked encoding is added as the URL for chunked encoding used in this example returns binary data.
             *  However, event handler can also be used in case chunked encoding is used.
             */
            if (!esp_http_client_is_chunked_response(evt->client)) {
                // If user_data buffer is configured, copy the response into the buffer
                int copy_len = 0;
                if (evt->user_data) {
                    copy_len = MIN(evt->data_len, (MAX_HTTP_OUTPUT_BUFFER - output_len));
                    if (copy_len) {
                        memcpy(evt->user_data + output_len, evt->data, copy_len);
                    }
                } else {
                    const int buffer_len = esp_http_client_get_content_length(evt->client);
                    if (output_buffer == NULL) {
                        output_buffer = (char *) malloc(buffer_len);
                        output_len = 0;
                        if (output_buffer == NULL) {
                            ESP_LOGE(TAG, "Failed to allocate memory for output buffer");
                            return ESP_FAIL;
                        }
                    }
                    copy_len = MIN(evt->data_len, (buffer_len - output_len));
                    if (copy_len) {
                        memcpy(output_buffer + output_len, evt->data, copy_len);
                    }
                }
                output_len += copy_len;
            }

            break;
        case HTTP_EVENT_ON_FINISH:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_FINISH\n");
            if (output_buffer != NULL) {
                // Response is accumulated in output_buffer. Uncomment the below line to print the accumulated response
                // ESP_LOG_BUFFER_HEX(TAG, output_buffer, output_len);
                free(output_buffer);
                output_buffer = NULL;
            }
            output_len = 0;
            break;
        case HTTP_EVENT_DISCONNECTED:
            printf("HTTP_EVENT_DISCONNECTED\n");
            break;
    }
    return ESP_OK;
}


static void http_client_init () {
    esp_http_client_config_t config;
    config.url = "http://192.168.1.69:8884/";
    config.auth_type = HTTP_AUTH_TYPE_NONE;
    config.method = HTTP_METHOD_POST;
    config.event_handler = _http_event_handler;
    config.transport_type = HTTP_TRANSPORT_OVER_TCP;

    // because: sorry, unimplemented: non-trivial designated initializers not supported
    // idk how to fix;

    esp_http_client_handle_t client = esp_http_client_init(&config);

    // POST
    const char *post_data = "{\"field1\":\"value1\"}";
    //esp_http_client_set_url(client, "http://httpbin.org/post");
    esp_http_client_set_method(client, HTTP_METHOD_POST);
    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_post_field(client, post_data, strlen(post_data));
    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "HTTP POST Status = %d, content_length = %"PRIu64,
                esp_http_client_get_status_code(client),
                esp_http_client_get_content_length(client));
    } else {
        ESP_LOGE(TAG, "HTTP POST request failed: %s", esp_err_to_name(err));
    }
}











void setup()
{
	Serial.begin(115200);

	//    rtc.begin();
	Wire.begin();
	RTC.begin();
	get_time();

/*
	pinMode(R1, OUTPUT);
	pinMode(R2, OUTPUT);
	pinMode(R3, OUTPUT);
	pinMode(R4, OUTPUT);

	digitalWrite(R1, HIGH);
	digitalWrite(R2, HIGH);
	digitalWrite(R3, HIGH);
	digitalWrite(R4, HIGH);
*/

    settings_init();

	wifi_init();

    http_client_init();

	//    pinMode(13, INPUT);	//thermal;

	//    WiFi.mode(WIFI_STA);
	//    WiFi.begin(ssid, password);

	//    Serial.print("Connecting to WiFi ");
	//    while (WiFi.status() != WL_CONNECTED) {
	//        Serial.print('.');
	//        delay(1000);
	//    }

	//    printf("");

	//server = start_webserver();
}

void loop()
{
	runner.execute();
	//wifi_reconnect();
}

esp_err_t wifi_event_handler(void *ctx, system_event_t *event)
{
	printf("wifi_event_handler -> \n");
	if (event != NULL)
	{
		switch (event->event_id)
		{
			case SYSTEM_EVENT_STA_START:
				{
					if (esp_wifi_connect() != ESP_OK)
					{
						printf("Cannot connect to WiFi AP \n");
						stop_webserver(server);
					}

					//esp_err_to_name_r(esp_err_t code, char *buf, size_t buflen)
					break;
				}

			case SYSTEM_EVENT_STA_GOT_IP:
				{
					printf("IPv4-> %s \n", ip4addr_ntoa(&event->event_info.got_ip.ip_info.ip));
					printf("IPv6-> %s \n", ip6addr_ntoa(&event->event_info.got_ip6.ip6_info.ip));
					server = start_webserver();
					break;
				}

			case SYSTEM_EVENT_STA_DISCONNECTED:
				{
					if (esp_wifi_connect() != ESP_OK)
					{
						printf("Cannot connect to WiFi AP \n");
						stop_webserver(server);
					}

					break;
				}
		}
	}
}

esp_err_t wifi_init()
{
	printf("wifi_init start \n");
	tcpip_adapter_init();

	if (esp_event_loop_init(wifi_event_handler, NULL) != ESP_OK)
	{
		printf("Cannot init WiFi event loop \n");
	}

	wifi_init_config_t wifi_init_config = WIFI_INIT_CONFIG_DEFAULT();
	if (esp_wifi_init(&wifi_init_config) != ESP_OK)
	{
		printf("Cannot init WiFi module \n");
	}

	wifi_storage_t wifi_storage = WIFI_STORAGE_FLASH;
	if (esp_wifi_set_storage(wifi_storage) != ESP_OK)
	{
		printf("Cannot set WiFi storage \n");
	}

	wifi_interface_t wifi_interface = WIFI_IF_STA;
	wifi_config_t wifi_config = { .sta = {
		{.ssid = WIFI_SSID
			},
			{.password = WIFI_PASSWORD
			},
		}
	};

	if (esp_wifi_set_config(wifi_interface, &wifi_config) != ESP_OK)
	{
		printf("Cannot set WiFi config \n");
	}

	wifi_mode_t wifi_mode = WIFI_MODE_STA;
	if (esp_wifi_set_mode(wifi_mode) != ESP_OK)
	{
		printf("Cannot set WiFi mode \n");
	}

	if (esp_wifi_start() != ESP_OK)
	{
		printf("Cannot start WiFi \n");
	}

	//if (esp_wifi_connect() != ESP_OK) { 	printf("Cannot connect to WiFi AP \n"); }

	//else { 	return ESP_OK; }

	printf("wifi_init end \n");

	return ESP_OK;
}

/*
void wifi_reconnect()
{
    get_time();
    wifi_status = WiFi.status();
    if ((wifi_status != WL_CONNECTED)) {
        if ((__unixtime__ - reconnect_time_prev >= reconnect_time)) {
            printf("reconnecting...");
            WiFi.disconnect();
            WiFi.reconnect();

            reconnect_time_prev = __unixtime__;
            reconnect_time += 5;
            if (reconnect_time >= reconnect_time_max) {
                esp_restart();
            }
        }
    } else {
        reconnect_time = 5;        
    }
}

Task wifi_reconnect_task(15000, TASK_FOREVER, wifi_reconnect, &runner, true, NULL, NULL);
*/

/// WARNING
/// HARDCODE ;)
Task time_toggler(20000, TASK_FOREVER, []()
{
	printf("time_toggler start \n");

	if (hour == turn_off_hour && minute == turn_off_minute)
	{
		cJSON *_off = cJSON_Parse("[{ \"digitalWrite\": { \"pin\": 33, \"value\": 1 } }]");
		exec_packet(_off);
		printf("turned off \n");
	}
	else if (hour == turn_on_hour && minute == turn_on_minute)
	{
		cJSON *_on = cJSON_Parse("[{ \"digitalWrite\": { \"pin\": 33, \"value\": 0 } }]");
		exec_packet(_on);
		printf("turned on \n");
	}

	printf("time_toggler end \n");

}, &runner, true, NULL, NULL);

// Also need to deprecate;
// Much time i worked about it, but i want to use (server (master)) -> (device (slave)) architecture;
void exec_scheduler()
{
	printf("exec_scheduler start \n");
	get_time();
	if (scheduled_isset && scheduled_time > 0)
	{
		if (__unixtime__ >= scheduled_time)
		{
			printf("Scheduled activation \n");
			printf("Scheduled command %s \n", sched_cmd->valuestring);

			cJSON *cmd = sched_cmd;
			exec_packet(cmd);

			scheduled_clear();
		}
	}

	printf("exec_scheduler start \n");
}

//Task exec_scheduler_task(1000, TASK_FOREVER, exec_scheduler, &runner, true, NULL, NULL);

void scheduled(cJSON *command, int next_time)
{
	printf("scheduled start \n");
	get_time();
	scheduled_time = __unixtime__ + next_time;
	sched_cmd = command;
	scheduled_isset = true;

	printf("Scheduled is set \n");
	printf("Scheduled command %s \n", sched_cmd);
}

void scheduled_clear()
{
	printf("scheduled_clear start \n");
	scheduled_time = 0;
	sched_cmd = cJSON_CreateNull();
	scheduled_isset = false;

	printf("Scheduled is clear \n");
}

#define ON_VAL "on"
#define OFF_VAL "off"

char *parse_value(int value, bool invert)
{
    return ( char* ) ( ( value ^ invert ) ? ON_VAL : OFF_VAL );
}

char *parse_value(int value)
{
    return parse_value(value, false);
}

char *exec_packet(const cJSON *const pack)
{
	cJSON * buf_val;
    char *ret_ = "";
	printf("exec_packet start \n");
    printf("packet length %d \n", cJSON_GetArraySize(pack));

	// USE STRCMP

	if (pack == NULL || cJSON_IsInvalid(pack))
	{
		ESP_LOGE(TAG, "parser malfunction");
		return "parser malfunction";
	}

	if (cJSON_GetArraySize(pack) < 0)
	{
		return "data misfunction";
	}

	for (uint32_t i = 0; i < cJSON_GetArraySize(pack); i++)
	{
		const cJSON *data = cJSON_GetArrayItem(pack, i);
        char *data_type = json_type( (cJSON *) data);
        char *data_tmp = cJSON_Print(data);
		printf("data: %s type: %s\n", data_tmp, data_type);
		//    printf(data["pin"]);
		//    printf(data.hasOwnProperty("schedule"));
		//    printf(data.hasOwnProperty("pin"));

        if ( cJSON_IsString(data) ) {

            printf("    data is string \n");

            if ( strcmp("status", data->valuestring) == 0 ) {
                printf("    data is status \n");
                uint8_t temp_farenheit = temprature_sens_read();
                float temp_celsius = ( temp_farenheit - 32 ) / 1.8;

                buf_val = cJSON_CreateNull();
                buf_val = cJSON_CreateArray();

                int pin1s = 32;
                int val1 = digitalRead(pin1s);
                ret_ = parse_value(val1);
                cJSON *pin1 = cJSON_CreateObject();
                cJSON_AddNumberToObject(pin1, "pin", pin1s);
                cJSON_AddStringToObject(pin1, "value", ret_);
                cJSON_AddItemToArray(buf_val, pin1);

                int pin2s = 33;
                int val2 = digitalRead(pin2s);
                ret_ = parse_value(val2);
                cJSON *pin2 = cJSON_CreateObject();
                cJSON_AddNumberToObject(pin2, "pin", pin2s);
                cJSON_AddStringToObject(pin2, "value", ret_);
                cJSON_AddItemToArray(buf_val, pin2);

                ret_ = cJSON_Print(buf_val);
                continue;
            }

        } else if ( cJSON_IsObject(data) ) {

            printf("    data is object \n");

            if (cJSON_GetObjectItemCaseSensitive(data, (const char *) "schedule") != NULL)
            {
                buf_val = cJSON_GetObjectItem(data, "schedule");
                //printf("schedule \n");

                if (json_type(buf_val) == "string")
                {
                    //printf("is string \n");

                    cJSON * json;
                    json = cJSON_GetObjectItemCaseSensitive(data, "schedule");
                    if ( strcmp("state", json->valuestring) == 0 )
                    {
                        printf("getting state schedule \n");
                        ret_ = (char*)(scheduled_isset ? "scheduled" : "not scheduled");
                        continue;
                    }

                    json = cJSON_GetObjectItemCaseSensitive(data, "schedule");
                    if ( strcmp("clear", json->valuestring) == 0 )
                    {
                        scheduled_clear();
                        ret_ = "ok; scheduler is clear";
                        continue;
                    }
                }
                else if (json_type(buf_val) == "object")
                {
                    if (cJSON_GetObjectItemCaseSensitive(buf_val, "time") != NULL &&
                        cJSON_GetObjectItemCaseSensitive(buf_val, "cmd") != NULL)
                    {
                        int tme = cJSON_GetObjectItem(buf_val, "time")->valueint;
                        cJSON *cmd = cJSON_GetObjectItem(buf_val, "cmd");
                        scheduled(cmd, tme);
                        ret_ = "ok; scheduler is set";
                        continue;
                    }
                    else
                    {
                        return "error; no time or cmd parameter";
                    }
                }
            }

            if (cJSON_GetObjectItemCaseSensitive(data, "digitalWrite") != NULL)
            {
                buf_val = cJSON_GetObjectItem(data, "digitalWrite");
                if (cJSON_GetObjectItemCaseSensitive(buf_val, "pin") != NULL &&
                    cJSON_GetObjectItemCaseSensitive(buf_val, "value") != NULL)
                {
                    int pin = cJSON_GetObjectItem(buf_val, "pin")->valueint;
                    int val = cJSON_GetObjectItem(buf_val, "value")->valueint;
                    digitalWrite(pin, val);

                    printf("digitalWrite \n");
                    ret_ = (char*)(val ? "off" : "on");

                    buf_val = cJSON_CreateNull();
                    buf_val = cJSON_CreateObject();
                    cJSON_AddNumberToObject(buf_val, "pin", pin);
                    cJSON_AddStringToObject(buf_val, "value", ret_);

                    ret_ = cJSON_Print(buf_val);

                    continue;
                }
            }

            if (cJSON_GetObjectItemCaseSensitive(data, "digitalRead") != NULL)
            {
                buf_val = cJSON_GetObjectItem(data, "digitalRead");
                if (cJSON_GetObjectItemCaseSensitive(buf_val, "pin") != NULL)
                {
                    int pin = cJSON_GetObjectItem(buf_val, "pin")->valueint;
                    int val = digitalRead(pin);
                    ret_ = (char*)(val ? "off" : "on");
                    //strcpy(ret_, (char*) "");
                    printf("digitalRead \n");

                    buf_val = cJSON_CreateNull();
                    buf_val = cJSON_CreateObject();
                    cJSON_AddNumberToObject(buf_val, "pin", pin);
                    cJSON_AddStringToObject(buf_val, "value", ret_);

                    ret_ = cJSON_Print(buf_val);

                    continue;
                }
            }
        }
	}

	printf("exec_packet end \n");
	return ret_;
}