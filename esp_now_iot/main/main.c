/////////////////////////////////////////////////////
/*
    1. Print cJSON number variables causes Guru Meditation :D;
        FIX: Need to disable "nano" formatting in menuconfig;
        make menuconfig -> Component config -> Newlib -> "nano" formatting

    2. UART data sending only in non separated format;
        i will make a buffer concatenator (maybe later XD);
        echo -en '\x12\x02[{"to":"34:94:54:62:9f:74","digitalWrite":{"pin":2,"value":2}}]' > /dev/ttyUSB1
*/
/*
    Need to fix:
        idk why but os_free through __MSX_DEBUGV__ works fine, but without nest calling it causes crash;
 */
/////////////////////////////////////////////////////



#include "sdkconfig.h"

#include "nvs.h"
#include "nvs_flash.h"
//#include "esp_vfs_dev.h"




#include <esp_libc.h>
#include <esp_err.h>
#include <tcpip_adapter.h>
#include <esp_http_server.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include "driver/gpio.h"
#include "driver/uart.h"
const uart_port_t uart_port = UART_NUM_0;
const uint32_t uart_buffer_size = (1024 * 1);
const uint32_t uart_buffer_size_x2 = (uart_buffer_size * 2);

#include "FreeRTOS.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/timers.h"
#include <queue.h>
#include <task.h>

#include <cJSON.h>

httpd_handle_t server = NULL;

//static xQueueHandle uart_queue;
static QueueHandle_t uart_queue;
static xQueueHandle event_loop_queue;


#define CONFIG_STATION_MODE         1 //deprecated 4 me
#define ESPNOW_QUEUE_SIZE           6

#define MESH_PASSWD                 "DNj6KdZT"
#define MESH_PASSWD_LEN             ets_strlen(MESH_PASSWD)
#define MESH_MAX_HOP                (4)
#define MESH_SSID_PREFIX            "Keenetic-6193"
#define MESH_SSID_PREFIX_LEN        ets_strlen(MESH_SSID_PREFIX)
#define MESH_CHANNEL                0 // CONFIG_ESPNOW_CHANNEL
#define PORT                        8066

#if CONFIG_STATION_MODE
#define ESPNOW_WIFI_MODE WIFI_MODE_STA
#define ESPNOW_WIFI_IF   ESP_IF_WIFI_STA
#else
#define ESPNOW_WIFI_MODE WIFI_MODE_AP
#define ESPNOW_WIFI_IF   ESP_IF_WIFI_AP
#endif


int32_t last_mem = 0;
int32_t get_leak()
{
    uint32_t fh = esp_get_free_heap_size();
    return ( fh > last_mem ) ? fh - last_mem : last_mem - fh;
}

void set_mem()
{
    last_mem = esp_get_free_heap_size();
}

#define ___MSX_FMT  "H: %d | L: %d | %s >>>"

#define __MSX_DEBUG__(f)   { os_printf(""___MSX_FMT" ___ %s ___ %s \t\t\t", esp_get_free_heap_size(), get_leak(), __FUNCTION__, #f, f == 0 ? "OK" : "ERROR"); set_mem(); }

#define __MSX_DEBUGV__(f)   { os_printf(""___MSX_FMT" ___ %s ___ \t\t\t", esp_get_free_heap_size(), get_leak(), __FUNCTION__, #f); f; os_printf("OK \n"); set_mem(); }

#define __MSX_PRINTF__(__format, __VA_ARGS__...) { os_printf(""___MSX_FMT" "__format" \n", esp_get_free_heap_size(), get_leak(), __FUNCTION__, __VA_ARGS__); set_mem(); }
#define __MSX_PRINT__(__format) { os_printf(""___MSX_FMT" "__format" \n", esp_get_free_heap_size(), get_leak(), __FUNCTION__); set_mem();}



#define MIN(a, b)(((a) < (b)) ? (a) : (b))
#define MAX(a, b)(((a) > (b)) ? (a) : (b))

uint32_t get_hash(char *data, size_t sz)
{
    uint32_t ret = 0x00000001;
    while (sz--)
    {
        ret = 31 * ret + data[sz];
    }
    return ret;
}



uint8_t pingmsg[128];
uint8_t my_mac[ESP_NOW_ETH_ALEN];
static const uint8_t broadcast_mac[ESP_NOW_ETH_ALEN] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };




