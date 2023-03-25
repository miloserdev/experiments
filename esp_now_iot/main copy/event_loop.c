#include <tcpip_adapter.h>
#include <esp_wifi_types.h>
#include <cJSON.h>

#include "executor.c"

static xQueueHandle event_loop_queue;

void event_loop(void *params)
{

    msx_event_t evt; //malloc( sizeof( msx_event_t ) );
    //memset( evt, 0, sizeof( msx_event_t ) );

    vTaskDelay(5000 / portTICK_RATE_MS);

    while( xQueueReceive( event_loop_queue, &evt, portMAX_DELAY ) == pdTRUE )
    {
        switch( evt.id )
        {
            case MSX_ESP_NOW_SEND_CB:
            {
                os_printf( "event_loop >> MSX_ESP_NOW_SEND_CB \n" );
                break;
            }
            case MSX_ESP_NOW_RECV_CB:
            {
                uint8_t *data = evt.data;
                char *datas = /*may cause crash*/ os_malloc(evt.len);
                for (int i = 0; i < evt.len - 1; i++)
                {
                    datas[i] = data[i];
                    os_printf("%c", data[i]);
                }

                cJSON *pack = cJSON_Parse(datas);
                if ( cJSON_IsInvalid(pack) )
                {
                    os_printf("recv_cb >> json receive malfunction \n");
                    return;
                }
                char *exec_data = exec_packet(pack);

                os_printf("] \n");
                os_printf( "event_loop >> MSX_ESP_NOW_RECV_CB \n" );
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
                break;
            }
            default:
            {
                os_printf( "event_loop >> UNKNOWN_EVENT %d \n", evt.id );
                break;
            }
        }
    }
}