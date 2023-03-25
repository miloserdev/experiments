#include <stdint.h>
#include <stddef.h>
#include <esp_now.h>

typedef struct {
    //add bool broadcast unicast;
    uint32_t magic;
    size_t len;
    uint8_t *buffer;
    uint8_t mac_addr[ESP_NOW_ETH_ALEN];
    esp_now_send_status_t status;
} msx_message_t;


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
    size_t len;
} msx_event_t;