#pragma pack(1)
typedef struct {
    //add bool broadcast unicast;
    uint32_t magic;
    uint8_t mac_addr[ESP_NOW_ETH_ALEN];
    uint8_t buffer[CONFIG_ESPNOW_SEND_LEN];
    size_t len;
} msx_message_t;
#pragma pack(0)

typedef enum {
    MSX_ESP_NOW_SEND_CB = 99,
    MSX_ESP_NOW_RECV_CB,
    MSX_ESP_NOW_INIT,

    MSX_UART_DATA,

    MSX_WIFI_EVENT_STA_START,
    MSX_WIFI_EVENT_STA_DISCONNECTED,
    MSX_WIFI_EVENT_WIFI_INIT,
    MSX_IP_EVENT_STA_GOT_IP,
} msx_event_id_t;

typedef struct {

} msx_event_data_t;

typedef struct {
    //msx_event_id_t id;
    //msx_event_data_t data;
    //uint8_t *from;
    esp_event_base_t base;
    esp_now_send_status_t status;
    int32_t id;
    void *data;
    size_t len;
} msx_event_t;






















#define MSG_STACK_SIZE  6
uint32_t msg_stack[MSG_STACK_SIZE];

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

#define ON_VAL "on"
#define OFF_VAL "off"

char *parse_value(int value, bool invert)
{
    return ( char* ) ( ( value ^ invert ) ? ON_VAL : OFF_VAL );
}










































cJSON * read_pin(int pin)
{
    char pin_name[10] = "0";
    sprintf(pin_name, "%d", pin);

    int val1 = gpio_get_level(pin);
    char *ret_ = parse_value(val1, false);
    cJSON *pin1 = cJSON_CreateObject();
    cJSON_AddStringToObject(pin1, "pin", pin_name);
    cJSON_AddStringToObject(pin1, "value", ret_);
    return pin1;
}
























bool add_peer(uint8_t *mac_addr, uint8_t *lmk, uint8_t channel, wifi_interface_t ifidx, bool encrypt)
{
    __MSX_PRINTF__("mac: "MACSTR"", MAC2STR(mac_addr));

    size_t peer_sz = sizeof(esp_now_peer_info_t);
    esp_now_peer_info_t *peer = /*may cause crash*/ os_malloc(peer_sz);
    if (peer == NULL) {
        __MSX_PRINT__("malloc fault");
        return ESP_FAIL;
    } else
    {
        __MSX_PRINT__("malloc ok");
    }

    memset(peer, 0, peer_sz);
    peer->channel = channel;
    peer->ifidx = ifidx;
    peer->encrypt = encrypt;
    if (lmk != NULL) memcpy(peer->lmk, lmk, ESP_NOW_KEY_LEN);
    memcpy(peer->peer_addr, mac_addr, ESP_NOW_ETH_ALEN);
    // peer->priv

    __MSX_DEBUG__( esp_now_add_peer(peer) );

    __MSX_DEBUGV__( os_free(peer)   );
/*     os_free(mac_addr);
    __MSX_PRINT__("free 'mac_addr'");
    os_free(lmk);
    __MSX_PRINT__("free 'lmk'"); */
    return ESP_OK;
}






void send_packet_raw(const uint8_t *peer_addr, const uint8_t *data, size_t len)
{
    //espnow_send_param_t *msg = /*may cause crash*/ os_malloc(sizeof(espnow_send_param_t));
    //memset(msg, 0, sizeof(espnow_send_param_t));

    size_t msx_sz = sizeof(msx_message_t);
    msx_message_t *msg = os_malloc(msx_sz);
    memset(msg, 0, sizeof(msx_message_t));

    __MSX_PRINT__("msx_message_t allocated");

    uint32_t _magic = esp_random();
    msg->magic = _magic;
    msg->len = len;
    //msg->buffer = os_malloc(CONFIG_ESPNOW_SEND_LEN);

    __MSX_PRINT__("esp_random generated");

    memset(msg->buffer, 0, CONFIG_ESPNOW_SEND_LEN);
    memcpy(msg->buffer, data, /* len */CONFIG_ESPNOW_SEND_LEN);
    memcpy(msg->mac_addr, peer_addr, ESP_NOW_ETH_ALEN);

    __MSX_PRINT__("memset & memcpy of msg");

    
    uint8_t buffer[msx_sz];
    memset(buffer, 0, msx_sz);
    memcpy(buffer, msg, msx_sz);

    __MSX_PRINT__("memset & memcpy of uint8_t buffer");

    if (esp_now_send(msg->mac_addr, buffer, msx_sz) != ESP_OK) {
    //          if (esp_now_send(msg->mac_addr, msg->buffer, msg->len) != ESP_OK) {
    //if (esp_now_send(broadcast_mac, data, sizeof(data)) != ESP_OK) {
        __MSX_PRINT__("SEND ERROR");
    }

    if ( !stack_exists(msg_stack, _magic) )
    {
        stack_push(msg_stack, _magic);
    } else
    {
        __MSX_PRINT__("WARNING BLYAT!!! esp_random() generates _magic number that same as previous");
    }


    __MSX_DEBUGV__( os_free(&buffer)    );
    __MSX_DEBUGV__( os_free(msg)        );

/*     __MSX_DEBUGV__( os_free(data)       );
    __MSX_DEBUGV__( os_free(peer_addr)  ); */
}

