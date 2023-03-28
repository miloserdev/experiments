/////////////////////////////////////////////////////
/*
    1. Print cJSON number variables causes Guru Meditation :D;
        FIX: Need to disable "nano" formatting in menuconfig;
        make menuconfig -> Component config -> Newlib -> "nano" formatting
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
    os_printf("______ add_peer ______mac: "MACSTR"_\n", MAC2STR(mac_addr) );
    
    for (size_t i = 0; i < ESP_NOW_ETH_ALEN; i++)
    {
        os_printf("%02X:", mac_addr[i]);
    }
    os_printf("\n");
    
    size_t peer_sz = sizeof(esp_now_peer_info_t);
    esp_now_peer_info_t *peer = /*may cause crash*/ os_malloc(peer_sz);
    if (peer == NULL) {
        os_printf("______ esp_now_add_peer ______%s_\n", "Malloc peer information fail" );
        return ESP_FAIL;
    } else
    {
        os_printf("______ esp_now_add_peer ______%s_\n", "Malloc ok" );
    }
    memset(peer, 0, peer_sz);
    peer->channel = channel;
    peer->ifidx = ifidx;
    peer->encrypt = encrypt;
    if (lmk != NULL) memcpy(peer->lmk, lmk, ESP_NOW_KEY_LEN);
    memcpy(peer->peer_addr, mac_addr, ESP_NOW_ETH_ALEN);
    // peer->priv

    os_printf("______ esp_now_add_peer ______%d_\n", esp_now_add_peer(peer) );

    os_free(peer);
    os_free(mac_addr);
    os_free(lmk);
    return ESP_OK;
}









void send_packet_raw(const uint8_t *peer_addr, const uint8_t *data, size_t len)
{
    //espnow_send_param_t *msg = /*may cause crash*/ os_malloc(sizeof(espnow_send_param_t));
    //memset(msg, 0, sizeof(espnow_send_param_t));

    size_t msx_sz = sizeof(msx_message_t);
    msx_message_t *msg = os_malloc(msx_sz);
    memset(msg, 0, sizeof(msx_message_t));

    uint32_t _magic = esp_random();
    msg->magic = _magic;
    msg->len = len;
    //msg->buffer = os_malloc(CONFIG_ESPNOW_SEND_LEN);

    memset(msg->buffer, 0, CONFIG_ESPNOW_SEND_LEN);
    memcpy(msg->buffer, data, /* len */CONFIG_ESPNOW_SEND_LEN);
    memcpy(msg->mac_addr, peer_addr, ESP_NOW_ETH_ALEN);

    
    uint8_t buffer[msx_sz];
    memset(buffer, 0, msx_sz);
    memcpy(buffer, msg, msx_sz);

    if (esp_now_send(msg->mac_addr, buffer, msx_sz) != ESP_OK) {
    //          if (esp_now_send(msg->mac_addr, msg->buffer, msg->len) != ESP_OK) {
    //if (esp_now_send(broadcast_mac, data, sizeof(data)) != ESP_OK) {
        os_printf("Send error \n");
    }

    if ( !stack_exists(msg_stack, _magic) )
    {
        stack_push(msg_stack, _magic);
    } else
    {
        os_printf("WARNING BLYAT!!! send_packet_raw >> generates _magic number that same as previous \n");
    }

    os_free(msg);

    os_free(&buffer);

    os_free(data);
    os_free(peer_addr);
}

