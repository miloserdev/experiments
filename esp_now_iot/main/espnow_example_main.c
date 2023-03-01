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

#include "esp_err.h"
#include "esp_vfs_dev.h"


#include "uart_init.c"


static const char *TAG = "espnow_example";

static const uint8_t example_broadcast_mac[ESP_NOW_ETH_ALEN] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
static uint16_t s_example_espnow_seq[EXAMPLE_ESPNOW_DATA_MAX] = { 0, 0 };

static void example_wifi_init(void)
{
    tcpip_adapter_init();

    ESP_ERROR_CHECK(esp_event_loop_create_default());

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK( esp_wifi_init(&cfg) );
    ESP_ERROR_CHECK( esp_wifi_set_storage(WIFI_STORAGE_RAM) );
    ESP_ERROR_CHECK( esp_wifi_set_mode(ESPNOW_WIFI_MODE) );
    ESP_ERROR_CHECK( esp_wifi_start());
    ESP_ERROR_CHECK( esp_wifi_set_channel(CONFIG_ESPNOW_CHANNEL, 0) );
}


static void example_espnow_send_cb(const uint8_t *mac_addr, esp_now_send_status_t status)
{
    example_espnow_event_send_cb_t *send = malloc(sizeof(example_espnow_event_send_cb_t));

    if (mac_addr == NULL) {
        os_printf("example_espnow_send_cb >> error >> status: %d \n", status);
        return;
    }

    memcpy(send->mac_addr, mac_addr, ESP_NOW_ETH_ALEN);
    send->status = status;

    os_printf("example_espnow_send_cb >> mac: "MACSTR" status: %d \n", MAC2STR(send->mac_addr), send->status);

}

static void example_espnow_recv_cb(const uint8_t *mac_addr, const uint8_t *data, int len)
{
    example_espnow_event_recv_cb_t *recv = malloc(sizeof(example_espnow_event_recv_cb_t));

    if (mac_addr == NULL || data == NULL || len <= 0) {
        ESP_LOGE(TAG, "Receive cb arg error");
        return;
    }

    memcpy(recv->mac_addr, mac_addr, ESP_NOW_ETH_ALEN);
    recv->data = malloc(len);
    memcpy(recv->data, data, len);
    recv->data_len = len;

    os_printf("example_espnow_recv_cb >> mac: "MACSTR" len: %d data: [", MAC2STR(recv->mac_addr), recv->data_len);

    for (int i = 0; i < recv->data_len - 1; i++)
    {
        os_printf("%c", recv->data[i]);
    }
    os_printf("] \n");


    if (esp_now_is_peer_exist(mac_addr) == false) {
        esp_now_peer_info_t *peer = malloc(sizeof(esp_now_peer_info_t));
        if (peer == NULL) {
            os_printf("Malloc peer information fail \n");
        }
        memset(peer, 0, sizeof(esp_now_peer_info_t));
        peer->channel = CONFIG_ESPNOW_CHANNEL;
        peer->ifidx = ESPNOW_WIFI_IF;
        peer->encrypt = true;
        memcpy(peer->lmk, CONFIG_ESPNOW_LMK, ESP_NOW_KEY_LEN);
        memcpy(peer->peer_addr, mac_addr, ESP_NOW_ETH_ALEN);
        ESP_ERROR_CHECK( esp_now_add_peer(peer) );
        free(peer);
    }
}


void send_packet(const uint8_t *peer_addr, const uint8_t *strs, size_t len)
{
    example_espnow_send_param_t *data;
    data = malloc(sizeof(example_espnow_send_param_t));
    memset(data, 0, sizeof(example_espnow_send_param_t));
    data->unicast = false;
    data->broadcast = true;
    data->state = 0;
    data->magic = esp_random();
    data->count = CONFIG_ESPNOW_SEND_COUNT;
    data->delay = CONFIG_ESPNOW_SEND_DELAY;
    data->len = CONFIG_ESPNOW_SEND_LEN;
    data->buffer = malloc(CONFIG_ESPNOW_SEND_LEN);

    memset(data->buffer, 0, CONFIG_ESPNOW_SEND_LEN);
    memcpy(data->buffer, strs, len);
    memcpy(data->dest_mac, peer_addr, ESP_NOW_ETH_ALEN);

    if (esp_now_send(data->dest_mac, data->buffer, data->len) != ESP_OK) {
    //if (esp_now_send(example_broadcast_mac, strs, sizeof(strs)) != ESP_OK) {
        os_printf("Send error \n");
    }
}


static esp_err_t example_espnow_init(void)
{
    example_espnow_send_param_t *send_param;

    /* Initialize ESPNOW and register sending and receiving callback function. */
    ESP_ERROR_CHECK( esp_now_init() );
    ESP_ERROR_CHECK( esp_now_register_send_cb(example_espnow_send_cb) );
    ESP_ERROR_CHECK( esp_now_register_recv_cb(example_espnow_recv_cb) );

    /* Set primary master key. */
    ESP_ERROR_CHECK( esp_now_set_pmk((uint8_t *)CONFIG_ESPNOW_PMK) );

    /* Add broadcast peer information to peer list. */
    esp_now_peer_info_t *peer = malloc(sizeof(esp_now_peer_info_t));
    if (peer == NULL) {
        os_printf("Malloc peer information fail \n");
        return ESP_FAIL;
    }
    memset(peer, 0, sizeof(esp_now_peer_info_t));
    peer->channel = CONFIG_ESPNOW_CHANNEL;
    peer->ifidx = ESPNOW_WIFI_IF;
    peer->encrypt = false;
    memcpy(peer->peer_addr, example_broadcast_mac, ESP_NOW_ETH_ALEN);
    ESP_ERROR_CHECK( esp_now_add_peer(peer) );
    free(peer);

    return ESP_OK;
}


static void app_loop()
{

    vTaskDelay(5000 / portTICK_RATE_MS);

    const uint8_t data[] = "fuck this boards at all!!!";
    send_packet(example_broadcast_mac, data, sizeof(data));
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


void app_main()
{
    // Initialize NVS
    ESP_ERROR_CHECK( nvs_flash_init() );

	initialize_console();

    example_wifi_init();
    example_espnow_init();

    xTaskCreate(vTaskFunction, "vTaskFunction_loop", 16 * 1024, NULL, 0, NULL);
    printf("________TASK_INIT_DONE________\n");
}