void send_packet(const uint8_t *peer_addr, const cJSON *data)
{
    char *char_data = cJSON_PrintUnformatted(data); // not important to print unformatted but shorter than full;
    size_t len = strlen(char_data);
    uint8_t uint8_data[len];
    
    for (size_t i = 0; i < len; i++) uint8_data[i] = char_data[i];
    __MSX_PRINTF__("buffer is %s", char_data);
    
/*     cJSON *new_data = cJSON_GetArrayItem(data, 0);

    if ( !cJSON_GetObjectItemCaseSensitive(new_data, "magic") )
    {
        printf("send_packet >> no magic >> return \n");
        return;
    }

    uint32_t _magic = cJSON_GetObjectItem(new_data, "magic"); */

    send_packet_raw(peer_addr, uint8_data, len);

    __MSX_DEBUGV__( memset(char_data, 0, len)    );
    __MSX_DEBUGV__( os_free(char_data)           );

    __MSX_DEBUGV__( memset(uint8_data, 0, len)   );
    __MSX_DEBUGV__( os_free(&uint8_data)         );           //      maybe crash
}















char *exec_packet(char *fuckdata)
{
    __MSX_PRINTF__("input data size: %d", strlen(fuckdata));

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
                size_t to_size = strlen(to_str);

/*                 char *datas_raw = cJSON_Print(pack); // cJSON_PrintUnformatted
                int datas_size = strlen((const char *)datas_raw);
                uint8_t datas_u8[datas_size];
                for (size_t i = 0; i < datas_size; i++) datas_u8[i] = datas_raw[i]; */

                uint8_t my_mac[ESP_NOW_ETH_ALEN] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
                esp_wifi_get_mac(ESP_IF_WIFI_STA, my_mac);
                // esp_efuse_mac_get_default();

                uint8_t peer_addrs[ESP_NOW_ETH_ALEN] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
                /* int addr_int[6] = {0, 0, 0, 0, 0, 0}; */
                int res = sscanf(to_str, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx", &peer_addrs[0], &peer_addrs[1], &peer_addrs[2], &peer_addrs[3], &peer_addrs[4], &peer_addrs[5]);
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
                if (cJSON_GetObjectItemCaseSensitive(data, "pingmsg") != NULL)
                {
                    // buf_val = cJSON_GetObjectItem(data, "pingmsg");
                    char *msg = cJSON_GetObjectItem(data, "pingmsg")->valuestring;

                    memset(pingmsg, 0, sizeof(pingmsg));
                    
                    //uint8_t *msg8t = (uint8_t *)msg; // working convertion;
                    //memcpy(pingmsg, msg8t, strlen(msg));

                    size_t lens = strlen(msg);
                    for (size_t i = 0; i < lens; i++) pingmsg[i] = msg[i];

                    // os_free(msg8t);
                    __MSX_DEBUGV__( os_free(msg)    );
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
                        int pin = (gpio_num_t) cJSON_GetObjectItem(digital_write_val, "pin")->valueint;
                        int val = cJSON_GetObjectItem(digital_write_val, "value")->valueint;

                        //val = (val > 1) ? (rand() % 2) : val;
                        val = (int) ( (bool) !gpio_get_level(pin) );

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
        __MSX_DEBUGV__( os_free(fuckdata)       );

        __MSX_PRINTF__("return is %s ", ret_);

        __MSX_PRINT__("end");

	return ret_;
}


/* char *exec_packet(char *buff, size_t len)
{
    char fuck[len];
    memcpy(fuck, buff, len);

    cJSON *tmp = cJSON_Parse(fuck);
    char *ret = exec_packet(tmp);

    os_printf(">>>>>>>>>>>>>>>>>>>>>>>>>> cJSON_Delete(tmp) trying \n");
    cJSON_Delete(tmp);
    cJSON_free(tmp);
    os_free(ret);
    os_free(&fuck);
    os_printf(">>>>>>>>>>>>>>>>>>>>>>>>>> cJSON_Delete(tmp) ok \n");
    return ret;
}
 */






































void event_loop(void *params)
{

    msx_event_t evt; //malloc( sizeof( msx_event_t ) );
    //memset( evt, 0, sizeof( msx_event_t ) );

    vTaskDelay(5000 / portTICK_RATE_MS);

    while( xQueueReceive( event_loop_queue, &evt, portMAX_DELAY ) == pdTRUE )
    {
        switch( evt.id )
        {
            case MSX_ESP_NOW_INIT:
            {
                __MSX_PRINT__("MSX_ESP_NOW_INIT");
                break;
            }
            case MSX_ESP_NOW_SEND_CB:
            {
                __MSX_PRINT__("MSX_ESP_NOW_SEND_CB");
                break;
            }
            case MSX_ESP_NOW_RECV_CB:
            {
                __MSX_PRINT__("MSX_ESP_NOW_RECV_CB");

            //    char *datas = /*may cause crash*/ (char *) os_malloc(evt.len);
            //    memcpy(datas, evt.data, evt.len);
                
            //    os_printf("MSX_ESP_NOW_RECV_CB >> wtf buf | size %d | buf %s \n", evt.len, datas);   // fucking memory leak
                
                //cJSON *pack = cJSON_Parse( (char *) evt.data );

                char *exec_data = exec_packet((char *) evt.data);
                os_free(exec_data);

                //cJSON_free(pack);
                //cJSON_Delete(pack);
                //os_free(pack);

                __MSX_DEBUGV__( os_free(evt.data)   );

                break;
            }

            case MSX_UART_DATA:
            {
                __MSX_PRINT__("MSX_UART_DATA");

                __MSX_PRINTF__("uart data is %.*s", evt.len, (char *) evt.data);
                char *asd = exec_packet((char *) evt.data);
                __MSX_DEBUGV__( os_free(asd)    );
                // os_free(asd);
                //os_free(tmp1);


                __MSX_DEBUGV__( os_free(evt.data)   );

                break;
            }

            case WIFI_EVENT_STA_START:
            {
                __MSX_PRINT__("MSX_WIFI_EVENT_STA_START");
                break;
            }
            case WIFI_EVENT_STA_DISCONNECTED:
            {
                __MSX_PRINT__("MSX_WIFI_EVENT_STA_DISCONNECTED");
                break;
            }
            case IP_EVENT_STA_GOT_IP:
            {
                ip_event_got_ip_t *event = (ip_event_got_ip_t *)evt.data;
                __MSX_PRINTF__("MSX_IP_EVENT_STA_GOT_IP >> ip_info.ip : %s", ip4addr_ntoa(&event->ip_info.ip));
                __MSX_DEBUGV__( os_free(event)  );
                break;
            }
            default:
            {
                __MSX_PRINTF__("UNKNOWN_EVENT %d", evt.id);
                break;
            }
        }

        __MSX_DEBUGV__( os_free(evt.data)   );
        __MSX_DEBUGV__( os_free(&evt)   );
    }

    __MSX_DEBUGV__( vTaskDelete(NULL)   );
}































bool raise_event(int id, esp_event_base_t base, esp_now_send_status_t status, void *data, size_t len)
{
    if (id < 0)
    {
        printf("%s >> no id specified \n", __FUNCTION__);
        return pdFAIL;
    }
    msx_event_t *evt = (msx_event_t *) malloc( sizeof(  msx_event_t ) );
    evt->id = id;
    evt->base = (base ? base : NULL);
    evt->status = (status ? status : 0);
    //evt->data = (data ? data : NULL);
    if (data != NULL)
    {
        void *dat = os_malloc(len);
        memset(dat, 0, len);
        memcpy(dat, data, len);

        evt->data = dat;
    }
    evt->len = len;
    /* __MSX_DEBUG__( (  */
    bool x = (xQueueSend(event_loop_queue, evt, portMAX_DELAY) != pdTRUE);
    /*  ) ); */
    os_free(evt); // causes crash
    return pdTRUE;
}





// [{"to":"34:94:54:62:9f:74","digitalWrite":{"pin":2,"value":2}}]



#define RD_BUF_SIZE uart_buffer_size
#define EX_UART_NUM uart_port


static void uart_task(void *pvParameters)
{
    uart_event_t event;
    uint8_t *dtmp = (uint8_t *) malloc(RD_BUF_SIZE);

    for (;;) {
        // Waiting for UART event.
        if (xQueueReceive(uart_queue, (void *)&event, (portTickType)portMAX_DELAY)) {
            bzero(dtmp, RD_BUF_SIZE);
            // maybe printf;

            switch (event.type) {
                // Event of UART receving data
                // We'd better handler data event fast, there would be much more data events than
                // other types of events. If we take too much time on data event, the queue might be full.
                case UART_DATA:
                {
                    uart_read_bytes(EX_UART_NUM, dtmp, event.size, portMAX_DELAY);
                    
                    uint8_t buff[event.size];
                    memset(buff, 0, event.size);
                    memcpy(buff, dtmp, event.size);

                    __MSX_PRINTF__("generating event MSX_UART_DATA with data >> %s", buff);


                    if ( raise_event(MSX_UART_DATA, NULL, 0, buff, event.size) != pdTRUE )
                    {
                        __MSX_PRINT__("raise_event error");
                    }

                    //os_free(&buff);
                    break;
                }

                // Event of HW FIFO overflow detected
                case UART_FIFO_OVF:
                    os_printf("hw fifo overflow \n");
                    //__MSX_PRINT__("hw fifo overflow");
                    // If fifo overflow happened, you should consider adding flow control for your application.
                    // The ISR has already reset the rx FIFO,
                    // As an example, we directly flush the rx buffer here in order to read more data.
                    uart_flush_input(EX_UART_NUM);
                    xQueueReset(uart_queue);
                    break;

                // Event of UART ring buffer full
                case UART_BUFFER_FULL:
                    os_printf("ring buffer full \n");
                    // If buffer full happened, you should consider encreasing your buffer size
                    // As an example, we directly flush the rx buffer here in order to read more data.
                    uart_flush_input(EX_UART_NUM);
                    xQueueReset(uart_queue);
                    break;

                case UART_PARITY_ERR:
                    os_printf("uart parity error \n");
                    break;

                // Event of UART frame error
                case UART_FRAME_ERR:
                    os_printf("uart frame error \n");
                    break;

                // Others
                default:
                    os_printf("uart event type: %d \n", event.type);
                    break;
            }
        }
    }

    free(dtmp);
    dtmp = NULL;
    vTaskDelete(NULL);
}







static void uart_task2(void *params)
{
    uart_event_t evt;

    uint8_t *uart_buf = (uint8_t *) os_malloc( uart_buffer_size );
    os_printf("uart_task >> os_malloc %d \n", uart_buffer_size);

    for (;;)
    {
        if ( xQueueReceive( uart_queue, (void */* const */) &evt, (/* TickType_t */portTickType) portMAX_DELAY ) /* == pdTRUE  */)
        {
            //memset(uart_buf, 0, uart_buffer_size);
            bzero(uart_buf, uart_buffer_size);
            os_printf("uart_task >> bzero %d \n", uart_buffer_size);

            size_t sz = evt.size;
            os_printf( "uart_task >> _________type: %d____size: %d______ \n", evt.type, sz);

            switch(evt.type)
            {
                case UART_DATA:
                {

        //          [{"to": "34:94:54:62:9f:74", "digitalWrite":{"pin": 2, "value": 0} }]


        // [{"to":"34:94:54:62:9f:74","digitalWrite":{"pin":2,"value":2}}]

                    uart_read_bytes(uart_port, uart_buf, sz, portMAX_DELAY);

                    char exit_buf[sz];
                    for (size_t i = 0; i < sz; i++) exit_buf[i] = uart_buf[i];
                    
                    os_printf("\n\n>>>>>>>>>>>> UART COMPLETE RECEIVED | data %s | size %d <<<<<<<<<<<<<<<\n\n", exit_buf, sz);

                    if ( raise_event(MSX_UART_DATA, NULL, 0, exit_buf, sz) != pdTRUE )
                    {
                        os_printf("%s >> raise_event error >> \n", __FUNCTION__);
                        os_free(uart_buf);
                        os_free(exit_buf);
                    }

                    bzero(uart_buf, uart_buffer_size);


                    bzero(exit_buf, sz);
                    os_free(&exit_buf);

                    break;
                }
                case UART_BUFFER_FULL:
                {
                    os_printf( "uart_task >> UART_BUFFER_FULL \n");
                    uart_flush_input(uart_port);
                    xQueueReset(uart_queue);
                    break;
                }
                case UART_FIFO_OVF:
                {
                    os_printf( "uart_task >> UART_FIFO_OVF \n");
                    uart_flush_input(uart_port);
                    xQueueReset(uart_queue);
                    break;
                }
                case UART_FRAME_ERR:
                {
                    os_printf( "uart_task >> UART_FRAME_ERR \n");
                    break;
                }
                case UART_PARITY_ERR:
                {
                    os_printf( "uart_task >> UART_PARITY_ERR \n");
                    break;
                }
                default:
                {
                    os_printf( "uart_task >> UNKNOWN EVENT \n");
                    break;
                }
            }
        }
    }
    

    os_free(uart_buf);       ///     VERY IMPORTANT !!!
    uart_buf = NULL;     //         memory leak;
    vTaskDelete(NULL);
}










///memory free
//esp01 33836
//esp8266 34064






































static void send_cb(const uint8_t *mac_addr, esp_now_send_status_t status)
{
    __MSX_PRINTF__("mac: "MACSTR" status: %d", MAC2STR(mac_addr), status);
    raise_event(MSX_ESP_NOW_SEND_CB, NULL, status, NULL, 0);
    //os_free(mac_addr);    // maybe memory leak;
    //os_free(evt);
}



static void recv_cb(const uint8_t *mac_addr, const uint8_t *data, int len)
{
    __MSX_PRINTF__("mac: "MACSTR" size: %d", MAC2STR(mac_addr), len);   // ???

    size_t msx_sz = sizeof(msx_message_t);
    msx_message_t *msg = os_malloc(msx_sz);
    memset(msg, 0, msx_sz);
    memcpy(msg, data, msx_sz);  // compile msx_message_t from raw uint8_t data;

    if ( !stack_exists(msg_stack, msg->magic) )
    {
        stack_push(msg_stack, msg->magic);
    } else
    {
        __MSX_PRINT__("SHITTY WARNING!!! >> _magic is already exists in msg_stack");
        return;
    }

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
    
    uint8_t msg_buf[msg->len];
    memcpy(msg_buf, msg->buffer, msg->len);

    if ( raise_event(MSX_ESP_NOW_RECV_CB, NULL, 0, msg_buf, msg->len) != pdTRUE )
    {
        __MSX_PRINT__("raise_event error");
/*         os_free(msg->buffer);
        os_free(msg_buf); */
    }


    __MSX_PRINT__("free memory section");
    // os_free(msg->buffer);
    // os_free(msg_buf);

    os_free(msg); // need to memcpy bcuz causes ^&W%#*&$W%^&Q@$%^#
    os_free(mac_addr);
    //os_free(evt);
}


void set_pingmsg(uint8_t *data, size_t len)
{
    memset(pingmsg, 0, sizeof(pingmsg));
    memcpy(pingmsg, data, len);
}

static esp_err_t espnow_init(void)
{
    __MSX_DEBUG__( esp_now_init() );
    //__MSX_DEBUG__( esp_now_init() );
    __MSX_DEBUG__( esp_now_register_send_cb(send_cb) );
    __MSX_DEBUG__( esp_now_register_recv_cb(recv_cb) );
    __MSX_DEBUG__( esp_now_set_pmk((uint8_t *)CONFIG_ESPNOW_PMK) );

    add_peer(broadcast_mac, NULL, MESH_CHANNEL, ESPNOW_WIFI_IF, false);
    //os_free(evt);

    raise_event(MSX_ESP_NOW_INIT, NULL, ESP_OK, NULL, 0);

    return ESP_OK;
}






























static void event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    raise_event(event_id, event_base, 0, event_data, 0);
    //os_free(evt);
    os_free(event_data);
}


static void wifi_init(void)
{
    tcpip_adapter_init();
    __MSX_DEBUG__( 0 );
    __MSX_DEBUG__( esp_event_loop_create_default() );

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    //wifi_sta_config_t sta = WIFI_INIT_CONFIG_DEFAULT();
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = MESH_SSID_PREFIX,
            .password = MESH_PASSWD,
            .channel = MESH_CHANNEL
        },
    };
    __MSX_DEBUG__( esp_wifi_init(&cfg) );
    __MSX_DEBUG__( esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL) );
    __MSX_DEBUG__( esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL) );
    __MSX_DEBUG__( esp_wifi_set_storage(WIFI_STORAGE_RAM) );
    __MSX_DEBUG__( esp_wifi_set_mode(WIFI_MODE_APSTA) );
    __MSX_DEBUG__( esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config) );
    __MSX_DEBUG__( esp_wifi_start() );
    __MSX_DEBUG__( esp_wifi_set_channel(MESH_CHANNEL, 0)) ;
    __MSX_DEBUG__( esp_wifi_connect() );
    __MSX_DEBUG__( esp_wifi_get_mac(ESP_IF_WIFI_STA, my_mac) );
    __MSX_DEBUG__( esp_wifi_set_ps(WIFI_PS_NONE) );

    raise_event(MSX_WIFI_EVENT_WIFI_INIT, NULL, 0, NULL, 0);
