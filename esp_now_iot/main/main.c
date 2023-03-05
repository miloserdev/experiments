/* ESPNOW Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

/*
   This example shows how to use ESPNOW.
   Prepare two device, one for sending ESPNOW data and another for receiving
   ESPNOW data.
*/
#include "sdkconfig.h"

#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <assert.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/timers.h"
#include "nvs_flash.h"
#include "esp_event_loop.h"
#include "tcpip_adapter.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_now.h"
#include "rom/ets_sys.h"
#include "rom/crc.h"
#include "espnow_example.h"

#include "driver/uart.h"


#include <stdio.h>
#include <string.h>
#include "esp_system.h"
#include "esp_log.h"
#include "esp_console.h"
#include "FreeRTOS.h"
#include "driver/uart.h"
#include "linenoise/linenoise.h"
#include "argtable3/argtable3.h"
#include "nvs.h"
#include "nvs_flash.h"

#include "driver/gpio.h"

#include "esp_err.h"
#include "esp_vfs_dev.h"

#include "uart_init.c"

//#include "/home/max/workspace/to_git/experiments/esp_now_iot/include/eJSON.h"

//#include "../include/eJSON.h"
#include "cJSON.h"

//#include "lwip/tcp.h"
#include "esp_http_server.h"













httpd_handle_t server = NULL;

static const char *TAG = "espnow_example";

uint8_t my_mac[ESP_NOW_ETH_ALEN];
static const uint8_t example_broadcast_mac[ESP_NOW_ETH_ALEN] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
static uint16_t s_example_espnow_seq[EXAMPLE_ESPNOW_DATA_MAX] = { 0, 0 };

#define MIN(a, b)(((a) < (b)) ? (a) : (b))
#define MAX(a, b)(((a) > (b)) ? (a) : (b))




#define MESH_PASSWD             "DNj6KdZT"
#define MESH_PASSWD_LEN         ets_strlen(MESH_PASSWD)
#define MESH_MAX_HOP            (4)
#define MESH_SSID_PREFIX        "Keenetic-6193"
#define MESH_SSID_PREFIX_LEN    ets_strlen(MESH_SSID_PREFIX)
#define MESH_CHANNEL            0 // CONFIG_ESPNOW_CHANNEL

#define PORT        8066

bool _connected = false;


char *exec_packet(cJSON *pack);
httpd_handle_t start_webserver(void);
void stop_webserver();




typedef enum {
    MSX_ESP_NOW_SEND_CB = 99,
    MSX_ESP_NOW_RECV_CB,

    MSX_WIFI_EVENT_STA_START,
    MSX_WIFI_EVENT_STA_DISCONNECTED,
    MSX_IP_EVENT_STA_GOT_IP,
} msx_event_id_t;

typedef struct {

} msx_event_data_t;

typedef struct {
    //msx_event_id_t id;
    //msx_event_data_t data;
    //uint8_t *from;
    esp_event_base_t base;
    int32_t id;
    void *data;
} msx_event_t;

static xQueueHandle event_loop_queue;





uint8_t pingmsg[128];


#define ON_VAL "on"
#define OFF_VAL "off"

char *parse_value(int value, bool invert)
{
    return ( char* ) ( ( value ^ invert ) ? ON_VAL : OFF_VAL );
}





#define EXAMPLE_ESP_WIFI_SSID      CONFIG_ESP_WIFI_SSID
#define EXAMPLE_ESP_WIFI_PASS      CONFIG_ESP_WIFI_PASSWORD
#define EXAMPLE_ESP_MAXIMUM_RETRY  10
static int s_retry_num = 0;

static void event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    msx_event_t *evt = (msx_event_t *) malloc( sizeof(  msx_event_t ) );
    evt->base = event_base;
    evt->id = event_id;
    evt->data = event_data;
    os_printf("______ event_handler ______%d_\n", ( xQueueSend(event_loop_queue, evt, portMAX_DELAY) != pdTRUE ) );
    return;

