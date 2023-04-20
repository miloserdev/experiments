#ifndef __MSX_DEBUG_INIT__
#define __MSX_DEBUG_INIT__


#ifndef __ESP_FILE__
#define __ESP_FILE__    NULL
#endif

#include <stdint.h>

#include <esp_heap_caps.h>
#include <esp_libc.h>
#include <esp_err.h>
#include <esp_wifi.h>

/* a lot of esp_get_free_heap_size */

static int32_t last_mem = 0;
static int32_t get_leak() { return esp_get_free_heap_size() - last_mem; }

static void set_mem() { last_mem = esp_get_free_heap_size(); }

#define ___MSX_FMT  "H: %d | L: %d | %s >>>"


#define __DEBUG__ 1

#ifdef __DEBUG__
#define __MSX_DEBUG__(f)   { os_printf(""___MSX_FMT" ___ %s ___ %s \t\t\t \n", esp_get_free_heap_size(), get_leak(), __FUNCTION__, #f, f == 0 ? "OK" : "ERROR"); set_mem(); }
#define __MSX_DEBUGV__(f)   { os_printf(""___MSX_FMT" ___ %s ___ \t\t\t \n", esp_get_free_heap_size(), get_leak(), __FUNCTION__, #f); f; os_printf("OK \n"); set_mem(); }
#define __MSX_PRINTF__(__format, __VA_ARGS__...) { os_printf(""___MSX_FMT" "__format" \n", esp_get_free_heap_size(), get_leak(), __FUNCTION__, __VA_ARGS__); set_mem(); }
#define __MSX_PRINT__(__format) { os_printf(""___MSX_FMT" "__format" \n", esp_get_free_heap_size(), get_leak(), __FUNCTION__); set_mem();}
#else
#define __MSX_DEBUG__(f)    { f; }
#define __MSX_DEBUGV__(f)   { f; }
#define __MSX_PRINTF__(...)
#define __MSX_PRINT__(...) 
#endif


#endif