/*     msx_event_t *evt = (msx_event_t *) malloc( sizeof(  msx_event_t ) );
    evt->id = MSX_WIFI_EVENT_WIFI_INIT;
    evt->base = NULL;
    evt->data = NULL;
    __MSX_DEBUG__( ( xQueueSend(event_loop_queue, evt, portMAX_DELAY) != pdTRUE ) ); */

    os_free(&cfg);
    os_free(&wifi_config);
}




/*
typedef struct {
    //msx_event_id_t id;
    //msx_event_data_t data;
    //uint8_t *from;
    esp_event_base_t base;
    esp_now_send_status_t status;
    int32_t id;
    void *data;
    size_t len;
} msx_event_t;
*/





























#define HTTP_BUF_SIZE   256
char http_buf[HTTP_BUF_SIZE];

esp_err_t get_handler(httpd_req_t *req)
{
    cJSON *root = cJSON_CreateArray();

    cJSON_AddItemToArray(root, read_pin(0));
    cJSON_AddItemToArray(root, read_pin(1));
    cJSON_AddItemToArray(root, read_pin(2));

    char *string = cJSON_Print(root); // cJSON_PrintUnformatted

    __MSX_PRINTF__("string %s", string);

	httpd_resp_send(req, string, strlen(string));

    cJSON_Delete(root); // crash warning
    __MSX_DEBUGV__( os_free(root)   );
    __MSX_DEBUGV__( os_free(string) );

    __MSX_PRINT__("get_handler end");

    // do it next day
    return ESP_OK;
}