/*     if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {
        evt->id = MSX_WIFI_EVENT_STA_START;
        os_printf("______ event_handler ______%d_\n", ( xQueueSend(event_loop_queue, evt, portMAX_DELAY) != pdTRUE ) );

        esp_wifi_connect();
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        evt->id = MSX_WIFI_EVENT_STA_DISCONNECTED;
        os_printf("______ event_handler ______%d_\n", ( xQueueSend(event_loop_queue, evt, portMAX_DELAY) != pdTRUE ) );

        if (s_retry_num < EXAMPLE_ESP_MAXIMUM_RETRY)
        {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "retry to connect to the AP");
        }
        else
        {
            // xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
            os_printf("event_handler >> WIFI_FAIL_BIT \n");
        }
        ESP_LOGI(TAG, "connect to the AP fail");
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        evt->id = MSX_IP_EVENT_STA_GOT_IP;

        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;

        evt->data = event_data;
        //memcpy(evt.data, event_data, sizeof(event_data));

        ESP_LOGI(TAG, "got ip:%s", ip4addr_ntoa(&event->ip_info.ip));
        s_retry_num = 0;
        // xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
        os_printf("event_handler >> WIFI_CONNECTED_BIT \n");

        os_printf("______ event_handler ______%d_\n", ( xQueueSend(event_loop_queue, evt, portMAX_DELAY) != pdTRUE ) );
    } */
}




/* esp_err_t wifi_init2()
{
    tcpip_adapter_init();

	os_printf("wifi_init start \n");

    esp_event_loop_create_default();

	wifi_init_config_t wifi_init_config = WIFI_INIT_CONFIG_DEFAULT();
	if (esp_wifi_init(&wifi_init_config) != ESP_OK)
	{
		os_printf("Cannot init WiFi config \n");
	}

    //esp_wifi_set_default_wifi_sta_handlers()
    

	if ( esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL) != NULL)
	{
		os_printf("Cannot init WiFi connect event \n");
	}
	
	if ( esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL) != NULL)
	{
		os_printf("Cannot init WiFi disconnect handler \n");
	}

	//esp_wifi_set_storage(WIFI_STORAGE_RAM);
	wifi_storage_t wifi_storage = WIFI_STORAGE_FLASH;
	if (esp_wifi_set_storage(wifi_storage) != ESP_OK)
	{
		os_printf("Cannot set WiFi storage \n");
	}

	wifi_interface_t wifi_interface = WIFI_MODE_STA;
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = MESH_SSID_PREFIX,
            .password = MESH_PASSWD,
            //.channel = MESH_CHANNEL
            //bssid_set = 0
        },
    };

	wifi_mode_t wifi_mode = WIFI_MODE_STA;
	if (esp_wifi_set_mode(wifi_mode) != ESP_OK)
	{
		os_printf("Cannot set WiFi mode \n");
	}

	if (esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config) != ESP_OK)
	{
		os_printf("Cannot set WiFi config \n");
	}

	if (esp_wifi_start() != ESP_OK)
	{
		os_printf("Cannot start WiFi \n");
	}

	esp_wifi_connect();

    os_printf("______ esp_wifi_set_channel ______%d_\n", esp_wifi_set_channel(MESH_CHANNEL, 0)) ;

	//esp_register_shutdown_handler(&wifi_stops);
	
	os_printf("wifi_init end \n");
	return ESP_OK;
} */


