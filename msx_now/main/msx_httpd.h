#ifndef __MSX_HTTPD_INIT__
#define __MSX_HTTPD_INIT__


#include <esp_libc.h>
#include <esp_err.h>
#include <esp_system.h>
#include <esp_http_server.h>
#include <cJSON.h>


#include "msx_debug.h"
#include "msx_utils.h"
#include "msx_executor.h"


static httpd_handle_t msx_server = NULL;
#define PORT            8066


esp_err_t init_httpd();
esp_err_t stop_httpd();
esp_err_t post_handler(httpd_req_t *req);
esp_err_t status_get_handler(httpd_req_t *req);


static httpd_uri_t uri_post = { .uri = "/", .method = HTTP_POST, .handler = post_handler, .user_ctx = NULL };
static httpd_uri_t uri_get = { .uri = "/status", .method = HTTP_GET, .handler = status_get_handler, .user_ctx = NULL };


#endif