httpd_uri_t uri_get = { .uri = "/status",
	.method = HTTP_GET,
	.handler = &get_handler,
	.user_ctx = NULL
};



esp_err_t post_handler(httpd_req_t *req)
{
    __MSX_PRINT__("start");

    cJSON *buffer = NULL;
	char content[512];
	size_t recv_size = MIN(req->content_len, sizeof(content));

	int ret = httpd_req_recv(req, content, recv_size);
	if (ret <= 0)
	{
		if (ret == HTTPD_SOCK_ERR_TIMEOUT)
		{
			httpd_resp_send_408(req);
		}

		return ESP_FAIL;
	}

    __MSX_PRINTF__("buffer %.*s", req->content_len, content);

/*     cJSON *parsed_buffer = cJSON_Parse((const char *) content);
    os_printf("parsing buffer ... \n");
    size_t buf_len = cJSON_GetArraySize(parsed_buffer);
    os_printf("parsing buffer ... len %d \n", buf_len); */
    
    /////////////////////////////////////////////////////
    // NEED TO DISABLE "nano" formatting in menuconfig //
    /////////////////////////////////////////////////////
/* 
    for (size_t i = 0; i < buf_len; i++)
    {
        cJSON *item = cJSON_GetArrayItem(parsed_buffer, i);
        os_printf("getting item %d \n", i);
        char *printed = cJSON_PrintUnformatted(item);// cJSON_Print(item);
        os_printf("parsed_buffer item %d --> %s \n", i, printed);
    }

    return; // !!!!!!!!!!!!!!!!! */

	//buffer = cJSON_Parse((const char *) content);
	const char *resp = (const char *) exec_packet((const char *) content);

	httpd_resp_send(req, resp, sizeof(resp));


    __MSX_DEBUGV__( os_free(resp)           );
    __MSX_DEBUGV__( cJSON_Delete(buffer)    ); // crash warning
    __MSX_DEBUGV__( os_free(buffer)         );
    __MSX_DEBUGV__( os_free(&content)       ); // ??? crash    
    __MSX_DEBUGV__( os_free(&req)           );

    __MSX_PRINT__("end");

	return ESP_OK;
}
httpd_uri_t uri_post = { .uri = "/",
	.method = HTTP_POST,
	.handler = &post_handler,
	.user_ctx = NULL
};