static void wifi_init(void)
{
    tcpip_adapter_init();
    os_printf("______ tcpip_adapter_init ______%d_\n", 0 );

    os_printf("______ esp_event_loop_create_default ______%d_\n", esp_event_loop_create_default() );


    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    //wifi_sta_config_t sta = WIFI_INIT_CONFIG_DEFAULT();
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = MESH_SSID_PREFIX,
            .password = MESH_PASSWD,
            .channel = MESH_CHANNEL
        },
    };
    os_printf("______ esp_wifi_init ______%d_\n", esp_wifi_init(&cfg) );

    os_printf("______ esp_event_handler_register WIFI_EVENT ______%d_\n", esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL) );
    os_printf("______ esp_event_handler_register IP_EVENT ______%d_\n", esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL) );

    os_printf("______ esp_wifi_set_storage ______%d_\n", esp_wifi_set_storage(WIFI_STORAGE_RAM) );
    os_printf("______ esp_wifi_set_mode ______%d_\n", esp_wifi_set_mode(WIFI_MODE_APSTA) );
    os_printf("______ esp_wifi_set_config ______%d_\n", esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config) );
    os_printf("______ esp_wifi_start ______%d_\n", esp_wifi_start() );
    os_printf("______ esp_wifi_set_channel ______%d_\n", esp_wifi_set_channel(MESH_CHANNEL, 0)) ;
    os_printf("______ esp_wifi_connect ______%d_\n", esp_wifi_connect() );


     os_printf("______ esp_wifi_get_mac ______%d_\n", esp_wifi_get_mac(ESP_IF_WIFI_STA, my_mac) );

    os_printf("______ esp_wifi_set_ps ______%d_\n", esp_wifi_set_ps(WIFI_PS_NONE) );

    os_free(&cfg);
    os_free(&wifi_config);
}











static void example_espnow_send_cb(const uint8_t *mac_addr, esp_now_send_status_t status)
{
    example_espnow_event_send_cb_t *send = /*may cause crash*/ os_malloc(sizeof(example_espnow_event_send_cb_t));

    if (mac_addr == NULL) {
        os_printf("example_espnow_send_cb >> error >> status: %d \n", status);
        return;
    }

    memcpy(send->mac_addr, mac_addr, ESP_NOW_ETH_ALEN);
    send->status = status;

    os_printf("example_espnow_send_cb >> mac: "MACSTR" status: %d \n", MAC2STR(send->mac_addr), send->status);

    os_free(send->mac_addr);
    os_free(send);

    os_free(&mac_addr);
}


static void example_espnow_recv_cb(const uint8_t *mac_addr, const uint8_t *data, int len)
{
    example_espnow_event_recv_cb_t *recv = /*may cause crash*/ os_malloc(sizeof(example_espnow_event_recv_cb_t));

    if (mac_addr == NULL || data == NULL || len <= 0) {
        ESP_LOGE(TAG, "Receive cb arg error");
        return;
    }

    memcpy(recv->mac_addr, mac_addr, ESP_NOW_ETH_ALEN);
    recv->data = /*may cause crash*/ os_malloc(len);
    memcpy(recv->data, data, len);
    recv->data_len = len;

    os_printf("example_espnow_recv_cb >> mac: "MACSTR" len: %d data: [", MAC2STR(recv->mac_addr), recv->data_len);

    char *datas = /*may cause crash*/ os_malloc(len);
    for (int i = 0; i < recv->data_len - 1; i++)
    {
        datas[i] = recv->data[i];
        os_printf("%c", recv->data[i]);
    }
    os_printf("] \n");
    
    cJSON *pack = cJSON_Parse(datas);
    if ( cJSON_IsInvalid(pack) )
    {
        os_printf("example_espnow_recv_cb >> json receive malfunction \n");
        return;
    }
    char *exec_data = exec_packet(pack);


    if (esp_now_is_peer_exist(mac_addr) == false) {
        esp_now_peer_info_t *peer = /*may cause crash*/ os_malloc(sizeof(esp_now_peer_info_t));
        if (peer == NULL) {
            os_printf("Malloc peer information fail \n");
        }
        memset(peer, 0, sizeof(esp_now_peer_info_t));
        peer->channel = MESH_CHANNEL;
        peer->ifidx = ESPNOW_WIFI_IF;
        peer->encrypt = true;
        memcpy(peer->lmk, CONFIG_ESPNOW_LMK, ESP_NOW_KEY_LEN);
        memcpy(peer->peer_addr, mac_addr, ESP_NOW_ETH_ALEN);
        ESP_ERROR_CHECK( esp_now_add_peer(peer) );

        os_free(peer);
    }

    os_free(exec_data);
    cJSON_Delete(pack);
    cJSON_free(pack);
    os_free(pack); // may cause segfault;

    os_free(datas);
    os_free(recv->mac_addr);
    os_free(recv->data);
    os_free(recv);

    os_free(&mac_addr);
    os_free(&data);
}

















