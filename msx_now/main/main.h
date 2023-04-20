/**
 * @ Author: miloserdev
 * @ Create Time: 2023-04-20 16:56:48
 * @ Modified by: miloserdev
 * @ Modified time: 2023-04-20 19:07:16
 * @ Description:
 */

#ifndef __MSX_MAIN__
#define __MSX_MAIN__


#include <string.h>
#include <sys/param.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_event.h"
/* #include "protocol_examples_common.h" */
#include "nvs.h"
#include "nvs_flash.h"

#include <driver/uart.h>
#include <driver/gpio.h>

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>

#include <netinet/in.h>
#include <lwip/inet.h>

#include <esp8266/eagle_soc.h>  // ????????


#include "msx_debug.h"
#include "msx_event_loop.h"
#include "msx_uart.c"
#include "msx_wifi.h"
#include "msx_ota.c"
#include "msx_executor.h"
#include "msx_espnow.h"
#include "msx_espnow.h"
#include "msx_utils.h"


void app_loop();
void app_main();


#endif