#ifndef __MSX_ESPNOW_INIT__
#define __MSX_ESPNOW_INIT__


#include <esp_libc.h>
#include <esp_now.h>
/* #include <esp_http_server.h> */
#include <string.h>

#include <cJSON.h>

#include "msx_httpd.c"
#include "msx_event_loop.c"
#include "msx_debug.c"
#include "msx_wifi.c"
#include "msx_utils.c"


#define MSX_PEER_COUNT 4
#define CONFIG_ESPNOW_PMK   "pmk1234567890123"
#define CONFIG_ESPNOW_LMK   "lmk1234567890123"
uint8_t broadcast_mac[6] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };


#define MSG_STACK_SIZE  6
uint32_t msg_stack[MSG_STACK_SIZE];

uint32_t peer_count = 0;


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
esp_err_t pair_request(uint8_t mac[ESP_NOW_ETH_ALEN]);
esp_err_t send_packet(uint8_t mac[ESP_NOW_ETH_ALEN], packet_t *pack);
esp_err_t send_packet_raw(uint8_t mac[ESP_NOW_ETH_ALEN], uint8_t data[PACKET_BUFFER_SIZE], size_t len);
esp_err_t select_cast(uint8_t src_mac[ESP_NOW_ETH_ALEN], packet_t *pack);

esp_err_t peers_get_handler(httpd_req_t *req);

int mac_cmp_json(cJSON *obj, uint8_t mac[ESP_NOW_ETH_ALEN]);
int mac_cmp_char_uint8t(char mac1[ESP_NOW_ETH_ALEN], uint8_t mac2[ESP_NOW_ETH_ALEN]);

void stack_print (uint32_t *stack);
bool stack_push (uint32_t *dest, uint32_t data);
bool stack_null (uint32_t *stack);
bool stack_exists (uint32_t *stack, uint32_t data);


httpd_uri_t peers_uri_get = { .uri = "/peers", .method = HTTP_GET, .handler = peers_get_handler, .user_ctx = NULL };


packet_t *init_packet()
{
    size_t sz = sizeof(packet_t);
    if (sz > 250)
    {
        __MSX_PRINT__("can not allocate | size of packet_t can not be more than 250 bytes");
        return NULL;
    }
    return (packet_t*) os_malloc(sz);
}