httpd_handle_t start_webserver(void)
{
    __MSX_PRINT__("start");
	httpd_config_t config = HTTPD_DEFAULT_CONFIG();
	config.server_port = PORT;

	server = NULL;

	if (httpd_start(&server, &config) == ESP_OK)
	{
		httpd_register_uri_handler(server, &uri_get);
		httpd_register_uri_handler(server, &uri_post);
        __MSX_PRINT__("All handlers in register");
	}

	return server;
}

void stop_webserver(httpd_handle_t server)
{
    __MSX_PRINT__("start");
	if (server)
	{
		httpd_stop(server);
	}
    __MSX_PRINT__("end");
}






























static void app_loop()
{

    vTaskDelay(2000 / portTICK_RATE_MS);

    return;

    char *char_data = "[{\"digitalWrite\":{\"pin\":2,\"value\":2}}]";
    size_t len = strlen(char_data);
    uint8_t uint8_data[len];
    
    for (size_t i = 0; i < len; i++)
    {
        uint8_data[i] = char_data[i];
    }

    send_packet_raw(broadcast_mac, uint8_data, len);

    os_free(char_data);
    os_free(&uint8_data);

    return; // !!!!!!!!!!!!

    

    uint32_t _magic = esp_random();
    char *char_magic[sizeof(_magic)];
    sprintf(char_magic, "%08x", _magic);

                                             // \"magic\": \"0\",
                                             // char *char_data = "[{\"broadcast\":\"hello fuck\"}]";
                                             // = cJSON_Parse(char_data);

    cJSON *data = cJSON_CreateArray();

    cJSON *crs = cJSON_CreateObject();
    cJSON_AddStringToObject(crs, "magic", char_magic);
    cJSON_AddStringToObject(crs, "broadcast", "hello fuck");

    cJSON_AddItemToArray(data, crs);

    send_packet(broadcast_mac, data); // BACK!

    stack_print(msg_stack);
}


