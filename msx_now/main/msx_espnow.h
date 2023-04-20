#ifndef __MSX_ESPNOW_INIT__
#define __MSX_ESPNOW_INIT__


#include <esp_libc.h>
#include <esp_now.h>
/* #include <esp_http_server.h> */
#include <string.h>
#include <cJSON.h>


#include "msx_debug.h"
#include "msx_httpd.h"
#include "msx_event_loop.h"
#include "msx_wifi.h"
#include "msx_utils.h"
#include "msx_executor.h"
#include "msx_event_loop.h"


#define MSX_PEER_COUNT 4
#define CONFIG_ESPNOW_PMK   "pmk1234567890123"
#define CONFIG_ESPNOW_LMK   "lmk1234567890123"
static const uint8_t broadcast_mac[ESP_NOW_ETH_ALEN] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };


#define MSG_STACK_SIZE  6
extern uint32_t msg_stack[MSG_STACK_SIZE];
extern uint32_t peer_count;


enum packet_type_e
{
    PACKET_TYPE_DATA = 0,
    PACKET_TYPE_PAIR = 1,
    PACKET_TYPE_KILL = 2,
};


#define PACKET_BUFFER_SIZE  200
typedef struct
{
    uint32_t magic;
    uint8_t mac_addr[ESP_NOW_ETH_ALEN];
    enum packet_type_e type;
    size_t len;
    uint8_t buffer[PACKET_BUFFER_SIZE];
} __attribute__((packed)) packet_t;
//static size_t pack_sz = sizeof(packet_t);
// //  4 + 8 + 200 + 4
// //  we have 34 bytes free
// 28 bytes free


packet_t *init_packet();
esp_err_t init_espnow();
void send_cb(const uint8_t *mac_addr, esp_now_send_status_t status);
void recv_cb(const uint8_t *mac_addr, const uint8_t *data, int len);

esp_err_t add_peer(uint8_t mac[ESP_NOW_ETH_ALEN], uint8_t lmk[16], uint8_t channel, wifi_interface_t ifidx, bool encrypted);
esp_err_t pair_request(uint8_t mac[ESP_NOW_ETH_ALEN]);
esp_err_t unpair_request(uint8_t mac[ESP_NOW_ETH_ALEN]);
esp_err_t send_packet(uint8_t mac[ESP_NOW_ETH_ALEN], packet_t *pack);
esp_err_t send_packet_raw(uint8_t mac[ESP_NOW_ETH_ALEN], uint8_t data[PACKET_BUFFER_SIZE], size_t len);

esp_err_t retransmit_packet(/* uint8_t src_mac[ESP_NOW_ETH_ALEN],  */packet_t *pack);
esp_err_t select_cast(uint8_t src_mac[ESP_NOW_ETH_ALEN], packet_t *pack);
esp_err_t multi_cast(packet_t *pack);
esp_err_t radar_peers();
void print_peers();


esp_err_t peers_get_handler(httpd_req_t *req);
extern httpd_uri_t peers_uri_get;


int mac_cmp_json(cJSON *obj, uint8_t mac[ESP_NOW_ETH_ALEN]);
int mac_cmp_char_uint8t(char mac1[ESP_NOW_ETH_ALEN], uint8_t mac2[ESP_NOW_ETH_ALEN]);

void stack_print (uint32_t *stack);
bool stack_push (uint32_t *dest, uint32_t data);
bool stack_null (uint32_t *stack);
bool stack_exists (uint32_t *stack, uint32_t data);


#endif


        /*
            letter about me.

            a few years ago, i started coding to MCU,
            at this moment i want to begin my small business
            about develop of IoT devices...

            finally, i stuck with broken flash chip :)
        */
