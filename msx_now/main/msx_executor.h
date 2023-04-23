#ifndef __MSX_EXECUTOR_INIT__
#define __MSX_EXECUTOR_INIT__


#include <esp_libc.h>
#include <esp_err.h>
#include <esp_system.h>
#include <esp_now.h>
#include <esp_http_server.h>
#include <cJSON.h>


#include "msx_debug.h"
#include "msx_utils.h"
#include "msx_event_loop.h"
#include "msx_espnow.h"


char *exec_packet(char *datas, __size_t len);


#endif