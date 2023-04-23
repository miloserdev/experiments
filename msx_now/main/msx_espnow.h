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


#define MSX_MAX_PEER_COUNT 4
#define RADAR_PING_DELAY_ZERO_PEER 3000
#define RADAR_PING_DELAY_CONNECTED 30000


#define CONFIG_ESPNOW_PMK   "pmk1234567890123"
#define CONFIG_ESPNOW_LMK   "lmk1234567890123"
static const __uint8_t broadcast_mac[ESP_NOW_ETH_ALEN] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };


#define MSG_STACK_SIZE  6
extern __uint32_t msg_stack[MSG_STACK_SIZE];
extern __uint32_t peer_count;


enum packet_type_e
{
    PACKET_TYPE_DATA = 0x00,
    PACKET_TYPE_PAIR = 0x01,
    PACKET_TYPE_KILL = 0x02,
};


#define PACKET_BUFFER_SIZE  200
typedef struct
{
    __uint32_t magic;
    __uint8_t mac_addr[ESP_NOW_ETH_ALEN];
    enum packet_type_e type : 2;
    __size_t len;
    __uint8_t buffer[PACKET_BUFFER_SIZE];
} __attribute__((packed)) packet_t;
// __packed__
static size_t pack_sz = sizeof(packet_t);
// //  4 + 8 + 200 + 4
// //  we have 34 bytes free
// 28 bytes free


packet_t *init_packet();
esp_err_t init_espnow();
void send_cb(const __uint8_t *mac_addr, esp_now_send_status_t status);
void recv_cb(const __uint8_t *mac_addr, const __uint8_t *data, int len);

esp_err_t add_peer(__uint8_t mac[ESP_NOW_ETH_ALEN], __uint8_t lmk[16], __uint8_t channel, wifi_interface_t ifidx, bool encrypted);
esp_err_t pair_request(__uint8_t mac[ESP_NOW_ETH_ALEN]);
esp_err_t unpair_request(__uint8_t mac[ESP_NOW_ETH_ALEN]);
esp_err_t send_packet(__uint8_t mac[ESP_NOW_ETH_ALEN], packet_t *pack);
esp_err_t send_packet_raw(__uint8_t mac[ESP_NOW_ETH_ALEN], __uint8_t data[PACKET_BUFFER_SIZE], __size_t len);

esp_err_t retransmit_packet(/* __uint8_t src_mac[ESP_NOW_ETH_ALEN],  */packet_t *pack);
esp_err_t select_cast(__uint8_t src_mac[ESP_NOW_ETH_ALEN], packet_t *pack);
esp_err_t multi_cast(packet_t *pack);
esp_err_t radar_peers();
void radar_loop( void (*func)(void) );
void print_peers();


esp_err_t peers_get_handler(httpd_req_t *req);
extern httpd_uri_t peers_uri_get;


int mac_cmp_json(cJSON *obj, __uint8_t mac[ESP_NOW_ETH_ALEN]);
int mac_cmp_char_uint8t(char mac1[ESP_NOW_ETH_ALEN], __uint8_t mac2[ESP_NOW_ETH_ALEN]);

void stack_print (__uint32_t *stack);
bool stack_push (__uint32_t *dest, __uint32_t data);
bool stack_null (__uint32_t *stack);
bool stack_exists (__uint32_t *stack, __uint32_t data);


#endif


        /*
            letter about me.

            a few years ago, i started coding to MCU,
            at this moment i want to begin my small business
            about develop of IoT devices...

            finally, i stuck with broken flash chip :)
        */
