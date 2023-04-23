#ifndef __MSX_UART_INIT__
#define __MSX_UART_INIT__


#include "FreeRTOS.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/timers.h"
#include <queue.h>
#include <task.h>

#include <esp_err.h>
#include <driver/uart.h>


#include "msx_debug.h"
#include "msx_event_loop.h"


static QueueHandle_t uart_queue;


const uart_port_t uart_port = UART_NUM_0;
const __uint32_t uart_buffer_size = (1024 * 1);
const __uint32_t uart_buffer_size_x2 = (uart_buffer_size * 2);


esp_err_t init_uart();
void uart_task(void *params);


#endif