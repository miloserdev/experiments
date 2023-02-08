#include <stdint.h>
#include "sdkconfig.h"
//#include "RTCDS1307.h"
#include <stdio.h>
#include <driver/gpio.h>
#include <esp_err.h>
#include <esp_system.h>
#include <esp_event.h>
#include <esp_event_loop.h>
//#include <esp_event_legacy.h>
#include <esp_intr_alloc.h>
#include <esp_event_base.h>
#include <esp_event.h>

//#include "esp_err.h"
//#include "esp_wifi.h"
//#include "esp_wifi_internal.h"
//#include "esp_event.h"
//#include "esp_event_loop.h"
#include "esp_task.h"
#include "esp_eth.h"
#include "esp_system.h"

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "rom/ets_sys.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"

#include "lwip/err.h"
#include "lwip/sys.h"

//#include "tcpip_adapter.h"

static const char *TAG = "relay_array_1";
#include <esp_wifi.h>
#include <esp_wifi_types.h>
//#include <tcpip_adapter.h>
#define WIFI_SSID "Keenetic-6193"
#define WIFI_PASSWORD "DNj6KdZT"
#include <esp_http_client.h>
#include <esp_http_server.h>
#define PORT 8081
#include <nvs.h>
#include <nvs_flash.h>
#include <esp_partition.h>
#include <esp_ota_ops.h>   	// get running partition
#define OTA_BUF_SIZE 1024
#include <esp_sntp.h>
#define SNTP_SERVER "pool.ntp.org"
// #define SNTP_SYNC_INTERVAL ((60 * 60) * 1000) * 12	// 12 hours;
#define SNTP_SYNC_INTERVAL (((60 * 60) * 1000) *24) * 7	// 7 days;
#include "cJSON.h"

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




//RTC_DS1307 RTC;
//DateTime _now;
//#include <TaskScheduler.h>

//Scheduler runner;





char *exec_packet(cJSON *pack);
//char *exec_packet(cJSON *pack);
uint32_t get_time();
int settings_init();
esp_err_t wifi_init();
esp_err_t wifi_stops();
void scheduled_clear();

#define ON_VAL "on"
#define OFF_VAL "off"

char * parse_value(int value, bool invert);
uint32_t parse_lohi(char * lohi);
int parse_type(char * type);

httpd_handle_t server = NULL;
static esp_netif_t *esp_netif_this = NULL;
bool _connected = false;

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



const char *gateway_addr = "http://192.168.1.69:8884/";

static const char *settings =
"["
"{\"id\":\"relay_1\",\"pin\":32,\"type\":\"ANALOG\",\"pinmode\":\"OUTPUT\",\"defval\":\"HIGH\",\"invert\":true},"
"{\"id\":\"relay_2\",\"pin\":33,\"type\":\"ANALOG\",\"pinmode\":\"OUTPUT\",\"defval\":\"HIGH\",\"invert\":true},"
"{\"id\":\"relay_3\",\"pin\":25,\"type\":\"ANALOG\",\"pinmode\":\"OUTPUT\",\"defval\":\"HIGH\",\"invert\":true},"
"{\"id\":\"relay_4\",\"pin\":26,\"type\":\"ANALOG\",\"pinmode\":\"OUTPUT\",\"defval\":\"HIGH\",\"invert\":true}"
"]";




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

	// NOT USING NOW;

	__unixtime__ = 10000;
    return __unixtime__;

    /* FIX TIME
	_now = RTC.now();

	year = _now.year();
	month = _now.month();
	weekday = _now.dayOfTheWeek();
	day = _now.day();
	hour = _now.hour();
	minute = _now.minute();
	second = _now.second();
	__unixtime__ = _now.unixtime();

	printf("time -> %lu \n", __unixtime__);
	return __unixtime__;
	*/
}

//Task get_time_task(1000, TASK_FOREVER, []() { 	get_time(); }, &runner, true, NULL, NULL);

void sntp_update_rtc(struct timeval *tv)
{
	printf("SNTP RTC update event \n");

	/*
		// maybe
		__unixtime__ = tv->tv_sec;
		// to run without RTC
	*/

	//struct DateTime dt = DateTime(tv->tv_sec);
	//RTC.adjust(dt);
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

}

/* FIX
Task get_sntp_task(SNTP_SYNC_INTERVAL, TASK_FOREVER, []()
{
	update_time_sntp();
}, &runner, true, NULL, NULL);
*/

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




char *parse_value(int value, bool invert)
{
    return ( char* ) ( ( value ^ invert ) ? ON_VAL : OFF_VAL );
}

uint32_t parse_lohi (char *lohi)
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