void vTaskFunction( void * pvParameters )
{
    TickType_t xLastWakeTime;
    const TickType_t xFrequency = 10;
    xLastWakeTime = xTaskGetTickCount();
    for( ;; )
    {
        vTaskDelayUntil( &xLastWakeTime, xFrequency );

        app_loop();

        os_printf("esp_get_free_heap_size >> %d \n", esp_get_free_heap_size());
    }
}




















void app_main()
{
    event_loop_queue = xQueueCreate( ESPNOW_QUEUE_SIZE, sizeof( msx_event_t ) );
    xTaskCreate(event_loop, "vTask_event_loop", 16 * 1024, NULL, 0, NULL);
                                                // ??????


    __MSX_DEBUG__( nvs_flash_init() );

    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };
    uart_param_config(uart_port, &uart_config);

    uart_driver_install(uart_port, uart_buffer_size_x2, uart_buffer_size_x2, 10, &uart_queue, 0);
    
    //uart_queue = xQueueCreate( ESPNOW_QUEUE_SIZE, uart_buffer_size );
    //uart_set_baudrate(uart_port, 115200);
    xTaskCreate(uart_task, "vTask_uart_task", uart_buffer_size_x2, NULL, 0, NULL);


    // esp_get_free_heap_size >> 41616 <-- exec_packet
    //                           41648 <-- app_loop
    // esp_get_free_heap_size >> 41692 <-- app_loop
    // esp_get_free_heap_size >> 42088 <-- espnow_init
    // esp_get_free_heap_size >> 42168 <-- wifi_init
    // esp_get_free_heap_size >> 84064 <-- with event_loop
    // esp_get_free_heap_size >> 100824 
    //                              ^
    //                              |
    //                              ok



    wifi_init();
    espnow_init();
    server = start_webserver();

    xTaskCreate(vTaskFunction, "vTaskFunction_loop", 16 * 1024, NULL, 0, NULL);

    __MSX_PRINT__("init done");

}