void send_packet(const uint8_t *peer_addr, const cJSON *data)
{
    char *char_data = cJSON_Print(data);
    size_t len = strlen(char_data);
    uint8_t uint8_data[len];
    
    for (size_t i = 0; i < len; i++)
    {
        uint8_data[i] = char_data[i];
    }

    os_printf("fuckin buf %s \n", char_data);
    
    cJSON *new_data = cJSON_GetArrayItem(data, 0);

    if ( !cJSON_GetObjectItemCaseSensitive(new_data, "magic") )
    {
        printf("send_packet >> no magic >> return \n");
        return;
    }

    uint32_t _magic = cJSON_GetObjectItem(new_data, "magic");
    stack_print(msg_stack);
    if ( stack_exists(msg_stack, _magic) )
    {
        os_printf( "send_packet >> message exists in stack \n" );
    } else
    {
        stack_push(msg_stack, _magic);
    }


    send_packet_raw(peer_addr, uint8_data, len);
}















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

        //    os_printf(data["pin"]);
        //    os_printf(data.hasOwnProperty("schedule"));
        //    os_printf(data.hasOwnProperty("pin"));

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

                char *datas_raw = cJSON_Print(pack); // cJSON_PrintUnformatted
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
                os_printf("parse is fuck %d \n", res);

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
                    os_printf("exex_packet >> not for me... \n");
                    
                    esp_now_del_peer(peer_addrs);
                    if (!esp_now_is_peer_exist(peer_addrs))
                    {
                        os_printf("    peer not found \n");

                        if ( add_peer(peer_addrs, /* & */(uint8_t *) CONFIG_ESPNOW_LMK, MESH_CHANNEL, ESPNOW_WIFI_IF, false) == 0)
                        {
                            os_printf("    peer added >> sending direct message \n");
                            send_packet(peer_addrs, pack);
                        }
                        else
                        {
                            os_printf("    failed to add peer >> broadcast \n");
                            send_packet(broadcast_mac, pack);
                        }
                    }
                    else
                    {
                        send_packet(peer_addrs, pack);
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
                        int pin = (gpio_num_t) cJSON_GetObjectItem(digital_write_val, "pin")->valueint;
                        int val = cJSON_GetObjectItem(digital_write_val, "value")->valueint;

                        val = (val > 1) ? (rand() % 2) : val;

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

                        os_free(pin);
                        os_free(val);
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

    //cJSON_free(buf_val);
    cJSON_Delete(buf_val);
	os_printf("exec_packet cJSON_Delete \n");


    os_free(buf_val);
    os_printf("exec_packet os_free(buf_val) \n");
    os_free(pack);
    os_printf("exec_packet os_free(pack) \n");

    os_printf("exec_packet end \n");

	return ret_;
}










































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
                os_printf( "event_loop >> MSX_ESP_NOW_INIT \n" );
                break;
            }
            case MSX_ESP_NOW_SEND_CB:
            {
                os_printf( "event_loop >> MSX_ESP_NOW_SEND_CB \n" );
                break;
            }
            case MSX_ESP_NOW_RECV_CB:
            {
                os_printf( "event_loop >> MSX_ESP_NOW_RECV_CB \n" );

            //    char *datas = /*may cause crash*/ (char *) os_malloc(evt.len);
            //    memcpy(datas, evt.data, evt.len);
                
            //    os_printf("MSX_ESP_NOW_RECV_CB >> wtf buf | size %d | buf %s \n", evt.len, datas);   // fucking memory leak
                
                cJSON *pack = cJSON_Parse( (char *) evt.data );

                char *exec_data = exec_packet(pack);
                os_free(exec_data);

                //cJSON_free(pack);
                cJSON_Delete(pack);
                os_free(pack);

                os_free(evt.data);

                break;
            }
            case WIFI_EVENT_STA_START:
            {
                os_printf( "event_loop >> MSX_WIFI_EVENT_STA_START \n" );
                break;
            }
            case WIFI_EVENT_STA_DISCONNECTED:
            {
                os_printf( "event_loop >> MSX_WIFI_EVENT_STA_DISCONNECTED \n" );
                break;
            }
            case IP_EVENT_STA_GOT_IP:
            {
                ip_event_got_ip_t *event = (ip_event_got_ip_t *)evt.data;
                os_printf( "event_loop >> MSX_IP_EVENT_STA_GOT_IP %s \n", ip4addr_ntoa(&event->ip_info.ip) );
                os_free(event);
                break;
            }
            default:
            {
                os_printf( "event_loop >> UNKNOWN_EVENT %d \n", evt.id );
                break;
            }
        }

        os_free(evt.data);
        os_free(&evt);
    }

    vTaskDelete(NULL);
}

















