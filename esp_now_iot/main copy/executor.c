#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <string.h>
#include <cJSON.h>

#include <esp_wifi.h>
#include <esp_now.h>
#include "driver/gpio.h"

#include "main.h"

#include "gpiojson.c"
#include "packets.c"

char *exec_packet(cJSON *pack)
{
	cJSON *buf_val = NULL;
    char *ret_ = "";
    int arr_sz = cJSON_GetArraySize(pack);
	os_printf("exec_packet start \n");
    os_printf("packet length %d \n", arr_sz);

    bool to_me = true;


	// USE STRCMP

	if (pack == NULL || cJSON_IsInvalid(pack))
	{
		os_printf("parser malfunction \n");
		return "parser malfunction";
	}

	if (cJSON_GetArraySize(pack) < 0)
	{
		return "data misfunction";
	}

	for (uint32_t i = 0; i < cJSON_GetArraySize(pack); i++)
	{
        os_printf("using pack %d \n", i);
        const cJSON *data = cJSON_GetArrayItem(pack, i);
        // char *data_type = json_type( (cJSON *) data);
        // char *data_tmp = cJSON_Print(data);
        // os_printf("data: %s \n", data_tmp);
        // printf("data: %s type: %s\n", data_tmp, data_type);

        //    printf(data["pin"]);
        //    printf(data.hasOwnProperty("schedule"));
        //    printf(data.hasOwnProperty("pin"));

        if (cJSON_IsString(data))
        {

            os_printf("    data is string \n");

            if (strcmp("status", data->valuestring) == 0)
            {
                os_printf("    data is status \n");
                // uint8_t temp_farenheit = temprature_sens_read();
                // float temp_celsius = ( temp_farenheit - 32 ) / 1.8;
                // ^ need to include to status packet;

                buf_val = cJSON_CreateNull();
                buf_val = cJSON_CreateArray();

                cJSON_AddItemToArray(buf_val, read_pin(0));
                cJSON_AddItemToArray(buf_val, read_pin(1));
                cJSON_AddItemToArray(buf_val, read_pin(2));

                ret_ = cJSON_Print(buf_val);
                cJSON_Delete(buf_val);
                continue;
            }
        }
        else if (cJSON_IsObject(data))
        {

            os_printf("    data is object \n");

            if (cJSON_GetObjectItemCaseSensitive(data, "to") != NULL)
            {
                os_printf("    parsing 'to' \n");
                char *to_str = cJSON_GetObjectItem(data, "to")->valuestring;
                size_t to_size = strlen(to_str);
                os_printf("    get 'to' valuestring %s \n", to_str);

                char *datas_raw = cJSON_Print(pack);
                os_printf("string: %s\n", datas_raw);

                int datas_size = strlen((const char *)datas_raw);
                os_printf("    strlen %d \n", datas_size);
                uint8_t datas_u8[datas_size];
                os_printf("    datas_u8 alloc >> size: %d \n", datas_size);

                for (size_t i = 0; i < datas_size; i++)
                {
                    datas_u8[i] = datas_raw[i];
                }

                os_printf("    datas_u8 parsed \n");

                uint8_t my_mac[ESP_NOW_ETH_ALEN];
                esp_wifi_get_mac(ESP_IF_WIFI_STA, my_mac);

                uint8_t peer_addrs[6] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
                int peer_addrs_int[6] = {0, 0, 0, 0, 0, 0};
                // sscanf(to_str, "%2hhx:%2hhx:%2hhx:%2hhx:%2hhx:%2hhx", &peer_addr[0], &peer_addr[1], &peer_addr[2], &peer_addr[3], &peer_addr[4], &peer_addr[5] );

                int res = sscanf(to_str, "%x:%x:%x:%x:%x:%x",
                                 &peer_addrs_int[0], &peer_addrs_int[1], &peer_addrs_int[2], &peer_addrs_int[3], &peer_addrs_int[4], &peer_addrs_int[5]);

                for (size_t i = 0; i < 6; i++)
                {
                    peer_addrs[i] = (uint8_t)peer_addrs_int[i];
                }
                printf("parse is fuck %d \n", res);

                os_printf("    peer mac is %02x:%02x:%02x:%02x:%02x:%02x\n",
                          peer_addrs[0],
                          peer_addrs[1],
                          peer_addrs[2],
                          peer_addrs[3],
                          peer_addrs[4],
                          peer_addrs[5]);

                os_printf("    my mac is %02x:%02x:%02x:%02x:%02x:%02x\n",
                          my_mac[0],
                          my_mac[1],
                          my_mac[2],
                          my_mac[3],
                          my_mac[4],
                          my_mac[5]);

                int mcmp = memcmp(peer_addrs, my_mac, ESP_NOW_ETH_ALEN);
                os_printf("MCMP >> %d \n", mcmp);
                if (mcmp == 0)
                {
                    to_me = true;
                }
                else
                {
                    to_me = false;
                    os_printf("exex_packet >> not to me... \n");
                    if (esp_now_is_peer_exist(peer_addrs) == false)
                    {
                        os_printf("    peer not found \n");
                        esp_now_peer_info_t *peer = /*may cause crash*/ os_malloc(sizeof(esp_now_peer_info_t));
                        if (peer == NULL)
                        {
                            os_printf("Malloc peer information fail \n");
                        }
                        os_printf("    make a peer info \n");
                        memset(peer, 0, sizeof(esp_now_peer_info_t));
                        peer->channel = MESH_CHANNEL;
                        peer->ifidx = ESPNOW_WIFI_IF;
                        peer->encrypt = false;
                        // memcpy(peer->lmk, CONFIG_ESPNOW_LMK, ESP_NOW_KEY_LEN);
                        // memcpy(peer->peer_addr, peer_addrs, ESP_NOW_ETH_ALEN);
                        if (esp_now_add_peer(peer) == 0)
                        {
                            os_printf("    add peer \n");
                            send_packet(peer_addrs, datas_u8, datas_size);
                        }
                        else
                        {
                            os_printf("    failed to add peer >> broadcast \n");
                            send_packet(broadcast_mac, datas_u8, datas_size);
                        }
                        os_free(peer);
                    }
                    else
                    {
                        send_packet(peer_addrs, datas_u8, datas_size);
                    }
                }
            }

            // IF SENDER IS NOT EQUALS TO RECV_CB MAC >> DO NOT SEND PACKET FOR HIM

            if (to_me)
            {
                os_printf("packet to me >> parsing... \n");
                if (cJSON_GetObjectItemCaseSensitive(data, "pingmsg") != NULL)
                {
                    // buf_val = cJSON_GetObjectItem(data, "pingmsg");
                    char *msg = cJSON_GetObjectItem(data, "pingmsg")->valuestring;

                    memset(pingmsg, 0, sizeof(pingmsg));
                    uint8_t *msg8t = (uint8_t *)msg; // working convertion
                    memcpy(pingmsg, msg8t, strlen(msg));

                    os_free(msg8t);
                    os_free(msg);
                }

                if (cJSON_GetObjectItemCaseSensitive(data, "broadcast") != NULL)
                {
                }

                if (cJSON_GetObjectItemCaseSensitive(data, "digitalWrite") != NULL)
                {
                    cJSON *digital_write_val = cJSON_GetObjectItem(data, "digitalWrite");
                    if (cJSON_GetObjectItemCaseSensitive(digital_write_val, "pin") != NULL &&
                        cJSON_GetObjectItemCaseSensitive(digital_write_val, "value") != NULL)
                    {
                        char *pin_ = cJSON_GetObjectItem(digital_write_val, "pin")->valuestring;
                        int pin = 255;
                        sscanf(pin_, "%d", &pin);

                        char *val_ = cJSON_GetObjectItem(digital_write_val, "value")->valuestring;
                        int val = 255;
                        sscanf(val_, "%d", &val);

                        os_printf("pin %d | val %d \n", pin, val);

                        gpio_set_direction(pin, GPIO_MODE_OUTPUT);
                        gpio_set_level(pin, val);
                        // digitalWrite(pin, val);
                        os_printf("digitalWrite \n");

                        cJSON *tmp_val = cJSON_CreateArray();

                        cJSON_AddItemToArray(tmp_val, read_pin(pin));

                        ret_ = cJSON_Print(tmp_val);

                        cJSON_Delete(tmp_val);
                        os_free(tmp_val);

                        os_free(pin_);
                        os_free(val_);
                        continue;
                    }
                }

                if (cJSON_GetObjectItemCaseSensitive(data, "digitalRead") != NULL)
                {
                    buf_val = cJSON_GetObjectItem(data, "digitalRead");
                    if (cJSON_GetObjectItemCaseSensitive(buf_val, "pin") != NULL)
                    {
                        int pin = cJSON_GetObjectItem(buf_val, "pin")->valueint;

                        cJSON *tmp_val = cJSON_CreateArray();
                        cJSON_AddItemToArray(tmp_val, read_pin(pin));

                        ret_ = cJSON_Print(tmp_val);
                        cJSON_Delete(tmp_val);
                        os_free(tmp_val);
                        os_free(pin);
                        continue;
                    }
                }
            }
        }
        /*                 cJSON *datas_arr = cJSON_GetObjectItem(data, "data");
                        os_printf("    get 'data' cJSON object \n");
                        char *datas = cJSON_Print(datas_arr);
                        os_printf("    get 'data' cJSON_Print \n");
                        size_t datas_size = 100; // ESP_NOW_MAX_DATA_LEN ??? //strlen(datas);
                        os_printf("    get 'data' strlen \n");
        */
       cJSON_Delete(data);
       os_free(data);
    }

    //cJSON_Delete(&buf_val);
    //os_printf("exec_packet cJSON_Delete \n");

    cJSON_free(buf_val);
	os_printf("exec_packet cJSON_free \n");


    os_free(buf_val);
    os_printf("exec_packet os_free(buf_val) \n");
    os_free(pack);
    os_printf("exec_packet os_free(pack) \n");

    os_printf("exec_packet end \n");

	return ret_;
}