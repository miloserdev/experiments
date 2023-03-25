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



#include "event_loop.c"
#include "executor.c"
#include "msx.c"
#include "gpiojson.c"

#include "stacks.c"



#define MIN(a, b)(((a) < (b)) ? (a) : (b))
#define MAX(a, b)(((a) > (b)) ? (a) : (b))











httpd_handle_t server = NULL;

//static uint16_t s_espnow_seq[ESPNOW_DATA_MAX] = { 0, 0 };






bool _connected = false;


char *exec_packet(cJSON *pack);
httpd_handle_t start_webserver(void);
void stop_webserver();




















#define ESP_WIFI_SSID      CONFIG_ESP_WIFI_SSID
#define ESP_WIFI_PASS      CONFIG_ESP_WIFI_PASSWORD
#define ESP_MAXIMUM_RETRY  10
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

        if (s_retry_num < ESP_MAXIMUM_RETRY)
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











static void send_cb(const uint8_t *mac_addr, esp_now_send_status_t status)
{
    msx_message_t *send = os_malloc(sizeof(msx_message_t));

    if (mac_addr == NULL) {
        os_printf("send_cb >> error >> status: %d \n", status);
        return;
    }

    memcpy(send->mac_addr, mac_addr, ESP_NOW_ETH_ALEN);
    send->status = status;

    os_printf("send_cb >> mac: "MACSTR" status: %d \n", MAC2STR(send->mac_addr), send->status);

    os_free(send->mac_addr);
    os_free(send);

    os_free(&mac_addr);
}


static void recv_cb(const uint8_t *mac_addr, const uint8_t *data, int len)
{

    msx_event_t *evt = (msx_event_t *) malloc( sizeof(  msx_event_t ) );
    //evt->base = event_base;
    evt->id = MSX_ESP_NOW_RECV_CB;
    evt->data = data;
    evt->len = len;
    os_printf("______ event_handler ______%d_\n", ( xQueueSend(event_loop_queue, evt, portMAX_DELAY) != pdTRUE ) );
    return;

    // espnow_event_recv_cb_t *recv = /*may cause crash*/ os_malloc(sizeof(espnow_event_recv_cb_t));

    msx_message_t *recv = os_malloc(sizeof(msx_message_t));
    memcpy(recv->buffer, data, len);


    if (mac_addr == NULL || data == NULL || len <= 0) {
        ESP_LOGE(TAG, "Receive cb arg error");
        return;
    }

/*
    memcpy(recv->mac_addr, mac_addr, ESP_NOW_ETH_ALEN);
    recv->data = os_malloc(len); // may cause crash
    memcpy(recv->data, data, len);
    recv->data_len = len;
*/

    os_printf("recv_cb >> mac: "MACSTR" len: %d data: [", MAC2STR(recv->mac_addr), recv->len);

    char *datas = /*may cause crash*/ os_malloc(len);
    for (int i = 0; i < recv->len - 1; i++)
    {
        datas[i] = recv->buffer[i];
        os_printf("%c", recv->buffer[i]);
    }
    os_printf("] \n");
    
    cJSON *pack = cJSON_Parse(datas);
    if ( cJSON_IsInvalid(pack) )
    {
        os_printf("recv_cb >> json receive malfunction \n");
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


    stack_print(msg_stack);

    os_free(exec_data);
    cJSON_Delete(pack);
    cJSON_free(pack);
    os_free(pack); // may cause segfault;

    os_free(datas);
    os_free(recv->mac_addr);
    os_free(recv->buffer);
    os_free(recv);

    os_free(&mac_addr);
    os_free(&data);
}































static esp_err_t espnow_init(void)
{

    uint8_t asd[] = "fuck this boards at all!!!";
    memset(pingmsg, 0, sizeof(pingmsg));
    memcpy(pingmsg, asd, sizeof(asd));

    /* Initialize ESPNOW and register sending and receiving callback function. */
    ESP_ERROR_CHECK( esp_now_init() );
    ESP_ERROR_CHECK( esp_now_register_send_cb(send_cb) );
    ESP_ERROR_CHECK( esp_now_register_recv_cb(recv_cb) );

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
    memcpy(peer->peer_addr, broadcast_mac, ESP_NOW_ETH_ALEN);
    ESP_ERROR_CHECK( esp_now_add_peer(peer) );

    os_free(peer);

    return ESP_OK;
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





























static void app_loop()
{

    vTaskDelay(5000 / portTICK_RATE_MS);

    if (!_connected)
    {
        _connected = true;
    }

    os_printf("esp_get_free_heap_size >> %d \n", esp_get_free_heap_size());

    //cJSON *datas = cJSON_Parse(&data);


    send_packet(broadcast_mac, pingmsg, sizeof(pingmsg)); // BACK!
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




















void app_main()
{

    event_loop_queue = xQueueCreate( ESPNOW_QUEUE_SIZE, sizeof( msx_event_t ) );
    xTaskCreate(event_loop, "vTask_event_loop", 16 * 1024, NULL, 0, NULL);

    // Initialize NVS
    ESP_ERROR_CHECK( nvs_flash_init() );

    uart_set_baudrate(0, 115200);

    //initialize_console();

    //wifi_init();
    wifi_init();
    espnow_init();
    server = start_webserver();

    xTaskCreate(vTaskFunction, "vTaskFunction_loop", 16 * 1024, NULL, 0, NULL);

    os_printf("________TASK_INIT_DONE________\n");

}
