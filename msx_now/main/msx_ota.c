#ifndef __MSX_OTA_INIT__
#define __MSX_OTA_INIT__


#include <stdint.h>

#include <esp_heap_caps.h>
#include <esp_libc.h>
#include <esp_err.h>
#include <esp_wifi.h>
#include <esp_ota_ops.h>
#include <esp_http_server.h>
#include <esp_partition.h>

#include "msx_debug.c"
#include "msx_httpd.c"

esp_err_t init_ota();
static esp_err_t http_handle_ota(httpd_req_t *req);
#define OTA_BUFFER_SIZE     1024


httpd_uri_t uri_ota_post = { .uri = "/update", .method = HTTP_POST, .handler = &http_handle_ota, .user_ctx = NULL };


esp_err_t init_ota()
{
    return httpd_register_uri_handler(msx_server, &uri_ota_post);
}

// curl 192.168.1.89:8066/update --no-buffer --data-binary @./build/msx.bin --output -
static esp_err_t http_handle_ota(httpd_req_t *req)
{

    const esp_partition_t *part;
	esp_ota_handle_t handle;
	char buf[OTA_BUFFER_SIZE];
	int total_size;
	int recv_size;
    size_t offset = 0;
	int remain;
	uint8_t percent;
    esp_err_t err;

	__MSX_DEBUG__( httpd_resp_set_type(req, "text/plain") );
	__MSX_DEBUG__( httpd_resp_send_chunk(req, "Start to update firmware.\n", 100) );

    //      WARNING!!!          make sure that your partition table contains OTA    ;
    //      make menuconfig > Partition Tables
	//		and do not forget to ./fla.sh boards with new partitions
    part = esp_ota_get_next_update_partition(NULL);

    total_size = req->content_len;

	__MSX_PRINTF__("Sent size: %d KB. \n", total_size / 1024);

/* 	ESP_ERROR_CHECK(httpd_resp_sendstr_chunk(req, "0        20        40        60        80       100%\n"));
	ESP_ERROR_CHECK(httpd_resp_sendstr_chunk(req, "|---------+---------+---------+---------+---------+\n"));
	ESP_ERROR_CHECK(httpd_resp_sendstr_chunk(req, "*")); */

	__MSX_DEBUG__( esp_ota_begin(part, total_size, &handle) );
    //                  fuck it     ;

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
		if (recv_size <= 0)
		{
			if (recv_size == HTTPD_SOCK_ERR_TIMEOUT)
			{
				continue;
			}
/*             httpd_resp_send_chunk(req, "Failed to receive firmware.", 100);
			httpd_resp_send_500(req); */
            __MSX_PRINT__("update failed!");
            vTaskDelay(5000 / portTICK_RATE_MS);
			return ESP_FAIL;
		}

		err = esp_ota_write(handle, (const void*) buf, recv_size);
        //err = esp_partition_write(part, offset, (const void *) buf, recv_size);
        if (err != ESP_OK)
        {
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
	__MSX_PRINT__("update successful");

	httpd_resp_send_chunk(req, "*\nOK\n", 2);
	httpd_resp_send_chunk(req, NULL, 1);

    vTaskDelay(5000 / portTICK_RATE_MS);

	esp_restart();

	return ESP_OK;
}


#endif