esp_err_t init_espnow()
{
    __MSX_DEBUG__( esp_now_init() );
    __MSX_DEBUG__( esp_now_register_send_cb(send_cb) );
    __MSX_DEBUG__( esp_now_register_recv_cb(recv_cb) );
    __MSX_DEBUG__( esp_now_set_pmk((uint8_t *)CONFIG_ESPNOW_PMK) );

    __MSX_DEBUG__( add_peer(broadcast_mac, (uint8_t*) CONFIG_ESPNOW_LMK, MESH_CHANNEL, WIFI_IF, false) );

    //__MSX_DEBUG__( httpd_register_uri_handler(msx_server, &peers_uri_get) );

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

    // magic word that will say others to add us to peer list
    char *coffee = "COFFEE"; // free coffee
    
    blink();
    __MSX_PRINTF__("mac: "MACSTR" size: %d", MAC2STR(mac_addr), len);   // ???
    if (len > sizeof(packet_t))
    {
        __MSX_PRINTF__("unknown packet with size %d | ignoring", len);
        return;
    }

    size_t pack_sz = sizeof(packet_t);
    packet_t *pack = (packet_t *) os_malloc(pack_sz);
    memset(pack, 0, pack_sz);
    memcpy(pack, data, pack_sz);

    uint8_t buffer[pack->len];
    memset(buffer, 0, pack->len);
    memcpy(buffer, pack->buffer, pack->len);

    if ( stack_exists(msg_stack, pack->magic))
    {
        __MSX_PRINTF__("packet->magic %d is already processed | ignoring", pack->magic);
        goto recv_exit;
    }

    if (pack->len > PACKET_BUFFER_SIZE)
    {
        __MSX_PRINTF__("abnormal packet->len %d | ignoring", pack->len);
        goto recv_exit;
    }

    __MSX_PRINTF__("data %.*s", pack->len, buffer);
    if (strncmp(&buffer, coffee, strlen(coffee)) == 0)
    {
        __MSX_DEBUG__( pair_request(mac_addr) );
        return;
    }



    cJSON *root = cJSON_Parse( (const char*) buffer);
    

    if (mac_cmp_json(root, my_mac) == 0)
    {
        __MSX_PRINT__("this is my mac!");
        char *resp = exec_packet((const char*) buffer, pack->len);
        __MSX_PRINTF__("exec resp is %s", resp);
    } else
    {
        __MSX_PRINT__("redirecting!");
        if (peer_count < MSX_PEER_COUNT)
        {
            uint8_t peer[ESP_NOW_ETH_ALEN] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
            if ( !cJSON_GetObjectItemCaseSensitive(root, "to")) return false;
            char *to_str = cJSON_GetObjectItem(root, "to");

            __MSX_DEBUG__( sscanf(to_str, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx", &peer[0], &peer[1], &peer[2], &peer[3], &peer[4], &peer[5]) );
        
            __MSX_DEBUG__( add_peer(peer, (uint8_t*) CONFIG_ESPNOW_LMK, MESH_CHANNEL, WIFI_IF, false) );

            __MSX_DEBUG__( select_cast(peer, pack) );
        } else
        {
            __MSX_PRINT__("cannot do anything :)");
            //__MSX_DEBUG__( select_cast(mac_addr, pack) );
            goto recv_exit;
        }
    }





    cJSON_Delete(root);

//    goto recv_exit;

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
    __MSX_DEBUGV__( os_free(pack) );
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

    err = esp_now_add_peer(peer);
    if (err == ESP_OK) peer_count++;

    __MSX_PRINTF__("peer added "MACSTR"", MAC2STR(mac));

add_exit:
    __MSX_DEBUGV__( os_free(peer) );
    return err;
}


esp_err_t pair_request(uint8_t mac[ESP_NOW_ETH_ALEN])
{
    if (peer_count >= 4)
    {
        __MSX_PRINT__("max peer count reached");
        return ESP_FAIL;
    }

    if (esp_now_is_peer_exist(mac))
    {
        __MSX_PRINTF__("peer "MACSTR" exist", MAC2STR(mac));
        return ESP_ERR_ESPNOW_EXIST;
    }

    __MSX_DEBUG__( add_peer(mac, (uint8_t*) CONFIG_ESPNOW_LMK, MESH_CHANNEL, WIFI_IF, false) );

    return ESP_OK;
}


esp_err_t send_packet(uint8_t mac[ESP_NOW_ETH_ALEN], packet_t *pack)
{
    size_t pack_sz = sizeof(packet_t);
    return esp_now_send(mac, pack, pack_sz);
}


esp_err_t send_packet_raw(uint8_t mac[ESP_NOW_ETH_ALEN], uint8_t data[PACKET_BUFFER_SIZE], size_t len)
{
    esp_err_t err = ESP_OK;

    size_t pack_sz = sizeof(packet_t);
    packet_t *pack = (packet_t*) os_malloc(pack_sz);
    memcpy(pack->mac_addr, mac, ESP_NOW_ETH_ALEN);
    memcpy(pack->buffer, data, PACKET_BUFFER_SIZE);
    pack->len = len;

    // err = esp_now_is_peer_exist(mac);
    // if (err != ESP_OK) goto send_exit;

/*     if ( !esp_now_is_peer_exist(mac) )
    {
        if ( !add_peer(mac, (uint8_t*) CONFIG_ESPNOW_LMK, MESH_CHANNEL, WIFI_IF, false) )
        {
            __MSX_PRINTF__("cannot add peer"MACSTR"", MAC2STR(mac));
            err = ESP_ERR_ESPNOW_NOT_FOUND;
            goto send_exit;
        }
    } */
    __MSX_PRINTF__("sending [%.*s] to "MACSTR"", pack->len, pack->buffer, MAC2STR(mac));
    err = esp_now_send(mac, pack, pack_sz);

goto send_exit;
send_exit:
    os_free(pack);
    return err;
}


esp_err_t select_cast(uint8_t src_mac[ESP_NOW_ETH_ALEN], packet_t *pack)
{
    size_t peer_sz = sizeof(esp_now_peer_info_t);
    esp_now_peer_info_t *peer = (esp_now_peer_info_t*) os_malloc(peer_sz);
    memset(peer, 0, peer_sz);
    bool from_head = true;

    uint8_t e = 0x00;

    for (size_t i = 0; i < 4; i++)
    {
        esp_now_fetch_peer(from_head, peer);
        from_head = false;

        if (memcmp(peer->peer_addr[0], e, 1) != 0) continue;
        if (memcmp(peer->peer_addr, src_mac, ESP_NOW_ETH_ALEN) == 0) continue;

        send_packet(peer->peer_addr, pack);

        __MSX_PRINTF__("sending [%.*s] to "MACSTR"", pack->len, pack->buffer, MAC2STR(peer->peer_addr));
    }

    os_free(peer);
    return ESP_OK;
}


esp_err_t radare_signal_peers()
{
    char *coffee = "COFFEE";
    if (peer_count >= 4) return;
    return send_packet_raw(broadcast_mac, (uint8_t *) coffee, strlen(coffee));
}


void print_peers()
{
    size_t peer_sz = sizeof(esp_now_peer_info_t);
    esp_now_peer_info_t *peer = (esp_now_peer_info_t*) os_malloc(peer_sz);
    memset(peer, 0, peer_sz);
    bool from_head = true;

    for (esp_err_t e = esp_now_fetch_peer(true, peer); e == ESP_OK; e = esp_now_fetch_peer(false, peer))
    {
        __MSX_PRINTF__("peer "MACSTR" | ifidx %d | channel %d", MAC2STR(peer->peer_addr), peer->ifidx, peer->channel);
    }

    os_free(peer);
}


esp_err_t peers_get_handler(httpd_req_t *req)
{
    cJSON *root = cJSON_CreateArray();
    cJSON *peer_tmp = cJSON_CreateObject();

    size_t peer_sz = sizeof(esp_now_peer_info_t);
    esp_now_peer_info_t *peer = (esp_now_peer_info_t*) os_malloc(peer_sz);
    memset(peer, 0, peer_sz);

    for (esp_err_t e = esp_now_fetch_peer(true, peer); e == ESP_OK; e = esp_now_fetch_peer(false, peer))
    {
        char macstr[18];
        sprintf(macstr, MACSTR, MAC2STR(peer->peer_addr));
        cJSON_AddStringToObject(peer_tmp, "mac", macstr);
        cJSON_AddNumberToObject(peer_tmp, "ifidx", peer->ifidx);
        cJSON_AddNumberToObject(peer_tmp, "channel", peer->channel);

        __MSX_PRINTF__("peer "MACSTR" | ifidx %d | channel %d", MAC2STR(peer->peer_addr), peer->ifidx, peer->channel);
    }

    cJSON_AddItemToArray(root, peer_tmp);

    char *string = cJSON_Print(root); // cJSON_PrintUnformatted

    __MSX_PRINTF__("string %s", string);

    httpd_resp_set_type(req, "application/json");
	httpd_resp_send(req, string, strlen(string));

    /* __MSX_DEBUGV__(  */cJSON_Delete(root)  /* ) */;
    __MSX_DEBUGV__( os_free(root)   );
    __MSX_DEBUGV__( os_free(string) );

    __MSX_PRINT__("status_get_handler end");

    // do it next day
    return ESP_OK;
}


int mac_cmp_json(cJSON *obj, uint8_t mac[ESP_NOW_ETH_ALEN])
{
    if ( !cJSON_GetObjectItemCaseSensitive(obj, "to")) return false;

    char *to_str = cJSON_GetObjectItem(obj, "to");
    if (to_str == NULL) return false;

    return mac_cmp_char_uint8t(to_str, mac);
}

int mac_cmp_char_uint8t(char mac1[ESP_NOW_ETH_ALEN], uint8_t mac2[ESP_NOW_ETH_ALEN])
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


        /*
            letter about me.

            a few years ago, i started coding to MCU,
            at this moment i want to begin my small business
            about develop of IoT devices...

            finally, i stuck with broken flash chip :)
        */