#include "msx_espnow.h"


uint32_t msg_stack[MSG_STACK_SIZE];
uint32_t peer_count = 0;

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
    __MSX_PRINTF__("sending to "MACSTR" is %s", MAC2STR(mac_addr), (status == ESP_NOW_SEND_SUCCESS ? "success" : "FAILED!"));
   // __MSX_DEBUG__( raise_event(MSX_ESP_NOW_SEND_CB, NULL, status, NULL, 0) );
}


void recv_cb(const uint8_t *mac_addr, const uint8_t *data, int len)
{
    //blink();

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

    __MSX_PRINTF__("packet_t | magic %d | mac_addr "MACSTR" type %d | len %d | buffer %s", pack->magic, MAC2STR(pack->mac_addr), pack->type, pack->len, pack->buffer);


    switch(pack->type)
    {
        case PACKET_TYPE_PAIR:
        {
            __MSX_DEBUG__( pair_request(mac_addr) );
            goto recv_exit;
            break;
        }
        case PACKET_TYPE_KILL:
        {
            __MSX_DEBUG__( unpair_request(mac_addr) );
            goto recv_exit;
            break;
        }
        case PACKET_TYPE_DATA:
        {
            uint8_t buffer[pack->len];
            memset(buffer, 0, pack->len);
            memcpy(buffer, pack->buffer, pack->len);

            __MSX_PRINTF__("data %.*s", pack->len, buffer);

            if ( !stack_exists(msg_stack, pack->magic) )
            {
                stack_push(msg_stack, pack->magic);
            } else
            {
                __MSX_PRINTF__("packet->magic %d is already processed | ignoring", pack->magic);
                goto recv_exit;
            }

            if (pack->len > PACKET_BUFFER_SIZE)
            {
                __MSX_PRINTF__("abnormal packet->len %d | ignoring", pack->len);
                goto recv_exit;
            }

            __MSX_DEBUG__( raise_event(MSX_ESP_NOW_RECV_CB, NULL, 0, buffer, pack->len) );

            break;
        }
    }

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


esp_err_t unpair_request(uint8_t mac[ESP_NOW_ETH_ALEN])
{
    esp_err_t err = ESP_OK;
    if (memcmp(mac, broadcast_mac, ESP_NOW_ETH_ALEN) == 0)
    {
        __MSX_PRINTF__("broadcast ["MACSTR"] peer cannot be disconnected", MAC2STR(mac));
        err = ESP_FAIL;
        goto unpair_exit;
    }
    if (esp_now_is_peer_exist(mac))
    {
        err = esp_now_del_peer(mac);
        __MSX_PRINTF__("peer "MACSTR" %s", MAC2STR(mac), (err == ESP_OK ? "disconnected" : "CANNOT BE DISCONNECTED"));
        goto unpair_exit;
    } else
    {
        __MSX_PRINTF__("MALFUNCTION!!! peer "MACSTR" is connected but does not exist", MAC2STR(mac));
        err = ESP_FAIL;
        goto unpair_exit;
    }

unpair_exit:
    return err;
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
    pack->type = PACKET_TYPE_DATA;
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


/// ДОПИСАТЬ БЛЕАТЬ!!!

esp_err_t retransmit_packet(/* uint8_t src_mac[ESP_NOW_ETH_ALEN],  */packet_t *pack)
{
    esp_err_t err = ESP_OK;
    
    if (peer_count < MSX_PEER_COUNT)
    {
        err = add_peer(pack->mac_addr, (uint8_t*) CONFIG_ESPNOW_LMK, MESH_CHANNEL, WIFI_IF, false);
        if (err == ESP_OK)
        {
            __MSX_DEBUG__( select_cast(pack->mac_addr, pack) );
        }
    } else
    {
        __MSX_DEBUG__( multi_cast(pack) );
        //__MSX_DEBUG__( select_cast(mac_addr, pack) );
        goto ret_exit;
    }

ret_exit:
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

    __MSX_DEBUGV__( os_free(peer) );
    return ESP_OK;
}


esp_err_t multi_cast(packet_t *pack)
{
    size_t peer_sz = sizeof(esp_now_peer_info_t);
    esp_now_peer_info_t *peer = (esp_now_peer_info_t*) os_malloc(peer_sz);
    memset(peer, 0, peer_sz);

    for (esp_err_t e = esp_now_fetch_peer(true, peer); e == ESP_OK; e = esp_now_fetch_peer(false, peer))
    {
        esp_now_send(peer->peer_addr, pack, sizeof(packet_t));
        __MSX_PRINTF__("multicast to peer "MACSTR" | ifidx %d | channel %d", MAC2STR(peer->peer_addr), peer->ifidx, peer->channel);
    }

    __MSX_DEBUGV__( os_free(peer) );

    return ESP_OK;
}


// 34:94:54:62:9f:74
esp_err_t radar_peers()
{
    if (!esp_now_is_peer_exist(broadcast_mac))
    {
        __MSX_DEBUG__( add_peer(broadcast_mac, (uint8_t*) CONFIG_ESPNOW_LMK, MESH_CHANNEL, WIFI_IF, false) );
    }
    

    size_t pack_sz = sizeof(packet_t);
    packet_t *pack = (packet_t *) os_malloc(pack_sz);
    memset(pack, 0, pack_sz);
    pack->type = PACKET_TYPE_PAIR;
    pack->magic = esp_random();
    esp_err_t err = esp_now_send(broadcast_mac, pack, pack_sz);

    __MSX_PRINTF__("err is %d", err);

    __MSX_DEBUGV__( os_free(pack) );

    return err;
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

    __MSX_DEBUGV__( os_free(peer) );
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

    __MSX_DEBUGV__( os_free(peer) );

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