gpio_mode_t parse_pinmode (char *pinmode)
{
    if ( strcmp("INPUT", pinmode) == 0 ) return GPIO_MODE_INPUT;
    else if ( strcmp("OUTPUT", pinmode) == 0 ) return GPIO_MODE_OUTPUT;
    //else if ( strcmp("PULLUP", pinmode) == 0 ) return 0x04;
    //else if ( strcmp("INPUT_PULLUP", pinmode) == 0 ) return 0x05;
    //else if ( strcmp("PULLDOWN", pinmode) == 0 ) return 0x08;
    //else if ( strcmp("INPUT_PULLDOWN", pinmode) == 0 ) return 0x09;
    else if ( strcmp("OPEN_DRAIN", pinmode) == 0 ) return GPIO_MODE_INPUT_OUTPUT_OD;
    else if ( strcmp("OUTPUT_OPEN_DRAIN", pinmode) == 0 ) return GPIO_MODE_OUTPUT_OD;
    else if ( strcmp("SPECIAL", pinmode) == 0 ) return GPIO_MODE_INPUT_OUTPUT; //IO
    //else if ( strcmp("FUNCTION_1", pinmode) == 0 ) return 0x00;
    //else if ( strcmp("FUNCTION_2", pinmode) == 0 ) return 0x20;
    //else if ( strcmp("FUNCTION_3", pinmode) == 0 ) return 0x40;
    //else if ( strcmp("FUNCTION_4", pinmode) == 0 ) return 0x60;
    //else if ( strcmp("FUNCTION_5", pinmode) == 0 ) return 0x70;
    //else if ( strcmp("FUNCTION_6", pinmode) == 0 ) return 0xA0;
    //else if ( strcmp("ANALOG", pinmode) == 0 ) return 0xC0;
    //else return 0xDEAD;
    return GPIO_MODE_DISABLE;
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

		//printf("id_: %s pin_: %lu type: %lu pinmode_: %i defval: %i\n", id_, pin_, parse_type(type_), parse_pinmode(pinmode_), parse_lohi(defval_));
        if (pin_ && pinmode_)
        {
            gpio_config_t config = {
                .pin_bit_mask = BIT64(pin_),
                .mode = parse_pinmode(pinmode_)
				//.pull_up_en = GPIO_PULLUP_ENABLE; // резистор на питание
				//.pull_down_en = GPIO_PULLDOWN_DISABLE; // без резистора на землю
				//.intr_type = GPIO_PIN_INTR_DISABLE;  // прерывание запрещено
                //.pull_up_en= 
                //.pull_down_en= 
                //.intr_type=
            };
			printf("gpio set config \n");
            esp_err_t err = gpio_config( &config );
			//free(config);
         //   pinMode(pin_, parse_pinmode(pinmode_));
        }
        if (pin_ && defval_)
        {
			printf("gpio set level \n");
            gpio_set_level( pin_, parse_lohi(defval_) );
            //digitalWrite(pin_, parse_lohi(defval_));
        }
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

    }
    printf("settings_init end \n");
    return -1;
}





#define MAX_HTTP_RECV_BUFFER 512
#define MAX_HTTP_OUTPUT_BUFFER 2048

