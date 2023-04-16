#ifndef __MSX_OTA_INIT__
#define __MSX_OTA_INIT__


#include <stdint.h>

#include <esp_heap_caps.h>
#include <esp_libc.h>
#include <esp_err.h>
#include <esp_wifi.h>
#include <esp_ota_ops.h>
#include <esp_http_server.h>
#include <esp_http_client.h>
#include <esp_partition.h>
#include <esp_https_ota.h>

#include "msx_debug.c"
#include "msx_httpd.c"


#define OTA_RESTART_AFTER_FAILED_UPDATE 1
#define OTA_FIRMWARE_URL "http://192.168.1.123:8091/firmware/msx.bin"

esp_err_t init_ota();
esp_err_t stop_others();
static esp_err_t http_handle_ota(httpd_req_t *req);
static esp_err_t http_handle_ota_from_git(httpd_req_t *req);
#define OTA_BUFFER_SIZE     1024


httpd_uri_t uri_ota_post = { .uri = "/update", .method = HTTP_POST, .handler = &http_handle_ota, .user_ctx = NULL };
httpd_uri_t uri_ota_from_git_get = { .uri = "/update_git", .method = HTTP_GET, .handler = &http_handle_ota_from_git, .user_ctx = NULL };


esp_err_t init_ota()
{
    __MSX_DEBUG__( httpd_register_uri_handler(msx_server, &uri_ota_post) );
	__MSX_DEBUG__( httpd_register_uri_handler(msx_server, &uri_ota_from_git_get) );
	return ESP_OK;
}

esp_err_t stop_others()
{
	__MSX_DEBUG__( httpd_unregister_uri_handler(msx_server, uri_get.uri, uri_get.method) );
	__MSX_DEBUG__( httpd_unregister_uri_handler(msx_server, uri_post.uri, uri_post.method) );
    __MSX_PRINT__("All handlers is registered");
	return ESP_OK;
}


esp_err_t http_handle_ota_from_git(httpd_req_t *req)
{
	char *resp = "ok";
	httpd_resp_send(req, resp, strlen(resp));
	esp_err_t err = ESP_OK;

    esp_http_client_config_t config = {
        .url = OTA_FIRMWARE_URL,
        //.cert_pem = (char *)server_cert_pem_start,
    };

    err = esp_https_ota(&config);

	if (err != ESP_OK)
	{
		__MSX_PRINT__("update failed!");
		vTaskDelay(5000 / portTICK_RATE_MS);
		#ifdef OTA_RESTART_AFTER_FAILED_UPDATE
			goto _reload;
		#endif
	}

_reload:
	vTaskDelay(1000 / portTICK_RATE_MS);
	esp_restart();

	return err;
}


/* 
	i tested it on my LoLin NodeMCU V3 and it works great,
	and i dont know why, but it`s not working on ESP-01 board.

	p.s. just in case, try to edit partitions table
 */
// curl 192.168.1.101:8066/update --no-buffer --data-binary @./build/msx.bin --output -
static esp_err_t http_handle_ota(httpd_req_t *req)
{
    esp_err_t err;

    const esp_partition_t *part;
	esp_ota_handle_t handle;
	char buf[OTA_BUFFER_SIZE];
	int total_size;
	int recv_size;
    size_t offset = 0;
	int remain;
	uint8_t percent;

	__MSX_DEBUG__( httpd_resp_set_type(req, "text/plain") );
	__MSX_DEBUG__( httpd_resp_send_chunk(req, "Start to update firmware.\n", 100) );

    //      WARNING!!!          make sure that your partition table contains OTA    ;
	/* 
			# Name,   Type, SubType, Offset,   Size, Flags
			...
			ota_0,    0,    ota_0,   0x10000, 0xF0000
	*/
    //      make menuconfig > Partition Tables
	//		and do not forget to ./fla.sh boards with new partitions
    part = esp_ota_get_next_update_partition(NULL);

    total_size = req->content_len;

	__MSX_PRINTF__("Sent size: %d KB. \n", total_size / 1024);

/* 	ESP_ERROR_CHECK(httpd_resp_sendstr_chunk(req, "0        20        40        60        80       100%\n"));
	ESP_ERROR_CHECK(httpd_resp_sendstr_chunk(req, "|---------+---------+---------+---------+---------+\n"));
	ESP_ERROR_CHECK(httpd_resp_sendstr_chunk(req, "*")); */

	//__MSX_DEBUG__( stop_others() );
	__MSX_DEBUG__( esp_ota_begin(part, OTA_SIZE_UNKNOWN, &handle) );
	//__MSX_DEBUG__( esp_ota_begin(part, total_size, &handle) );

    remain = total_size;
    percent = 2;

	while (remain > 0)
	{
		if (remain < sizeof(buf))
		{
			recv_size = remain;
		}
		else
		{
			recv_size = sizeof(buf);
		}

		recv_size = httpd_req_recv(req, buf, recv_size);
		__MSX_PRINTF__("%d bytes received <<< %.*s >>>", recv_size, recv_size, buf);
		if (recv_size <= 0)
		{
			if (recv_size == HTTPD_SOCK_ERR_TIMEOUT)
			{
				continue;
			}
/*             httpd_resp_send_chunk(req, "Failed to receive firmware.", 100);
			httpd_resp_send_500(req); */
            __MSX_PRINT__("update failed!");
			err = ESP_FAIL;
            vTaskDelay(5000 / portTICK_RATE_MS);
			#ifdef OTA_RESTART_AFTER_FAILED_UPDATE
				goto _reload;
			#endif
			//return ESP_FAIL;
		}

		err = esp_ota_write(handle, (const void*) buf, recv_size);
        //err = esp_partition_write(part, offset, (const void *) buf, recv_size);
        if (err != ESP_OK)
        {
			__MSX_PRINTF__("ota error %d", err);
            return ESP_ERR_OTA_BASE;
        }

		remain -= recv_size;
		if (remain < (total_size *(100 - percent) / 100))
		{
			httpd_resp_send_chunk(req, "*", 1);
			percent += 2;
		}
	}

	__MSX_DEBUG__( esp_ota_end(handle) );
	__MSX_DEBUG__( esp_ota_set_boot_partition(part) );

	httpd_resp_send_chunk(req, "*\nOK\n", 2);
	httpd_resp_send_chunk(req, NULL, 1);
	__MSX_PRINT__("update successful");

_reload:
	vTaskDelay(1000 / portTICK_RATE_MS);
	esp_restart();

	return err;
}


#endif