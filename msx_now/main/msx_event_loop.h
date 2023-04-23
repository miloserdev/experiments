#ifndef __MSX_EVENT_LOOP__
#define __MSX_EVENT_LOOP__


#include "FreeRTOS.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/timers.h"
#include <queue.h>
#include <task.h>

#include <esp_err.h>
#include <tcpip_adapter.h>

#include <esp_event_base.h>
#include <esp_wifi_types.h>
#include <string.h>


#include "msx_debug.h"
#include "msx_executor.h"


//#define CONFIG_STATION_MODE         1 //deprecated 4 me
#define ESPNOW_QUEUE_SIZE           6


typedef struct {
    //msx_event_id_t id;
    //msx_event_data_t data;
    //__uint8_t *from;
    esp_event_base_t base;
    __uint32_t status;
    int32_t id;
    void *data;
    __size_t len;
} msx_event_t;


typedef enum {
    MSX_ESP_NOW_SEND_CB = 99,
    MSX_ESP_NOW_RECV_CB,
    MSX_ESP_NOW_INIT,
    MSX_ESP_NOW_DATA_SEND,

    MSX_UART_DATA,

    MSX_WIFI_EVENT_STA_START,
    MSX_WIFI_EVENT_STA_DISCONNECTED,
    MSX_WIFI_EVENT_WIFI_INIT,
    MSX_IP_EVENT_STA_GOT_IP,
} msx_event_id_t;


void event_loop(void *params);
esp_err_t init_event_loop();
void user_loop( void (*func)(void) );
esp_err_t init_user_loop(void (*task)(void));
bool raise_event(int id, esp_event_base_t base, __uint32_t status, void *data, __size_t len);


#endif