void send_packet(const uint8_t *peer_addr, const uint8_t *strs, size_t len)
{
    example_espnow_send_param_t *data = /*may cause crash*/ os_malloc(sizeof(example_espnow_send_param_t));
    memset(data, 0, sizeof(example_espnow_send_param_t));
    
    data->unicast = false;
    data->broadcast = true;
    data->state = 0;
    data->magic = esp_random();
    data->count = CONFIG_ESPNOW_SEND_COUNT;
    data->delay = CONFIG_ESPNOW_SEND_DELAY;
    data->len = CONFIG_ESPNOW_SEND_LEN;
    data->buffer = /*may cause crash*/ os_malloc(CONFIG_ESPNOW_SEND_LEN);

    memset(data->buffer, 0, CONFIG_ESPNOW_SEND_LEN);
    memcpy(data->buffer, strs, len);
    memcpy(data->dest_mac, peer_addr, ESP_NOW_ETH_ALEN);

    if (esp_now_send(data->dest_mac, data->buffer, data->len) != ESP_OK) {
    //if (esp_now_send(example_broadcast_mac, strs, sizeof(strs)) != ESP_OK) {
        os_printf("Send error \n");
    }

    os_free(data->buffer);
    os_free(data->dest_mac);
    os_free(data);
}














static esp_err_t example_espnow_init(void)
{

    uint8_t asd[] = "fuck this boards at all!!!";
    memset(pingmsg, 0, sizeof(pingmsg));
    memcpy(pingmsg, asd, sizeof(asd));

    example_espnow_send_param_t *send_param;

    /* Initialize ESPNOW and register sending and receiving callback function. */
    ESP_ERROR_CHECK( esp_now_init() );
    ESP_ERROR_CHECK( esp_now_register_send_cb(example_espnow_send_cb) );
    ESP_ERROR_CHECK( esp_now_register_recv_cb(example_espnow_recv_cb) );

    /* Set primary master key. */
    ESP_ERROR_CHECK( esp_now_set_pmk((uint8_t *)CONFIG_ESPNOW_PMK) );

    /* Add broadcast peer information to peer list. */
    esp_now_peer_info_t *peer = /*may cause crash*/ os_malloc(sizeof(esp_now_peer_info_t));
    if (peer == NULL) {
        os_printf("Malloc peer information fail \n");
        return ESP_FAIL;
    }
    memset(peer, 0, sizeof(esp_now_peer_info_t));
    peer->channel = MESH_CHANNEL;
    peer->ifidx = ESPNOW_WIFI_IF;
    peer->encrypt = false;
    memcpy(peer->peer_addr, example_broadcast_mac, ESP_NOW_ETH_ALEN);
    ESP_ERROR_CHECK( esp_now_add_peer(peer) );

    os_free(peer);

    return ESP_OK;
}














cJSON * read_pin(int pin)
{
    char pin_name[10] = "0";
    sprintf(pin_name, "%d", pin);

    int val1 = gpio_get_level(pin);
    char *ret_ = parse_value(val1, false);
    cJSON *pin1 = cJSON_CreateObject();
    cJSON_AddStringToObject(pin1, "pin", pin_name);
    cJSON_AddStringToObject(pin1, "value", ret_);
    return pin1;
}


#define HTTP_BUF_SIZE   256
char http_buf[HTTP_BUF_SIZE];