static void uart_task(void *params)
{
    uart_event_t evt;

    char *cmp_buf = (char *) os_malloc( uart_buffer_size );
    size_t cmp_ptr = 0;


        while ( xQueueReceive( uart_queue, (void *const) &evt, (TickType_t) portMAX_DELAY ) /*  == pdTRUE */ )
        {
            // os_printf( "uart_task >> _________%d__________ \n", evt.type);

            switch(evt.type)
            {
                case UART_DATA:
                {
                    size_t one_size = evt.size;
                    uint8_t *one_buf = (uint8_t *) os_malloc(one_size);
                    memset(one_buf, 0, one_size);

                    uart_read_bytes(uart_port, one_buf, one_size, portMAX_DELAY);

                    // uart_write_bytes(uart_port, (const char *) uart_buf, evt.size);

                     for (size_t i = 0; i < one_size; i++)
                    {
                        cmp_buf[cmp_ptr] = one_buf[i];
                        cmp_ptr++;
                    }

                    //os_printf("%s ", uart_buf);

                    if (one_buf[0] == 13)
                    {
                        os_printf("\n");
                        //uart_write_bytes(uart_port, (const char*) uart_buf, strlen(uart_buf));

                        os_printf("uart_task >> input: %s parsing ... \n", cmp_buf);

                        cJSON *tmp = cJSON_Parse(cmp_buf);

                        char *exec_tmp = exec_packet(tmp);

                        os_free(exec_tmp);

                        cJSON_Delete(tmp);

                        memset(cmp_buf, 0, uart_buffer_size);
                        os_free(cmp_buf);
                        cmp_ptr = 0;
                    }

                    os_free(one_buf);       ///     VERY IMPORTANT !!!

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
    

/*     os_free(uart_buf);
    uart_buf = NULL; */     //         memory leak;
    vTaskDelete(NULL);
}










///memory free
//esp01 33836
//esp8266 34064































bool raise_event(int id, esp_event_base_t base, esp_now_send_status_t status, void *data, size_t len)
{
    if (id < 0)
    {
        printf("ERROR >> raise_event >> no id specified \n");
        return pdFAIL;
    }
    msx_event_t *evt = (msx_event_t *) malloc( sizeof(  msx_event_t ) );
    evt->id = id;
    evt->base = (base ? base : NULL);
    evt->status = (status ? status : 0);
    evt->data = (data ? data : NULL);
    evt->len = len;
    /* os_printf("______ event_handler ______%d_\n", (  */
    bool x = (xQueueSend(event_loop_queue, evt, portMAX_DELAY) != pdTRUE);
    /*  ) ); */
    os_free(evt); // causes crash
    return pdTRUE;
}





































static void send_cb(const uint8_t *mac_addr, esp_now_send_status_t status)
{
    os_printf("send_cb >> mac: "MACSTR" status: %d \n", MAC2STR(mac_addr), status);
    raise_event(MSX_ESP_NOW_SEND_CB, NULL, status, NULL, 0);
    os_free(mac_addr);
    //os_free(evt);
}



static void recv_cb(const uint8_t *mac_addr, const uint8_t *data, int len)
{
    os_printf("recv_cb >> mac: "MACSTR" size: %d \n", MAC2STR(mac_addr), len);

    size_t msx_sz = sizeof(msx_message_t);
    msx_message_t *msg = os_malloc(msx_sz);
    memset(msg, 0, msx_sz);
    memcpy(msg, data, msx_sz);

    if ( !stack_exists(msg_stack, msg->magic) )
    {
        stack_push(msg_stack, msg->magic);
    } else
    {
        os_printf("SHITTY WARNING!!! recv_cb >> _magic is already exists in msg_stack \n");
        return;
    }

    os_printf("\n\n\nmsx_message_t through esp_now >> \n");
    os_printf("mac_addr -> "MACSTR" \n", MAC2STR(msg->mac_addr));
    os_printf("magic -> 0x%08X \n", msg->magic);
    os_printf("size -> %d \n", msg->len);
    for (size_t i = 0; i < msg->len; i++)
    {
        os_printf("%c", msg->buffer[i]);
    }
    os_printf("\n\n\n");

    // return; // !!!!!!!!!!!!!!!!!!!!
    
    uint8_t msg_buf[msg->len];
    memcpy(msg_buf, msg->buffer, msg->len);

    if ( raise_event(MSX_ESP_NOW_RECV_CB, NULL, 0, msg_buf, msg->len) != pdTRUE )
    {
        os_printf("recv_cb >> raise_event error >> \n");
        os_free(msg->buffer);
        os_free(msg_buf);
    }

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
    os_printf("______ esp_now_init ______%d_\n", esp_now_init() );
    os_printf("______ esp_now_register_send_cb ______%d_\n", esp_now_register_send_cb(send_cb) );
    os_printf("______ esp_now_register_recv_cb ______%d_\n", esp_now_register_recv_cb(recv_cb) );
    os_printf("______ esp_now_register_recv_cb ______%d_\n", esp_now_set_pmk((uint8_t *)CONFIG_ESPNOW_PMK) );

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
    os_printf("______ tcpip_adapter_init ______%d_\n", 0 );
    os_printf("______ esp_event_loop_create_default ______%d_\n", esp_event_loop_create_default() );

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    //wifi_sta_config_t sta = WIFI_INIT_CONFIG_DEFAULT();
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = MESH_SSID_PREFIX,
            .password = MESH_PASSWD,
            .channel = MESH_CHANNEL
        },
    };
    os_printf("______ esp_wifi_init ______%d_\n", esp_wifi_init(&cfg) );
    os_printf("______ esp_event_handler_register WIFI_EVENT ______%d_\n", esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL) );
    os_printf("______ esp_event_handler_register IP_EVENT ______%d_\n", esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL) );
    os_printf("______ esp_wifi_set_storage ______%d_\n", esp_wifi_set_storage(WIFI_STORAGE_RAM) );
    os_printf("______ esp_wifi_set_mode ______%d_\n", esp_wifi_set_mode(WIFI_MODE_APSTA) );
    os_printf("______ esp_wifi_set_config ______%d_\n", esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config) );
    os_printf("______ esp_wifi_start ______%d_\n", esp_wifi_start() );
    os_printf("______ esp_wifi_set_channel ______%d_\n", esp_wifi_set_channel(MESH_CHANNEL, 0)) ;
    os_printf("______ esp_wifi_connect ______%d_\n", esp_wifi_connect() );
    os_printf("______ esp_wifi_get_mac ______%d_\n", esp_wifi_get_mac(ESP_IF_WIFI_STA, my_mac) );
    os_printf("______ esp_wifi_set_ps ______%d_\n", esp_wifi_set_ps(WIFI_PS_NONE) );

    raise_event(MSX_WIFI_EVENT_WIFI_INIT, NULL, 0, NULL, 0);
