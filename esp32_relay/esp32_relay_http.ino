#define R1 32
#define R2 33
#define R3 25
#define R4 26

#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))

// #include "RTCDS1307.h"
//#include "Update.h"
//#include "WiFi.h"
#include <stdio.h>
#include <esp_err.h>
#include <esp_system.h>
#include <esp_event.h>
#include <esp_event_loop.h>
#include <esp_event_legacy.h>
#include <esp_intr_alloc.h>

static const char *TAG = "relay_array_1";

#include <esp_wifi.h>
#include <esp_wifi_types.h>
#include <tcpip_adapter.h>
#define WIFI_SSID "********"
#define WIFI_PASSWORD "********"

#include <esp_http_server.h>
#define PORT 8081

#include <esp_partition.h>
#include <esp_ota_ops.h>    // get running partition
#define OTA_BUF_SIZE 1024

#include <esp_sntp.h>
#define SNTP_SERVER "pool.ntp.org"
//#define SNTP_SYNC_INTERVAL ((60 * 60) * 1000) * 12 // 12 hours;
#define SNTP_SYNC_INTERVAL (((60 * 60) * 1000) *  24) * 7 // 7 days;

#include "JSON.h"
#include <SD.h>
#include <Wire.h>
#include "RTClib.h"
RTC_DS1307 RTC;
DateTime _now;

#include <TaskScheduler.h>
Scheduler runner;


//bool connected = false;
//wl_status_t wifi_status;

//uint32_t reconnect_time = 5; //seconds
//uint32_t reconnect_time_prev = 0;
//uint32_t reconnect_time_max = 120;

JSONVar buffer;
JSONVar sched_cmd;
uint32_t scheduled_time;
bool scheduled_isset = false;

int turn_on_hour = 17;
int turn_on_minute = 1;

int turn_off_hour = 1;
int turn_off_minute = 1;

uint8_t year, month, weekday, day, hour, minute, second;
uint32_t __unixtime__;  //seconds!
bool period = 0;

uint32_t get_time()
{
    _now = RTC.now();

    year = _now.year();
    month = _now.month();
    weekday = _now.dayOfTheWeek();
    day = _now.day();
    hour = _now.hour();
    minute = _now.minute();
    second = _now.second();
    __unixtime__ = _now.unixtime();

/*
    Serial.print("unixtime=");
    printf(_now.unixtime());
    Serial.print("-");
    Serial.print(day);
    Serial.print(" of ");
    Serial.print(month);
    Serial.print(" ");
    Serial.print(hour);
    Serial.print(":");
    Serial.print(minute);
    Serial.print(":");
    Serial.print(second);
    printf(" ");

    Serial.print(">");
    Serial.print(day);
    Serial.print("/");
    Serial.print(month);
    Serial.print("/");
    Serial.print(year);
    Serial.print("-");
    Serial.print(hour);
    Serial.print(":");
    Serial.print(minute);
    Serial.print(":");
    Serial.print(second);
    printf(";");
*/

    //printf("time -> %d \n", __unixtime__);
    return __unixtime__;
}
Task get_time_task(1000, TASK_FOREVER, []() { get_time(); }, &runner, true, NULL, NULL);

void sntp_update_rtc(struct timeval *tv) {
    printf("SNTP RTC update event \n");
    struct DateTime dt = DateTime(tv->tv_sec);
    RTC.adjust(dt);
}

