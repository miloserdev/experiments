#ifndef __MSX_EXECUTOR_INIT__
#define __MSX_EXECUTOR_INIT__


#include <esp_libc.h>
#include <esp_err.h>
#include <esp_system.h>
#include <esp_now.h>
#include <esp_http_server.h>

#include <cJSON.h>

#include "msx_debug.c"
#include "msx_utils.c"


char *exec_packet(char *datas, size_t len)
{
    char fuckdata[len];
    memcpy(fuckdata, datas, len);

    size_t input_size = len;
    __MSX_PRINTF__("input data %.*s size: %d", input_size, fuckdata, input_size);

    char *ret_ = "";

    cJSON *pack = cJSON_Parse(fuckdata);

    cJSON *ret_arr = cJSON_CreateArray();

    /* cJSON *tmp_val; */

	if (pack == NULL || cJSON_IsInvalid(pack))
	{
        __MSX_PRINT__("parser malfunction");
        __MSX_DEBUGV__( goto clean_exit );
		ret_ = "parser malfunction";
	}

    int arr_sz = cJSON_GetArraySize(pack);
	if (arr_sz < /* <= */ 0)
	{
        __MSX_PRINT__("data malfunction");
        __MSX_DEBUGV__( goto clean_exit );
		ret_ = "data misfunction";
	}

    bool to_me = true;

	for (uint32_t i = 0; i < arr_sz; i++)
	{
        __MSX_PRINTF__("using packet %d", i);
        const cJSON *data = cJSON_GetArrayItem(pack, i);
        // char *data_type = json_type( (cJSON *) data);
        // char *data_tmp = cJSON_Print(data);
        // os_printf("data: %s \n", data_tmp);
        // printf("data: %s type: %s\n", data_tmp, data_type);

        //    os_printf(data["pin"]);
        //    os_printf(data.hasOwnProperty("schedule"));
        //    os_printf(data.hasOwnProperty("pin"));

        if (cJSON_IsString(data))
        {
            __MSX_PRINT__("data is string");

            if (strcmp("status", data->valuestring) == 0)
            {
                __MSX_PRINT__("data is status");
                // uint8_t temp_farenheit = temprature_sens_read();
                // float temp_celsius = ( temp_farenheit - 32 ) / 1.8;
                // ^ need to include to status packet;

                cJSON *buf_val = cJSON_CreateArray();

                cJSON_AddItemToArray(buf_val, read_pin(0));
                cJSON_AddItemToArray(buf_val, read_pin(1));
                cJSON_AddItemToArray(buf_val, read_pin(2));

                /* ret_ = cJSON_Print(buf_val); */
                cJSON_Delete(buf_val);
                continue;
            }
        }

        else if (cJSON_IsObject(data))
        {

            __MSX_PRINT__("data is object");

            if (cJSON_GetObjectItemCaseSensitive(data, "to") != NULL)
            {
                char *to_str = cJSON_GetObjectItem(data, "to")->valuestring;
                /* size_t to_size = strlen(to_str); */

/*                 char *datas_raw = cJSON_Print(pack); // cJSON_PrintUnformatted
                int datas_size = strlen((const char *)datas_raw);
                uint8_t datas_u8[datas_size];
                for (size_t i = 0; i < datas_size; i++) datas_u8[i] = datas_raw[i]; */

                uint8_t my_mac[ESP_NOW_ETH_ALEN] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
                esp_wifi_get_mac(ESP_IF_WIFI_STA, my_mac);
                // esp_efuse_mac_get_default();

                uint8_t peer_addrs[ESP_NOW_ETH_ALEN] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
                /* int addr_int[6] = {0, 0, 0, 0, 0, 0}; */
                sscanf(to_str, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx", &peer_addrs[0], &peer_addrs[1], &peer_addrs[2], &peer_addrs[3], &peer_addrs[4], &peer_addrs[5]);
                //for (size_t i = 0; i < 6; i++) peer_addrs[i] = (uint8_t)addr_int[i];

/*                 os_printf("%s >> peer mac is %02x:%02x:%02x:%02x:%02x:%02x\n", __FUNCTION__, peer_addrs[0], peer_addrs[1], peer_addrs[2], peer_addrs[3], peer_addrs[4], peer_addrs[5]);
                os_printf("%s >> my mac is %02x:%02x:%02x:%02x:%02x:%02x\n", __FUNCTION__, my_mac[0], my_mac[1], my_mac[2], my_mac[3], my_mac[4], my_mac[5]);
 */
                __MSX_PRINTF__("peer mac is "MACSTR"", MAC2STR(peer_addrs));
                __MSX_PRINTF__("my mac is "MACSTR"", MAC2STR(my_mac));

                to_me = ( memcmp(peer_addrs, my_mac, ESP_NOW_ETH_ALEN) == 0 );

                if (!to_me)
                {
                    __MSX_PRINT__("not for me");
                    
                    esp_now_del_peer(peer_addrs);
                    if (!esp_now_is_peer_exist(peer_addrs))
                    {
                        __MSX_PRINT__("peer not found");

                        if ( add_peer(peer_addrs, /* & */(uint8_t *) CONFIG_ESPNOW_LMK, MESH_CHANNEL, ESPNOW_WIFI_IF, false) == 0)
                        {
                            __MSX_PRINT__("peer added >> sending direct message");
                            send_packet(peer_addrs, pack);
                        }
                        else
                        {
                            __MSX_PRINT__("failed to add peer >> broadcast");
                            send_packet(broadcast_mac, pack);
                        }
                    }
                    else
                    {
                        send_packet(peer_addrs, pack);
                    }
                }

                /* os_free(&addr_int); */
                /*  */
                //__MSX_DEBUGV__( os_free(&peer_addrs)   );
                /* os_free(&my_mac); */
/*                 os_free(&datas_u8);
                os_free(datas_raw); */
                __MSX_DEBUGV__( os_free(to_str)     );
            }

            // IF SENDER IS NOT EQUALS TO RECV_CB MAC >> DO NOT SEND PACKET FOR HIM

            if (to_me)
            {
                __MSX_PRINT__("packet to me >> parsing...");

                if (cJSON_GetObjectItemCaseSensitive(data, "broadcast") != NULL)
                {
                }

                if (cJSON_GetObjectItemCaseSensitive(data, "digitalWrite") != NULL)
                {
                    cJSON *digital_write_val = cJSON_GetObjectItem(data, "digitalWrite");
                    if (cJSON_GetObjectItemCaseSensitive(digital_write_val, "pin") != NULL &&
                        cJSON_GetObjectItemCaseSensitive(digital_write_val, "value") != NULL)
                    {
                        int pin = (gpio_num_t) cJSON_GetObjectItem(digital_write_val, "pin")->valueint;
                        int val = cJSON_GetObjectItem(digital_write_val, "value")->valueint;

                        val = (val > 1) ? !gpio_get_level(pin) : val;

                        __MSX_PRINTF__("digitalWrite >> pin %d | val %d ", pin, val);

                        gpio_set_direction(pin, GPIO_MODE_OUTPUT);
                        gpio_set_level(pin, val);

                        cJSON_AddItemToArray(ret_arr, read_pin(pin));

                        // cJSON_Delete(tmp_val);
                        continue;
                    }
                }

                if (cJSON_GetObjectItemCaseSensitive(data, "digitalRead") != NULL)
                {
                    cJSON *buf_val = cJSON_GetObjectItem(data, "digitalRead");
                    if (cJSON_GetObjectItemCaseSensitive(buf_val, "pin") != NULL)
                    {
                        int pin = cJSON_GetObjectItem(buf_val, "pin")->valueint;

                        // cJSON_AddItemToArray(tmp_val, read_pin(pin));

                        char pin_name[10] = "0";
                        sprintf(pin_name, "%d", pin);

                        int val = gpio_get_level(pin);
                        char *ret_ = parse_value(val, false);

                        cJSON *pin_obj = cJSON_CreateObject();
                        cJSON_AddStringToObject(pin_obj, "pin", pin_name);
                        cJSON_AddStringToObject(pin_obj, "value", ret_);

                        cJSON_AddItemToArray(ret_arr, pin_obj);

                        __MSX_PRINTF__("digitalRead >> pin %d | val %d ", pin, val);

                        // cJSON_Delete(tmp_val);
                        continue;
                    }
                }
            }
        }
       /* cJSON_Delete(data); */
    }

    __MSX_DEBUGV__( cJSON_Delete(pack)      );

    ret_ = cJSON_PrintUnformatted(ret_arr);

    clean_exit:

        __MSX_DEBUGV__( cJSON_Delete(ret_arr)   );
        //__MSX_DEBUGV__( os_free(fuckdata)       );

        __MSX_PRINTF__("return is %s ", ret_);

        __MSX_PRINT__("end");

	    return ret_;
}



#endif