esp_err_t get_handler(httpd_req_t *req)
{
    cJSON *root = cJSON_CreateArray();

    cJSON_AddItemToArray(root, read_pin(0));
    cJSON_AddItemToArray(root, read_pin(1));
    cJSON_AddItemToArray(root, read_pin(2));

    char *string = cJSON_Print(root);

    os_printf("string: %s\n", string);

	httpd_resp_send(req, string, strlen(string));

    os_free(root);
    os_free(string);
    //cJSON_Delete(obj);

	os_printf("get_handler end \n");

    // do it next day
    return ESP_OK;
}

httpd_uri_t uri_get = { .uri = "/status",
	.method = HTTP_GET,
	.handler = &get_handler,
	.user_ctx = NULL
};



esp_err_t post_handler(httpd_req_t *req)
{
    cJSON *buffer = NULL;
	os_printf("post_handler start \n");
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

	os_printf("buffer: %.*s \n", req->content_len, content);
	buffer = cJSON_Parse((const char *) content);
	const char *resp = (const char *) exec_packet(buffer);

	httpd_resp_send(req, resp, sizeof(resp));

	os_printf("post_handler end \n");

    os_free(resp);
    os_free(buffer);
    os_free(&content);

    os_free(&req);

	return ESP_OK;
}
httpd_uri_t uri_post = { .uri = "/",
	.method = HTTP_POST,
	.handler = &post_handler,
	.user_ctx = NULL
};


httpd_handle_t start_webserver(void)
{
	os_printf("Starting web server \n");
	httpd_config_t config = HTTPD_DEFAULT_CONFIG();
	config.server_port = PORT;

	server = NULL;

	if (httpd_start(&server, &config) == ESP_OK)
	{
		httpd_register_uri_handler(server, &uri_get);
		httpd_register_uri_handler(server, &uri_post);
		os_printf("All handlers in register \n");
	}

	return server;
}

void stop_webserver(httpd_handle_t server)
{
	os_printf("stop_webserver start \n");
	if (server)
	{
		httpd_stop(server);
	}

	os_printf("stop_webserver end \n");
}


























int macaddr_parse(const char *str, uint8_t *out)
{
	int res = sscanf(str, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
			 &out[0], &out[1], &out[2], &out[3], &out[4], &out[5]);

	return res == 6 ? 0 : -1;
}