// from esp-idf examples
esp_err_t _http_event_handler(esp_http_client_event_t *evt)
{
    static char *output_buffer;  // Buffer to store response of http request from event handler
    static int output_len;       // Stores number of bytes read
    switch(evt->event_id) {
        case HTTP_EVENT_REDIRECT:
            printf("HTTP_EVENT_REDIRECT\n");
            break;
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
                            printf("Failed to allocate memory for output buffer");
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
            printf("HTTP_EVENT_ON_FINISH\n");
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

/*
void http_client_init () {

    if (!_connected) return;

    printf("http_client_init begin \n");
    esp_http_client_config_t config =
    {
        .host = gateway_addr,
        .path = "/" };
    printf("esp_http_client_config_t init \n");

    // because: sorry, unimplemented: non-trivial designated initializers not supported
    // idk how to fix;

    esp_http_client_handle_t client = esp_http_client_init(&config);
    printf("esp_http_client_handle_t init \n");

    // POST
    const char *post_data = "{\"field1\":\"value1\"}";
    //esp_http_client_set_url(client, "http://httpbin.org/post");
    //esp_http_client_set_method(client, HTTP_METHOD_POST);
    //printf("esp_http_client_set_method init \n");
    esp_http_client_set_header(client, "Content-Type", "application/json");
    printf("esp_http_client_set_header init \n");
    esp_http_client_set_post_field(client, post_data, strlen(post_data));
    printf("esp_http_client_set_post_field init \n");
    esp_err_t err = esp_http_client_perform(client);
    printf("esp_http_client_perform init \n");
    if (err == ESP_OK) {
        printf("HTTP POST Status = %d, content_length = %"PRIu64,
                esp_http_client_get_status_code(client),
                esp_http_client_get_content_length(client));
    } else {
        ESP_LOGE(TAG, "HTTP POST request failed: %s", esp_err_to_name(err));
    }
    printf("http_client_init end \n");
}


Task http_client_init_task(5000, TASK_FOREVER, http_client_init, &runner, true, NULL, NULL);

*/



void open_nvs(nvs_handle* handle)
{
   // Open nvs
  printf ("\n");
  printf ("Opening Non-Volatile Storage (NVS) handle... ");

  esp_err_t ret = nvs_open ("storage", NVS_READWRITE, handle);

  if (ret != ESP_OK)
  {
    printf ("Error (%s) opening NVS handle!\n", esp_err_to_name (ret));
  }
  else
  {
    printf ("Done\n");
  }

}

void close_nvs(nvs_handle* handle)
{
  printf("\n");
  printf("Closing Non-Volatile Storage (NVS) handle...");

  nvs_close (*handle);

  printf ("Done\n");
}

void nvs_handler_init()
{
/*
	nvs_handle handle_nvs;

	esp_err_t ret = nvs_flash_init ();
	if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
	{
		// NVS partition was truncated and needs to be erased
		// Retry nvs_flash_init
		ESP_ERROR_CHECK(nvs_flash_erase ());
		ret = nvs_flash_init ();
	}
	ESP_ERROR_CHECK(ret);
*/
}


void http_fetch()
{
/*
    esp_http_client_config_t config = {
        .url = "http://httpbin.org/redirect/2",
        .event_handler = _http_event_handler,
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_err_t err = esp_http_client_perform(client);

    if (err == ESP_OK) {
    printf("Status = %d, content_length = %lld",
            esp_http_client_get_status_code(client),
            esp_http_client_get_content_length(client));
    }
    esp_http_client_cleanup(client);
*/
}


void app_main()
{
	//Serial.begin(115200);
	//    rtc.begin();
	//Wire.begin();
	//RTC.begin();

    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());



	get_time();

    settings_init();

	wifi_init();

    //runner.execute();
}
/*
                                    //  system_event_t
                                    // esp_event_base_t
                                    // FIX THIS SHIT
//esp_err_t wifi_event_handler(void *ctx, system_event_t *event)
esp_event_handler_t wifi_event_handler(void* handler_arg, esp_event_base_t base, int32_t id, void* event_data)
{
	printf("wifi_event_handler -> \n");
	if (event_data != NULL)
	{
		switch (id)
		{
            case HTTP_EVENT_ON_HEADER:
            {
                printf("HTTP_EVENT_ON_HEADER event\n");
                break;
            }
            case HTTP_EVENT_ON_CONNECTED:
            {
                printf("HTTP_EVENT_ON_CONNECTED event\n");
                break;
            }
			//case SYSTEM_EVENT_STA_START:
            case WIFI_EVENT_STA_START:
				{
					if (esp_wifi_connect() != ESP_OK)
					{
						printf("Cannot connect to WiFi AP \n");
						stop_webserver(server);
                        _connected = false;
					}

					//esp_err_to_name_r(esp_err_t code, char *buf, size_t buflen)
					break;
				}

			//case SYSTEM_EVENT_STA_GOT_IP:
            case IP_EVENT_STA_GOT_IP: // || ESP_NETIF_IP_EVENT_GOT_IP
				{
					ip_event_got_ip_t *event = (ip_event_got_ip_t *) event_data;
					printf("IPv4-> %s \n", ip4addr_ntoa(&event->ip_info.ip));
					//printf("IPv6-> %s \n", ip6addr_ntoa(&event->ip6_info.ip));
					server = start_webserver();
                    _connected = true;
					break;
				}

			//case SYSTEM_EVENT_STA_DISCONNECTED:
            case WIFI_EVENT_STA_DISCONNECTED:
				{
					if (esp_wifi_connect() != ESP_OK)
					{
						printf("Cannot connect to WiFi AP \n");
						stop_webserver(server);
                        _connected = false;
					}

					break;
				}
		}
	}
    return ESP_OK;
}
*/


static void disconnect_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
	if (esp_wifi_connect() != ESP_OK)
	{
		printf("Cannot connect to WiFi AP \n");
		stop_webserver(server);
		_connected = false;
	}
    printf("WiFi disconnected \n");
}

static void connect_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
	ip_event_got_ip_t *event = (ip_event_got_ip_t *) event_data;
	printf("IPv4-> %s \n", ip4addr_ntoa(&event->ip_info.ip));
	//printf("IPv6-> %s \n", ip6addr_ntoa(&event->ip6_info.ip));
	server = start_webserver();
	_connected = true;
    printf("WiFi connection success \n");
}


