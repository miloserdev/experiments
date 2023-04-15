#ifndef __MSX_ESPNOW_INIT__
#define __MSX_ESPNOW_INIT__


#include <esp_libc.h>
#include <esp_now.h>
#include <string.h>

#include <cJSON.h>

#include "msx_event_loop.c"
#include "msx_debug.c"
#include "msx_wifi.c"
#include "msx_utils.c"



#define CONFIG_ESPNOW_PMK   "pmk1234567890123"
#define CONFIG_ESPNOW_LMK   "lmk1234567890123"


#define MSG_STACK_SIZE  6
uint32_t msg_stack[MSG_STACK_SIZE];


#define PACKET_BUFFER_SIZE  200
typedef struct
{
    uint32_t magic;
    uint8_t mac_addr[ESP_NOW_ETH_ALEN];
    uint8_t buffer[PACKET_BUFFER_SIZE];
    size_t len;
} __attribute__((packed)) packet_t;
//  4 + 8 + 200 + 4
//  we have 34 bytes free


packet_t *init_packet();
esp_err_t init_espnow();
void send_cb(const uint8_t *mac_addr, esp_now_send_status_t status);
void recv_cb(const uint8_t *mac_addr, const uint8_t *data, int len);

esp_err_t add_peer(uint8_t mac[ESP_NOW_ETH_ALEN], uint8_t lmk[16], uint8_t channel, wifi_interface_t ifidx, bool encrypted);
esp_err_t send_packet_raw(uint8_t mac[ESP_NOW_ETH_ALEN], uint8_t data[PACKET_BUFFER_SIZE], size_t len);

bool mac_cmp_json(cJSON *obj, uint8_t mac[ESP_NOW_ETH_ALEN]);
bool mac_cmp_char_uint8t(char mac1[ESP_NOW_ETH_ALEN], uint8_t mac2[ESP_NOW_ETH_ALEN]);

void stack_print (uint32_t *stack);
bool stack_push (uint32_t *dest, uint32_t data);
bool stack_null (uint32_t *stack);
bool stack_exists (uint32_t *stack, uint32_t data);


packet_t *init_packet()
{
    return (packet_t*) os_malloc(sizeof(packet_t));
}


esp_err_t init_espnow()
{
    __MSX_DEBUG__( esp_now_init() );
    __MSX_DEBUG__( esp_now_register_send_cb(send_cb) );
    __MSX_DEBUG__( esp_now_register_recv_cb(recv_cb) );
    __MSX_DEBUG__( esp_now_set_pmk((uint8_t *)CONFIG_ESPNOW_PMK) );

    return ESP_OK;
}



void send_cb(const uint8_t *mac_addr, esp_now_send_status_t status)
{
    //blink();
    __MSX_PRINTF__("mac: "MACSTR" status: %d", MAC2STR(mac_addr), status);
   // __MSX_DEBUG__( raise_event(MSX_ESP_NOW_SEND_CB, NULL, status, NULL, 0) );
}



void recv_cb(const uint8_t *mac_addr, const uint8_t *data, int len)
{
    blink();
    __MSX_PRINTF__("mac: "MACSTR" size: %d", MAC2STR(mac_addr), len);   // ???

    packet_t *msg = data;
    uint8_t buffer[PACKET_BUFFER_SIZE];
    memset(buffer, 0, PACKET_BUFFER_SIZE);
    memcpy(buffer, msg->buffer, len);


    __MSX_PRINTF__("data %.*s", len, buffer);


    cJSON *root = cJSON_Parse( (const char*) buffer);
    __MSX_PRINTF__("mac compare %d", mac_cmp_json(root, my_mac));
    cJSON_Delete(root);

    goto recv_exit;

/*
    size_t sz = sizeof(packet_t);
    packet_t *msg = os_malloc(sz);
    memset(msg, 0, sz);
    memcpy(msg, data, sz);  // compile msx_message_t from raw uint8_t data;

    if ( !stack_exists(msg_stack, msg->magic) )
    {
        stack_push(msg_stack, msg->magic);
    } else
    {
        __MSX_PRINT__("SHITTY WARNING!!! >> _magic is already exists in msg_stack");
        goto recv_exit;
    }
*/
/*     os_printf("\n\n\nmsx_message_t through esp_now >> \n");
    os_printf("mac_addr -> "MACSTR" \n", MAC2STR(msg->mac_addr));
    os_printf("magic -> 0x%08X \n", msg->magic);
    os_printf("size -> %d \n", msg->len);
    for (size_t i = 0; i < msg->len; i++)
    {
        os_printf("%c", msg->buffer[i]);
    }
    os_printf("\n\n\n"); */

    // return; // !!!!!!!!!!!!!!!!!!!!
    
//    uint8_t msg_buf[msg->len];
//    memcpy(msg_buf, msg->buffer, msg->len);

    //__MSX_DEBUG__( raise_event(MSX_ESP_NOW_RECV_CB, NULL, 0, msg_buf, msg->len) );


recv_exit:
    __MSX_PRINT__("free memory section");
/*     __MSX_DEBUGV__( os_free(msg->mac_addr) );
    __MSX_DEBUGV__( os_free(msg->buffer) ); */
    __MSX_DEBUGV__( os_free(msg) );
    return;
}

                                                            // HARDCODED !!!