char *exec_packet(cJSON *pack)
{
	cJSON *buf_val = NULL;
    char *ret_ = "";
    int arr_sz = cJSON_GetArraySize(pack);
	os_printf("exec_packet start \n");
    os_printf("packet length %d \n", arr_sz);

    bool to_me = true;


	// USE STRCMP

	if (pack == NULL || cJSON_IsInvalid(pack))
	{
		os_printf("parser malfunction \n");
		return "parser malfunction";
	}

	if (cJSON_GetArraySize(pack) < 0)
	{
		return "data misfunction";
	}

	for (uint32_t i = 0; i < cJSON_GetArraySize(pack); i++)
	{
        os_printf("using pack %d \n", i);
        const cJSON *data = cJSON_GetArrayItem(pack, i);
        // char *data_type = json_type( (cJSON *) data);
        // char *data_tmp = cJSON_Print(data);
        // os_printf("data: %s \n", data_tmp);
        // printf("data: %s type: %s\n", data_tmp, data_type);

        //    printf(data["pin"]);
        //    printf(data.hasOwnProperty("schedule"));
        //    printf(data.hasOwnProperty("pin"));

        if (cJSON_IsString(data))
        {

            os_printf("    data is string \n");

            if (strcmp("status", data->valuestring) == 0)
            {
                os_printf("    data is status \n");
                // uint8_t temp_farenheit = temprature_sens_read();
                // float temp_celsius = ( temp_farenheit - 32 ) / 1.8;
                // ^ need to include to status packet;

                buf_val = cJSON_CreateNull();
                buf_val = cJSON_CreateArray();

                cJSON_AddItemToArray(buf_val, read_pin(0));
                cJSON_AddItemToArray(buf_val, read_pin(1));
                cJSON_AddItemToArray(buf_val, read_pin(2));

                ret_ = cJSON_Print(buf_val);
                cJSON_Delete(buf_val);
                continue;
            }
        }
        else if (cJSON_IsObject(data))
        {

            os_printf("    data is object \n");

            if (cJSON_GetObjectItemCaseSensitive(data, "to") != NULL)
            {
                os_printf("    parsing 'to' \n");
                char *to_str = cJSON_GetObjectItem(data, "to")->valuestring;
                size_t to_size = strlen(to_str);
                os_printf("    get 'to' valuestring %s \n", to_str);

                char *datas_raw = cJSON_Print(pack);
                os_printf("string: %s\n", datas_raw);

                int datas_size = strlen((const char *)datas_raw);
                os_printf("    strlen %d \n", datas_size);
                uint8_t datas_u8[datas_size];
                os_printf("    datas_u8 alloc >> size: %d \n", datas_size);

                for (size_t i = 0; i < datas_size; i++)
                {
                    datas_u8[i] = datas_raw[i];
                }

                os_printf("    datas_u8 parsed \n");

                uint8_t peer_addrs[6] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
                int peer_addrs_int[6] = {0, 0, 0, 0, 0, 0};
                // sscanf(to_str, "%2hhx:%2hhx:%2hhx:%2hhx:%2hhx:%2hhx", &peer_addr[0], &peer_addr[1], &peer_addr[2], &peer_addr[3], &peer_addr[4], &peer_addr[5] );

                int res = sscanf(to_str, "%x:%x:%x:%x:%x:%x",
                                 &peer_addrs_int[0], &peer_addrs_int[1], &peer_addrs_int[2], &peer_addrs_int[3], &peer_addrs_int[4], &peer_addrs_int[5]);

                for (size_t i = 0; i < 6; i++)
                {
                    peer_addrs[i] = (uint8_t)peer_addrs_int[i];
                }
                printf("parse is fuck %d \n", res);

                os_printf("    peer mac is %02x:%02x:%02x:%02x:%02x:%02x\n",
                          peer_addrs[0],
                          peer_addrs[1],
                          peer_addrs[2],
                          peer_addrs[3],
                          peer_addrs[4],
                          peer_addrs[5]);

                os_printf("    my mac is %02x:%02x:%02x:%02x:%02x:%02x\n",
                          my_mac[0],
                          my_mac[1],
                          my_mac[2],
                          my_mac[3],
                          my_mac[4],
                          my_mac[5]);

                int mcmp = memcmp(peer_addrs, my_mac, ESP_NOW_ETH_ALEN);
                os_printf("MCMP >> %d \n", mcmp);
                if (mcmp == 0)
                {
                    to_me = true;
                }
                else
                {
                    to_me = false;
                    os_printf("exex_packet >> not to me... \n");
                    if (esp_now_is_peer_exist(peer_addrs) == false)
                    {
                        os_printf("    peer not found \n");
                        esp_now_peer_info_t *peer = /*may cause crash*/ os_malloc(sizeof(esp_now_peer_info_t));
                        if (peer == NULL)
                        {
                            os_printf("Malloc peer information fail \n");
                        }
                        os_printf("    make a peer info \n");
                        memset(peer, 0, sizeof(esp_now_peer_info_t));
                        peer->channel = MESH_CHANNEL;
                        peer->ifidx = ESPNOW_WIFI_IF;
                        peer->encrypt = false;
                        // memcpy(peer->lmk, CONFIG_ESPNOW_LMK, ESP_NOW_KEY_LEN);
                        // memcpy(peer->peer_addr, peer_addrs, ESP_NOW_ETH_ALEN);
                        if (esp_now_add_peer(peer) == 0)
                        {
                            os_printf("    add peer \n");
                            send_packet(peer_addrs, datas_u8, datas_size);
                        }
                        else
                        {
                            os_printf("    failed to add peer >> broadcast \n");
                            send_packet(example_broadcast_mac, datas_u8, datas_size);
                        }
                        os_free(peer);
                    }
                    else
                    {
                        send_packet(peer_addrs, datas_u8, datas_size);
                    }
                }
            }

            // IF SENDER IS NOT EQUALS TO RECV_CB MAC >> DO NOT SEND PACKET FOR HIM

            if (to_me)
            {
                os_printf("packet to me >> parsing... \n");
                if (cJSON_GetObjectItemCaseSensitive(data, "pingmsg") != NULL)
                {
                    // buf_val = cJSON_GetObjectItem(data, "pingmsg");
                    char *msg = cJSON_GetObjectItem(data, "pingmsg")->valuestring;

                    memset(pingmsg, 0, sizeof(pingmsg));
                    uint8_t *msg8t = (uint8_t *)msg; // working convertion
                    memcpy(pingmsg, msg8t, strlen(msg));

                    os_free(msg8t);
                    os_free(msg);
                }

                if (cJSON_GetObjectItemCaseSensitive(data, "broadcast") != NULL)
                {
                }

                if (cJSON_GetObjectItemCaseSensitive(data, "digitalWrite") != NULL)
                {
                    cJSON *digital_write_val = cJSON_GetObjectItem(data, "digitalWrite");
                    if (cJSON_GetObjectItemCaseSensitive(digital_write_val, "pin") != NULL &&
                        cJSON_GetObjectItemCaseSensitive(digital_write_val, "value") != NULL)
                    {
                        char *pin_ = cJSON_GetObjectItem(digital_write_val, "pin")->valuestring;
                        int pin = 255;
                        sscanf(pin_, "%d", &pin);

                        char *val_ = cJSON_GetObjectItem(digital_write_val, "value")->valuestring;
                        int val = 255;
                        sscanf(val_, "%d", &val);

                        os_printf("pin %d | val %d \n", pin, val);

                        gpio_set_direction(pin, GPIO_MODE_OUTPUT);
                        gpio_set_level(pin, val);
                        // digitalWrite(pin, val);
                        os_printf("digitalWrite \n");

                        cJSON *tmp_val = cJSON_CreateArray();

                        cJSON_AddItemToArray(tmp_val, read_pin(pin));

                        ret_ = cJSON_Print(tmp_val);

                        cJSON_Delete(tmp_val);
                        os_free(tmp_val);

                        os_free(pin_);
                        os_free(val_);
                        continue;
                    }
                }

                if (cJSON_GetObjectItemCaseSensitive(data, "digitalRead") != NULL)
                {
                    buf_val = cJSON_GetObjectItem(data, "digitalRead");
                    if (cJSON_GetObjectItemCaseSensitive(buf_val, "pin") != NULL)
                    {
                        int pin = cJSON_GetObjectItem(buf_val, "pin")->valueint;

                        cJSON *tmp_val = cJSON_CreateArray();
                        cJSON_AddItemToArray(tmp_val, read_pin(pin));

                        ret_ = cJSON_Print(tmp_val);
                        cJSON_Delete(tmp_val);
                        os_free(tmp_val);
                        os_free(pin);
                        continue;
                    }
                }
            }
        }
        /*                 cJSON *datas_arr = cJSON_GetObjectItem(data, "data");
                        os_printf("    get 'data' cJSON object \n");
                        char *datas = cJSON_Print(datas_arr);
                        os_printf("    get 'data' cJSON_Print \n");
                        size_t datas_size = 100; // ESP_NOW_MAX_DATA_LEN ??? //strlen(datas);
                        os_printf("    get 'data' strlen \n");
        */
       cJSON_Delete(data);
       os_free(data);
    }

    //cJSON_Delete(&buf_val);
    //os_printf("exec_packet cJSON_Delete \n");

    cJSON_free(buf_val);
	os_printf("exec_packet cJSON_free \n");


    os_free(buf_val);
    os_printf("exec_packet os_free(buf_val) \n");
    os_free(pack);
    os_printf("exec_packet os_free(pack) \n");

    os_printf("exec_packet end \n");

	return ret_;
}



