esp_err_t wifi_stops()
{
	printf("wifi_stops start \n");
	esp_event_handler_unregister(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &disconnect_handler);
	esp_event_handler_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, &connect_handler);
	
    esp_err_t err = esp_wifi_stop();
    if (err == ESP_ERR_WIFI_NOT_INIT) {
        return ESP_FAIL;
    }

	esp_wifi_deinit();
	esp_wifi_clear_default_wifi_driver_and_handlers(esp_netif_this);
    esp_netif_destroy(esp_netif_this);
    esp_netif_this = NULL;
	printf("wifi_stops end \n");
	return ESP_OK;
}

esp_err_t wifi_init()
{
	printf("wifi_init start \n");

    //ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &connect_handler, &server));
    //ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &disconnect_handler, &server));

    //  esp_event_loop_init
	//if (esp_event_loop_run(wifi_event_handler, NULL) != ESP_OK)


	wifi_init_config_t wifi_init_config = WIFI_INIT_CONFIG_DEFAULT();
	if (esp_wifi_init(&wifi_init_config) != ESP_OK)
	{
		printf("Cannot init WiFi config \n");
	}


	esp_netif_config_t netif_config = ESP_NETIF_DEFAULT_WIFI_STA();
	esp_netif_t *netif = esp_netif_new(&netif_config);
	assert(netif);
    esp_netif_attach_wifi_station(netif);
    esp_wifi_set_default_wifi_sta_handlers();

	if ( esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &connect_handler, &server) != NULL)
	{
		printf("Cannot init WiFi connect event \n");
	}
	
	if ( esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &disconnect_handler, &server) != NULL)
	{
		printf("Cannot init WiFi disconnect handler \n");
	}

	//esp_wifi_set_storage(WIFI_STORAGE_RAM);
	wifi_storage_t wifi_storage = WIFI_STORAGE_FLASH;
	if (esp_wifi_set_storage(wifi_storage) != ESP_OK)
	{
		printf("Cannot set WiFi storage \n");
	}

	esp_netif_this = netif;

	wifi_interface_t wifi_interface = WIFI_IF_STA;
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASSWORD,
            //bssid_set = 0
        },
    };

	wifi_mode_t wifi_mode = WIFI_MODE_STA;
	if (esp_wifi_set_mode(wifi_mode) != ESP_OK)
	{
		printf("Cannot set WiFi mode \n");
	}

	if (esp_wifi_set_config(wifi_interface, &wifi_config) != ESP_OK)
	{
		printf("Cannot set WiFi config \n");
	}

	if (esp_wifi_start() != ESP_OK)
	{
		printf("Cannot start WiFi \n");
	}

	esp_wifi_connect();

	esp_register_shutdown_handler(&wifi_stops);
	
	printf("wifi_init end \n");
	return ESP_OK;
}

/// WARNING
/// HARDCODE ;)
/*
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
*/

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
	printf("Scheduled command %s \n", sched_cmd->valuestring);
}

void scheduled_clear()
{
	printf("scheduled_clear start \n");
	scheduled_time = 0;
	sched_cmd = cJSON_CreateNull();
	scheduled_isset = false;

	printf("Scheduled is clear \n");
}


char *exec_packet(cJSON *pack)
{
	cJSON * buf_val;
    char *ret_ = "";
	printf("exec_packet start \n");
    printf("packet length %d \n", cJSON_GetArraySize(pack));

	// USE STRCMP

	if (pack == NULL || cJSON_IsInvalid(pack))
	{
		printf("parser malfunction");
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
                // uint8_t temp_farenheit = temprature_sens_read();
                // float temp_celsius = ( temp_farenheit - 32 ) / 1.8;
				// ^ need to include to status packet;

                buf_val = cJSON_CreateNull();
                buf_val = cJSON_CreateArray();

                int pin1s = 32;
                int val1 = gpio_get_level(pin1s);
                ret_ = parse_value(val1, false);
                cJSON *pin1 = cJSON_CreateObject();
                cJSON_AddNumberToObject(pin1, "pin", pin1s);
                cJSON_AddStringToObject(pin1, "value", ret_);
                cJSON_AddItemToArray(buf_val, pin1);

                int pin2s = 33;
                int val2 = gpio_get_level(pin2s);
                ret_ = parse_value(val2, false);
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

                if ( cJSON_IsString(buf_val) )
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
                else if ( cJSON_IsObject(buf_val) )
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
                    gpio_set_level(pin, val);
                    //digitalWrite(pin, val);
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
                    int val = gpio_get_level(pin);
                    //int val = digitalRead(pin);
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