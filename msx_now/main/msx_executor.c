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

    cJSON *pack = cJSON_Parse(fuckdata);

    char *ret_ = "";
    cJSON *ret_arr = cJSON_CreateArray();

    /* cJSON *tmp_val; */

    if (pack == NULL || cJSON_IsInvalid(pack))
    {
        __MSX_PRINT__("parser malfunction");
        __MSX_DEBUGV__(goto clean_exit);

        cJSON *tmp = cJSON_CreateObject();
        cJSON_AddStringToObject(tmp, "error", "parser misfunction");
        cJSON_AddItemToArray(ret_arr, tmp);
    }

    int arr_sz = cJSON_GetArraySize(pack);
    if (arr_sz < /* <= */ 0)
    {
        __MSX_PRINT__("data malfunction");
        __MSX_DEBUGV__(goto clean_exit);

        cJSON *tmp = cJSON_CreateObject();
        cJSON_AddStringToObject(tmp, "error", "data misfunction");
        cJSON_AddItemToArray(ret_arr, tmp);
    }

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

                if (cJSON_GetObjectItemCaseSensitive(data, "light") != NULL)
                {
                    // buf_val = cJSON_GetObjectItem(data, "pingmsg");
                    char *msg = cJSON_GetObjectItem(data, "light")->valuestring;

                    blink();

                    // os_free(msg8t);
                    __MSX_DEBUGV__( os_free(msg)    );
                }

            if (cJSON_GetObjectItemCaseSensitive(data, "digitalWrite") != NULL)
            {
                cJSON *digital_write_val = cJSON_GetObjectItem(data, "digitalWrite");
                if (cJSON_GetObjectItemCaseSensitive(digital_write_val, "pin") != NULL &&
                    cJSON_GetObjectItemCaseSensitive(digital_write_val, "value") != NULL)
                {
                    int pin = (gpio_num_t)cJSON_GetObjectItem(digital_write_val, "pin")->valueint;
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
        /* cJSON_Delete(data); */
    }

    __MSX_DEBUGV__(cJSON_Delete(pack));

    ret_ = cJSON_PrintUnformatted(ret_arr);

clean_exit:

    __MSX_DEBUGV__(cJSON_Delete(ret_arr));
    //__MSX_DEBUGV__( os_free(fuckdata)       );

    __MSX_PRINTF__("return is %s ", ret_);

    return ret_;
}

#endif