void update_time_sntp() {

    setenv("TZ", "Europe/Moscow", 1);
    tzset();

    if(sntp_enabled()){
        sntp_stop();
    }

    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_servermode_dhcp(1);
    sntp_setservername(0, SNTP_SERVER);
    sntp_set_time_sync_notification_cb(&sntp_update_rtc);
    sntp_set_sync_mode(SNTP_SYNC_MODE_SMOOTH);
//    sntp_set_sync_interval(10000); //deprecated
    sntp_init();

    printf("SNTP try fetch time");

//  time_t now = 0;
//  struct tm timeinfo = { 0 };

    uint32_t retry = 0;
    uint32_t retries = 10;
    while(sntp_get_sync_status() == SNTP_SYNC_STATUS_RESET && ++retry < retries) {
        printf(".");
    }
    printf("SNTP fetch end \n");

//    time(&now);
//    localtime_r(&now, &timeinfo);

/*
    Serial.print("Now: ");
    printf(now);
    Serial.print("TM_MDAY: ");
    printf(timeinfo.tm_sec);
    printf(timeinfo.tm_min);
    printf(timeinfo.tm_hour);
    printf(timeinfo.tm_mday);
    printf(timeinfo.tm_mon);
    printf(timeinfo.tm_year);
    printf(timeinfo.tm_wday);

    printf("");
*/

}
Task get_sntp_task(SNTP_SYNC_INTERVAL, TASK_FOREVER, []() { update_time_sntp(); }, &runner, true, NULL, NULL);