static void app_loop()
{

    vTaskDelay(5000 / portTICK_RATE_MS);

    if (!_connected)
    {
        _connected = true;
    }

    os_printf("esp_get_free_heap_size >> %d \n", esp_get_free_heap_size());

    //cJSON *datas = cJSON_Parse(&data);


    // send_packet(example_broadcast_mac, pingmsg, sizeof(pingmsg)); // BACK!
}


void vTaskFunction( void * pvParameters )
{
    TickType_t xLastWakeTime;
    const TickType_t xFrequency = 10;
    // Initialise the xLastWakeTime variable with the current time.
    xLastWakeTime = xTaskGetTickCount();
    for( ;; )
    {
        // Wait for the next cycle.
        vTaskDelayUntil( &xLastWakeTime, xFrequency );

        app_loop();

        // Perform action here.
    }
}






#define BUF_SIZE (1024)

static void uart_callback(char *data)
{
    os_printf("data: %s \n", data);
}

static void echo_task()
{
    // Configure parameters of an UART driver,
    // communication pins and install the driver
    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_CTS
    };
    uart_param_config(UART_NUM_0, &uart_config);
    uart_driver_install(UART_NUM_0, BUF_SIZE * 2, 0, 0, NULL, 0);

    os_free(&uart_config);

    // Configure a temporary buffer for the incoming data
    uint8_t *data = (uint8_t *) /*may cause crash*/ os_malloc(BUF_SIZE);

    while (1) {
        // Read data from the UART
        int len = uart_read_bytes(UART_NUM_0, data, BUF_SIZE, 20 / portTICK_RATE_MS);
        // Write data back to the UART
        uart_write_bytes(UART_NUM_0, (const char *) data, len);
        //os_printf("\n");
    }
}