/*     msx_event_t *evt = (msx_event_t *) malloc( sizeof(  msx_event_t ) );
    evt->id = MSX_WIFI_EVENT_WIFI_INIT;
    evt->base = NULL;
    evt->data = NULL;
    os_printf("______ event_handler ______%d_\n", ( xQueueSend(event_loop_queue, evt, portMAX_DELAY) != pdTRUE ) ); */

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

    os_printf("string: %s\n", string);

	httpd_resp_send(req, string, strlen(string));

    cJSON_Delete(root); // crash warning
    os_free(root);
    os_free(string);

	os_printf("get_handler end \n");

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
    cJSON *buffer = NULL;
	os_printf("post_handler start \n");
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

	os_printf("buffer: %.*s \n", req->content_len, content);

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

	buffer = cJSON_Parse((const char *) content);
	const char *resp = (const char *) exec_packet(buffer);

	httpd_resp_send(req, resp, sizeof(resp));

	os_printf("post_handler end \n");

    os_free(resp);
    cJSON_Delete(buffer); // crash warning
    os_free(buffer);
    os_free(&content);

    os_free(&req);

	return ESP_OK;
}
httpd_uri_t uri_post = { .uri = "/",
	.method = HTTP_POST,
	.handler = &post_handler,
	.user_ctx = NULL
};


httpd_handle_t start_webserver(void)
{
	os_printf("Starting web server \n");
	httpd_config_t config = HTTPD_DEFAULT_CONFIG();
	config.server_port = PORT;

	server = NULL;

	if (httpd_start(&server, &config) == ESP_OK)
	{
		httpd_register_uri_handler(server, &uri_get);
		httpd_register_uri_handler(server, &uri_post);
		os_printf("All handlers in register \n");
	}

	return server;
}

void stop_webserver(httpd_handle_t server)
{
	os_printf("stop_webserver start \n");
	if (server)
	{
		httpd_stop(server);
	}

	os_printf("stop_webserver end \n");
}






























static void app_loop()
{

    vTaskDelay(1000 / portTICK_RATE_MS);

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

    cJSON *data;
    data = cJSON_CreateNull();
    data = cJSON_CreateArray();

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

        vTaskDelay(2000 / portTICK_RATE_MS); // app_loop();

        os_printf("esp_get_free_heap_size >> %d \n", esp_get_free_heap_size());
    }
}




















void app_main()
{
    //event_loop_queue = xQueueCreate( ESPNOW_QUEUE_SIZE, sizeof( msx_event_t ) );
    //xTaskCreate(event_loop, "vTask_event_loop", 16 * 1024, NULL, 0, NULL);
                                                // ??????


    os_printf("______ nvs_flash_init ______%d_\n", nvs_flash_init() );

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



    // wifi_init();
    // espnow_init();
    // server = start_webserver();

    xTaskCreate(vTaskFunction, "vTaskFunction_loop", 16 * 1024, NULL, 0, NULL);


    os_printf("________TASK_INIT_DONE________\n");

}