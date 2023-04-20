#include "msx_uart.h"


esp_err_t init_uart()
{
    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };
    __MSX_DEBUG__( uart_param_config(uart_port, &uart_config)   );

    __MSX_DEBUG__( uart_driver_install(uart_port, uart_buffer_size * 2, uart_buffer_size * 2, 10, &uart_queue, 0)   );

    __MSX_DEBUG__( xTaskCreate(uart_task, "vTask_uart_task", uart_buffer_size * 2, NULL, 0, NULL)   );

    return ESP_OK;
}


void uart_task(void *params)
{
    uart_event_t event;
    uint8_t *dtmp = (uint8_t *) malloc(uart_buffer_size);
    __MSX_PRINT__("dtmp malloc(uart_buffer_size)");

        while ( xQueueReceive(uart_queue, (void *) &event, (TickType_t) portMAX_DELAY) == pdTRUE)
        {
            memset(dtmp, 0, uart_buffer_size);

            switch (event.type)
            {
                case UART_DATA:
                {
                    __MSX_PRINT__("UART_DATA");

                    uart_read_bytes(uart_port, dtmp, event.size, portMAX_DELAY);

                    uint8_t buff[event.size];
                    memset(buff, 0, event.size);
                    memcpy(buff, dtmp, event.size);

                    __MSX_PRINTF__("generating event MSX_UART_DATA with size %d >> data >> %s", event.size, buff);

                    // esp_err_t esp_now_send(const uint8_t *peer_addr, const uint8_t *data, size_t len);

                    __MSX_DEBUG__( raise_event(MSX_UART_DATA, NULL, 0, (uint8_t *) &buff, event.size) );

                    /* __MSX_DEBUGV__( os_free(&buff)  ); */

                    break;
                }

                // Event of HW FIFO overflow detected
                case UART_FIFO_OVF:
                {
                    __MSX_PRINT__("UART_FIFO_OVF");
                    uart_flush_input(uart_port);
                    xQueueReset(uart_queue);
                    break;
                }

                // Event of UART ring buffer full
                case UART_BUFFER_FULL:
                {
                    __MSX_PRINT__("UART_BUFFER_FULL");
                    uart_flush_input(uart_port);
                    xQueueReset(uart_queue);
                    break;
                }

                case UART_PARITY_ERR:
                {
                    __MSX_PRINT__("UART_PARITY_ERR");
                    break;
                }

                case UART_FRAME_ERR:
                {
                    __MSX_PRINT__("UART_FRAME_ERR");
                    break;
                }

                default:
                {
                    __MSX_PRINTF__("unknown event %d", event.type);
                    break;
                }
            }
        }

    __MSX_DEBUGV__( os_free(dtmp)       );
    __MSX_DEBUGV__( dtmp = NULL         );
    __MSX_DEBUGV__( vTaskDelete(NULL)   );
}