void event_loop(void *params)
{

    msx_event_t evt; //malloc( sizeof( msx_event_t ) );
    //memset( evt, 0, sizeof( msx_event_t ) );

    vTaskDelay(5000 / portTICK_RATE_MS);

    while( xQueueReceive( event_loop_queue, &evt, portMAX_DELAY ) == pdTRUE )
    {
        switch( evt.id )
        {
            case MSX_ESP_NOW_SEND_CB:
            {
                os_printf( "event_loop >> MSX_ESP_NOW_SEND_CB \n" );
                break;
            }
            case MSX_ESP_NOW_RECV_CB:
            {
                os_printf( "event_loop >> MSX_ESP_NOW_RECV_CB \n" );
                break;
            }
            case WIFI_EVENT_STA_START:
            {
                os_printf( "event_loop >> MSX_WIFI_EVENT_STA_START \n" );
                break;
            }
            case WIFI_EVENT_STA_DISCONNECTED:
            {
                os_printf( "event_loop >> MSX_WIFI_EVENT_STA_DISCONNECTED \n" );
                break;
            }
            case IP_EVENT_STA_GOT_IP:
            {
                ip_event_got_ip_t *event = (ip_event_got_ip_t *)evt.data;
                os_printf( "event_loop >> MSX_IP_EVENT_STA_GOT_IP %s \n", ip4addr_ntoa(&event->ip_info.ip) );
                break;
            }
            default:
            {
                os_printf( "event_loop >> UNKNOWN_EVENT %d \n", evt.id );
                break;
            }
        }
    }
}





















void app_main()
{

    event_loop_queue = xQueueCreate( ESPNOW_QUEUE_SIZE, sizeof( msx_event_t ) );
    xTaskCreate(event_loop, "vTask_event_loop", 16 * 1024, NULL, 0, NULL);

    // Initialize NVS
    ESP_ERROR_CHECK( nvs_flash_init() );

    uart_set_baudrate(0, 115200);

    //initialize_console();

    //example_wifi_init();
    wifi_init();
    example_espnow_init();
    server = start_webserver();

    //xTaskCreate(vTaskFunction, "vTaskFunction_loop", 16 * 1024, NULL, 0, NULL);

    os_printf("________TASK_INIT_DONE________\n");

}
