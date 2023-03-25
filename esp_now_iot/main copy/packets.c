#include <stdint.h>
#include <stdbool.h>
#include <sdkconfig.h>

#include "msx.c"
#include "stacks.c"

void send_packet(const uint8_t *peer_addr, const uint8_t *strs, size_t len)
{
    //espnow_send_param_t *data = /*may cause crash*/ os_malloc(sizeof(espnow_send_param_t));
    //memset(data, 0, sizeof(espnow_send_param_t));

    msx_message_t *data = os_malloc(sizeof(msx_message_t));
    memset(data, 0, sizeof(msx_message_t));

    uint32_t _magic = esp_random();
    
    data->magic = _magic;
    data->len = CONFIG_ESPNOW_SEND_LEN;
    data->buffer = os_malloc(CONFIG_ESPNOW_SEND_LEN);

    memset(data->buffer, 0, CONFIG_ESPNOW_SEND_LEN);
    memcpy(data->buffer, strs, len);
    memcpy(data->mac_addr, peer_addr, ESP_NOW_ETH_ALEN);

    if (esp_now_send(data->mac_addr, data->buffer, data->len) != ESP_OK) {
    //if (esp_now_send(broadcast_mac, strs, sizeof(strs)) != ESP_OK) {
        os_printf("Send error \n");
    }

    stack_push(msg_stack, _magic);

    os_free(data->buffer);
    os_free(data->mac_addr);
    os_free(data);
}