esp_err_t get_time_handler(httpd_req_t* req)
{
    char buf[16];
    sprintf(buf, "%lu", __unixtime__);
    httpd_resp_send(req, buf, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

esp_err_t get_handler(httpd_req_t* req)
{
    printf("get method uri: %s \n", req->uri);
}

esp_err_t post_handler(httpd_req_t* req)
{
    char content[512];
    size_t recv_size = MIN(req->content_len, sizeof(content));

    int ret = httpd_req_recv(req, content, recv_size);
    if (ret <= 0) {
        if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
            httpd_resp_send_408(req);
        }
        return ESP_FAIL;
    }

    printf("buffer: %.*s \n", req->content_len, content);
    buffer = JSON.parse((char*)content);
    const char* resp = (const char*)exec_packet(buffer);

    httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

/*
static const char static_html[] PROGMEM =
    "<!DOCTYPE html>"
    " <html lang='en'>"
    " <head>"
    "     <meta charset='utf-8'>"
    "     <meta name='viewport' content='width=device-width,initial-scale=1'/>"
    " </head>"
    " <body>"
    " <form method='POST' action='' enctype='multipart/form-data'>"
    "     Firmware:<br>"
    "     <input type='file' accept='.bin,.bin.gz' name='firmware'>"
    "     <input type='submit' value='Update Firmware'>"
    " </form>"
    " <form method='POST' action='' enctype='multipart/form-data'>"
    "     FileSystem:<br>"
    "     <input type='file' accept='.bin,.bin.gz,.image' name='filesystem'>"
    "     <input type='submit' value='Update FileSystem'>"
    " </form>"
    " </body>"
    " </html>)";
esp_err_t get_update_handler(httpd_req_t* req)
{
    Serial.print("get method uri: ");
    printf(req->uri);
    
    httpd_resp_set_hdr(req, "Connection", "close");
    httpd_resp_send(req, static_html, HTTPD_RESP_USE_STRLEN);
    httpd_resp_set_status(req, "200 Ok");
    return ESP_OK;
}
*/

//pure ESP variant // encryption type multipart/form-data //temporary solutiuon
// thanks to @kimata from rabbit-note.com
// curl 192.168.1.73:8081/update/ --no-buffer --data-binary @./sketch_relay_new.ino.bin
static esp_err_t http_handle_ota(httpd_req_t *req)
{
    const esp_partition_t *part;
    esp_ota_handle_t handle;
    char buf[OTA_BUF_SIZE];
    int total_size;
    int recv_size;
    int remain;
    uint8_t percent;
 
    printf(TAG, "Start to update firmware. \n");
 
    ESP_ERROR_CHECK(httpd_resp_set_type(req, "text/plain"));
    ESP_ERROR_CHECK(httpd_resp_sendstr_chunk(req, "Start to update firmware.\n"));
 
    part = esp_ota_get_next_update_partition(NULL);
    // part_info_show("Target", part);
 
    total_size = req->content_len;
 
    printf(TAG, "Sent size: %d KB. \n", total_size / 1024);
 
    ESP_ERROR_CHECK(httpd_resp_sendstr_chunk(req, "0        20        40        60        80       100%\n"));
    ESP_ERROR_CHECK(httpd_resp_sendstr_chunk(req, "|---------+---------+---------+---------+---------+\n"));
    ESP_ERROR_CHECK(httpd_resp_sendstr_chunk(req, "*"));
 
    ESP_ERROR_CHECK(esp_ota_begin(part, total_size, &handle));
    remain = total_size;
    percent = 2;
    while (remain > 0) {
        if (remain < sizeof(buf)) {
            recv_size = remain;
        } else {
            recv_size = sizeof(buf);
        }
 
        recv_size = httpd_req_recv(req, buf, recv_size);
        if (recv_size <= 0) {
            if (recv_size == HTTPD_SOCK_ERR_TIMEOUT) {
                continue;
            }
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR,
                                "Failed to receive firmware.");
            return ESP_FAIL;
        }
 
        ESP_ERROR_CHECK(esp_ota_write(handle, buf, recv_size));
 
        remain -= recv_size;
        if (remain < (total_size * (100-percent) / 100)) {
            httpd_resp_sendstr_chunk(req, "*");
            percent += 2;
        }
    }
    ESP_ERROR_CHECK(esp_ota_end(handle));
    ESP_ERROR_CHECK(esp_ota_set_boot_partition(part));
    printf(TAG, "Finished writing firmware. \n");
 
    httpd_resp_sendstr_chunk(req, "*\nOK\n");
    httpd_resp_sendstr_chunk(req, NULL);

    esp_restart();
 
    // xTaskCreate(restart_task, "restart_task", 1024, NULL, 10, NULL);
 
    return ESP_OK;
}


httpd_uri_t uri_get = {
    .uri = "/",
    .method = HTTP_GET,
    .handler = get_handler,
    .user_ctx = NULL
};

httpd_uri_t uri_post = {
    .uri = "/",
    .method = HTTP_POST,
    .handler = post_handler,
    .user_ctx = NULL
};

httpd_uri_t uri_get_time = {
    .uri = "/time",
    .method = HTTP_GET,
    .handler = get_time_handler,
    .user_ctx = NULL
};

/*
httpd_uri_t uri_get_update = {
    .uri = "/update",
    .method = HTTP_GET,
    .handler = &get_update_handler,
    .user_ctx = NULL
};
*/

httpd_uri_t uri_post_update = {
    .uri = "/update/",
    .method = HTTP_POST,
    .handler = &http_handle_ota,
    .user_ctx = NULL
};


httpd_handle_t start_webserver(void)
{
    printf("Starting web server \n");
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = PORT;

    httpd_handle_t server = NULL;

    if (httpd_start(&server, &config) == ESP_OK) {
        httpd_register_uri_handler(server, &uri_get);
        httpd_register_uri_handler(server, &uri_post);
    //  httpd_register_uri_handler(server, &uri_get_update);
        httpd_register_uri_handler(server, &uri_post_update);
        httpd_register_uri_handler(server, &uri_get_time);
        printf("All handlers in register \n");
    }
    return server;
}

void stop_webserver(httpd_handle_t server)
{
    if (server) {
        httpd_stop(server);
    }
}

void setup()
{
    Serial.begin(115200);

    wifi_init();

    //    rtc.begin();
    Wire.begin();
    RTC.begin();
    get_time();

    pinMode(R1, OUTPUT);
    pinMode(R2, OUTPUT);
    pinMode(R3, OUTPUT);
    pinMode(R4, OUTPUT);

    digitalWrite(R1, HIGH);
    digitalWrite(R2, HIGH);
    digitalWrite(R3, HIGH);
    digitalWrite(R4, HIGH);

    //    pinMode(13, INPUT); //thermal;

//    WiFi.mode(WIFI_STA);
//    WiFi.begin(ssid, password);

//    Serial.print("Connecting to WiFi ");
//    while (WiFi.status() != WL_CONNECTED) {
//        Serial.print('.');
//        delay(1000);
//    }
//    printf("");

    start_webserver();
}


void loop()
{
    runner.execute();
    //wifi_reconnect();
}

esp_err_t wifi_event_handler(void *ctx, system_event_t *event)
{
    if (event != NULL)
    {
        switch (event->event_id)
        {
            case SYSTEM_EVENT_STA_START: {
                if ( esp_wifi_connect() != ESP_OK ) { printf("Cannot connect to WiFi AP \n"); }
                //esp_err_to_name_r(esp_err_t code, char *buf, size_t buflen)
                break;
            }
            case SYSTEM_EVENT_STA_GOT_IP: {
                printf("IPv4-> %s \n", ip4addr_ntoa( &event->event_info.got_ip.ip_info.ip ) );
                printf("IPv6-> %s \n", ip6addr_ntoa( &event->event_info.got_ip6.ip6_info.ip ) );
                break;
            }
            case SYSTEM_EVENT_STA_DISCONNECTED: {
                if ( esp_wifi_connect() != ESP_OK ) { printf("Cannot connect to WiFi AP \n"); }
                break;                
            }
        }
    }
}

esp_err_t wifi_init()
{
    tcpip_adapter_init();

    if ( esp_event_loop_init(wifi_event_handler, NULL) != ESP_OK ) { printf("Cannot init WiFi event loop \n"); }
    
    wifi_init_config_t wifi_init_config = WIFI_INIT_CONFIG_DEFAULT();
    if ( esp_wifi_init(&wifi_init_config) != ESP_OK ) { printf("Cannot init WiFi module \n"); }

    wifi_storage_t wifi_storage = WIFI_STORAGE_FLASH;
    if ( esp_wifi_set_storage(wifi_storage) != ESP_OK ) { printf("Cannot set WiFi storage \n"); }

    wifi_interface_t wifi_interface = WIFI_IF_STA;
    wifi_config_t wifi_config = {
        .sta = {
            { .ssid = WIFI_SSID },
            { .password = WIFI_PASSWORD },
        }        
    };
    if ( esp_wifi_set_config(wifi_interface, &wifi_config) != ESP_OK ){ printf("Cannot set WiFi config \n"); }

    wifi_mode_t wifi_mode = WIFI_MODE_STA;
    if ( esp_wifi_set_mode(wifi_mode) != ESP_OK ) { printf("Cannot set WiFi mode \n"); }

    if ( esp_wifi_start() != ESP_OK ) { printf("Cannot start WiFi \n"); }

    //if ( esp_wifi_connect() != ESP_OK ) { printf("Cannot connect to WiFi AP \n"); }
    //else { return ESP_OK; }

    return ESP_FAIL;
}

/*
void wifi_reconnect()
{
    get_time();
    wifi_status = WiFi.status();
    if ((wifi_status != WL_CONNECTED)) {
        if ((__unixtime__ - reconnect_time_prev >= reconnect_time)) {
            printf("reconnecting...");
            WiFi.disconnect();
            WiFi.reconnect();

            reconnect_time_prev = __unixtime__;
            reconnect_time += 5;
            if (reconnect_time >= reconnect_time_max) {
                esp_restart();
            }
        }
    } else {
        reconnect_time = 5;        
    }
}
Task wifi_reconnect_task(15000, TASK_FOREVER, wifi_reconnect, &runner, true, NULL, NULL);
*/

/// WARNING
/// HARDCODE ;)
Task time_toggler(20000, TASK_FOREVER, []() {

    if (hour == turn_off_hour && minute == turn_off_minute) {
        JSONVar _off = JSON.parse("[{ \"digitalWrite\": { \"pin\": 33, \"value\": 1 } }]"); 
        exec_packet(_off);
        printf("turned off \n");
    }
    else if (hour == turn_on_hour && minute == turn_on_minute) {
        JSONVar _on = JSON.parse("[{ \"digitalWrite\": { \"pin\": 33, \"value\": 0 } }]"); 
        exec_packet(_on);
        printf("turned on \n");
    }

}, &runner, true, NULL, NULL);


void exec_scheduler()
{
    get_time();
    if (scheduled_isset && scheduled_time > 0) {
        if (__unixtime__ >= scheduled_time) {

            printf("Scheduled activation \n");
            printf("Scheduled command %s \n", sched_cmd);

            JSONVar cmd = sched_cmd;
            exec_packet(cmd);

            scheduled_clear();
        }
    }
}
Task exec_scheduler_task(1000, TASK_FOREVER, exec_scheduler, &runner, true, NULL, NULL);

void scheduled(JSONVar &command, int next_time)
{
    get_time();
    scheduled_time = __unixtime__ + next_time;
    sched_cmd = command;
    scheduled_isset = true;

    printf("Scheduled is set \n");
    printf("Scheduled command %s \n", sched_cmd);
}

void scheduled_clear()
{
    scheduled_time = 0;
    sched_cmd = "";
    scheduled_isset = false;

    printf("Scheduled is clear \n");
}

char* exec_packet(JSONVar& pack)
{
    printf("exec packet begin \n");

    if (JSON.typeof(pack) == "undefined") {
        ESP_LOGE(TAG, "parser malfunction");
        return "parser malfunction";
    }

    if (pack.length() < 0) {
        return "data misfunction";
    }

    char* ret_ = "";

    printf("packet length %d \n", pack.length() );

    for (uint32_t i = 0; i < pack.length(); i++) {
        JSONVar data = pack[i];
        //printf("data: %s \n", pack[i]);
        //    printf(data["pin"]);
        //    printf( data.hasOwnProperty("schedule") );
        //    printf( data.hasOwnProperty("pin") );

        if (data.hasOwnProperty("schedule")) {
            //printf("schedule \n");

            if (JSON.typeof(data["schedule"]) == String("string")) {
                //printf("is string \n");

                if (data.hasPropertyEqual("schedule", "state")) {
                    printf("getting state schedule \n");
                    ret_ = (char*)(scheduled_isset ? "scheduled" : "not scheduled");
                    continue;
                }

                if (data.hasPropertyEqual("schedule", "clear")) {
                    scheduled_clear();
                    ret_ = "ok; scheduler is clear";
                    continue;
                }
            }
            else if (JSON.typeof(data["schedule"]) == String("object")) {

                if (JSONVar(data["schedule"]).hasOwnProperty("time") && JSONVar(data["schedule"]).hasOwnProperty("cmd")) {

                    int tme = data["schedule"]["time"];
                    JSONVar cmd = data["schedule"]["cmd"];
                    scheduled(cmd, tme);

                    ret_ = "ok; scheduler is set";
                    continue;
                }
                else {
                    return "error; no time or cmd parameter";
                }
            }
        }

        if (data.hasOwnProperty("digitalWrite")) {
            if (JSONVar(data["digitalWrite"]).hasOwnProperty("pin") && JSONVar(data["digitalWrite"]).hasOwnProperty("value")) {

                int pin = data["digitalWrite"]["pin"];
                int val = data["digitalWrite"]["value"];
                digitalWrite(pin, val);

                printf("digitalWrite \n");
                ret_ = (char*)(val ? "off" : "on");
                continue;
            }
        }

        if (data.hasOwnProperty("digitalRead")) {
            if (JSONVar(data["digitalRead"]).hasOwnProperty("pin")) {

                int pin = data["digitalRead"]["pin"];
                int val = digitalRead(pin);
                ret_ = (char*)(val ? "off" : "on");

                printf("digitalRead \n");
                continue;
            }
        }
    }
    return ret_;
}