esp_err_t add_peer(uint8_t mac[ESP_NOW_ETH_ALEN], uint8_t lmk[16], uint8_t channel, wifi_interface_t ifidx, bool encrypted)
{
    esp_err_t err = ESP_OK;

    size_t peer_sz = sizeof(esp_now_peer_info_t);
    esp_now_peer_info_t *peer = (esp_now_peer_info_t*) os_malloc(peer_sz);
    if (peer == NULL)
    {
        err = ESP_ERR_ESPNOW_NO_MEM;
        goto add_exit;
    }

    memset(peer, 0, peer_sz);
    peer->channel = channel;
    peer->ifidx = ifidx;
    peer->encrypt = encrypted;
    if (lmk != NULL) memcpy(peer->lmk, lmk, ESP_NOW_KEY_LEN);
    memcpy(peer->peer_addr, mac, ESP_NOW_ETH_ALEN);

    __MSX_DEBUG__( esp_now_add_peer(peer) );

add_exit:
    __MSX_DEBUGV__( os_free(peer) );
    return err;
}


esp_err_t send_packet_raw(uint8_t mac[ESP_NOW_ETH_ALEN], uint8_t data[PACKET_BUFFER_SIZE], size_t len)
{
    esp_err_t err = ESP_OK;

    size_t pack_sz = sizeof(packet_t);
    packet_t *msg = (packet_t*) os_malloc(pack_sz);
    memcpy(msg->mac_addr, mac, ESP_NOW_ETH_ALEN);
    memcpy(msg->buffer, data, PACKET_BUFFER_SIZE);
    msg->len = len;

    if ( !esp_now_is_peer_exist(mac) )
    {
        if ( !add_peer(mac, (uint8_t*) CONFIG_ESPNOW_LMK, MESH_CHANNEL, WIFI_IF, false) )
        {
            __MSX_PRINTF__("cannot add peer"MACSTR"", MAC2STR(mac));
            err = ESP_ERR_ESPNOW_NOT_FOUND;
            goto send_exit;
        }
    }

    err = esp_now_send(mac, data, pack_sz);

send_exit:
    os_free(msg);
    return err;
}


bool mac_cmp_json(cJSON *obj, uint8_t mac[ESP_NOW_ETH_ALEN])
{
    if ( !cJSON_GetObjectItemCaseSensitive(obj, "to")) return false;

    char *to_str = cJSON_GetObjectItem(obj, "to");
    if (to_str == NULL) return false;

    return mac_cmp_char_uint8t(to_str, mac);
}

bool mac_cmp_char_uint8t(char mac1[ESP_NOW_ETH_ALEN], uint8_t mac2[ESP_NOW_ETH_ALEN])
{
    uint8_t peer[ESP_NOW_ETH_ALEN] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    sscanf(mac1, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx", &peer[0], &peer[1], &peer[2], &peer[3], &peer[4], &peer[5]);
    return (memcmp(mac1, peer, ESP_NOW_ETH_ALEN) == 0);
}


void stack_print (uint32_t *stack)
{
    for (size_t i = 0; i < MSG_STACK_SIZE; i++)
    {
        os_printf("%d -> 0x%08X \n", i, stack[i]);
    }
}

bool stack_push (uint32_t *dest, uint32_t data)
{
//     os_printf("data is 0x%08X \n", data);
    size_t i = MSG_STACK_SIZE;
    if (i == 0) return false;
    do {
        dest[i] = (i == 0) ? data : dest[i - 1];
    } while( (i--) != 0);

/*     for (size_t i = MSG_STACK_SIZE; i--; i != 0) { stack[i] = (i == 0) ? data : stack[i - 1]; } */
    return true;
}

bool stack_null (uint32_t *stack)
{
    //memset
    for (size_t i = 0; i < MSG_STACK_SIZE; i++)
    {
        stack[i] = 0x00000000;
    }
    return true;
}

bool stack_exists (uint32_t *stack, uint32_t data)
{
    for (size_t i = 0; i < MSG_STACK_SIZE; i++)
    {
        if (stack[i] == data) return true;
    }
    return false;
}



#endif