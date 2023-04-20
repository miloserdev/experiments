#include "msx_httpd.h"


esp_err_t init_httpd()
{
    __MSX_PRINT__("start");
	httpd_config_t config = HTTPD_DEFAULT_CONFIG();
	config.server_port = PORT;

	/* server = NULL; */

	if (httpd_start(&msx_server, &config) == ESP_OK)
	{
		httpd_register_uri_handler(msx_server, &uri_get);
		httpd_register_uri_handler(msx_server, &uri_post);
        __MSX_PRINT__("All handlers is registered");
	}

	return /* server; */ ESP_OK;
}


esp_err_t stop_httpd()
{
    return httpd_stop(msx_server);
}


esp_err_t post_handler(httpd_req_t *req)
{
    __MSX_PRINT__("start");

    /* cJSON *buffer = NULL; */
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

    __MSX_PRINTF__("buffer %.*s size: %d", req->content_len, content, req->content_len);

/*     cJSON *parsed_buffer = cJSON_Parse((const char *) content);
    size_t buf_len = cJSON_GetArraySize(parsed_buffer); */
    
    /////////////////////////////////////////////////////
    // NEED TO DISABLE "nano" formatting in menuconfig //
    /////////////////////////////////////////////////////
    
	const char *resp = (const char *) exec_packet(content, req->content_len);
	httpd_resp_send(req, resp, sizeof(resp));
    __MSX_DEBUGV__( os_free( (void *) resp)           );

    /* __MSX_DEBUGV__( cJSON_Delete(buffer)    ); */ // crash warning
    /* __MSX_DEBUGV__( os_free(buffer)         ); */
    __MSX_DEBUGV__( os_free(&content)       ); // ??? crash    
    __MSX_DEBUGV__( os_free(&req)           );

    __MSX_PRINT__("end");

	return ESP_OK;
}


esp_err_t status_get_handler(httpd_req_t *req)
{
    cJSON *root = cJSON_CreateArray();

    cJSON *sysinf = cJSON_CreateObject();

    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);

    cJSON_AddStringToObject(sysinf, "version", __VERSION__);
    cJSON_AddNumberToObject(sysinf, "model", chip_info.model);
    cJSON_AddNumberToObject(sysinf, "features", chip_info.features);
    cJSON_AddNumberToObject(sysinf, "cores", chip_info.cores);
    cJSON_AddNumberToObject(sysinf, "revision", chip_info.revision);
    cJSON_AddItemToArray(root, sysinf);

    cJSON_AddItemToArray(root, read_pin(0));
    cJSON_AddItemToArray(root, read_pin(1));
    cJSON_AddItemToArray(root, read_pin(2));

    char *string = cJSON_Print(root); // cJSON_PrintUnformatted

    __MSX_PRINTF__("string %s", string);

    httpd_resp_set_type(req, "application/json");
	httpd_resp_send(req, string, strlen(string));

    /* __MSX_DEBUGV__(  */cJSON_Delete(root)  /* ) */;
    __MSX_DEBUGV__( os_free(root)   );
    __MSX_DEBUGV__( os_free(string) );

    __MSX_PRINT__("status_get_handler end");

    // do it next day
    return ESP_OK;
}