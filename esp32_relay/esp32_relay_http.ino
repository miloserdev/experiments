
#include <esp_partition.h>
#include <esp_ota_ops.h>                // get running partition
#include <HTTPUpdateServer.h>
HTTPUpdateServer httpUpdater;

#include <WebServer.h>
WebServer httpServer(80);

#define R1 32
#define R2 33
#define R3 25
#define R4 26           
#define R5 27

// #include "RTCDS1307.h"

#include "Update.h"

#include "WiFi.h"
#include <esp_err.h>
#include <esp_event.h>
#include <esp_intr_alloc.h>
#include <esp_http_server.h>

#define PORT 8081

#include "JSON.h"

#include <SD.h>
#include <Wire.h>
#include "RTClib.h"
RTC_DS1307 RTC;
DateTime _now;

#include <TaskScheduler.h>
Scheduler runner;

#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))

const char* ssid = "Keenetic-6193";
const char* password = "DNj6KdZT";
bool connected = false;
wl_status_t __status__;
uint32_t __reconnect_time__ = 5; //seconds
uint32_t __reconnect_time_prev__ = 0;
uint32_t __reconnect_time_max__ = 120;

JSONVar buffer;
JSONVar sched_cmd;
uint32_t scheduled_time;
bool scheduled_isset = false;

int turn_on_hour = 18;
int turn_on_minute = 01;

int turn_off_hour = 1;
int turn_off_minute = 30;

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
    Serial.println(_now.unixtime());
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
    Serial.println(" ");
*/

    Serial.println(__unixtime__);
    return __unixtime__;
}
Task get_time_task(1000, TASK_FOREVER, []() { get_time(); }, &runner, true, NULL, NULL);


/*
esp_err_t get_handler(httpd_req_t* req)
{
    const char resp[] = " <html><head><meta name=\"viewport\" content=\"initial-scale=1\"></head><body>"
                        "    <style>"
                        "    body {"
                        "    display: grid;"
                        "    place-content: center;"
                        "    width: 100%;"
                        "    height: 100%;"
                        "    background-color: #2e2e2e; }"
                        "    * { box-sizing: border-box;"
                        "    min-width: 50%;"
                        "    min-height: 20%;"
                        "    border-radius: 20px;"
                        "    padding: 20px;"
                        "    text-align: center;"
                        "    font-size: 1.5em;"
                        "    font-family: \"sans\";"
                        "    }"
                        "    </style>"
                        "    <script>"
                        "    function send_data(e, datas) {"
                        "        fetch('http://192.168.1.65:80', {"
                        "        method: 'POST',"
                        "        headers: {"
                        "            'Accept': 'application/json',"
                        "            'Content-Type': 'application/json'"
                        "        },"
                        "        body: JSON.stringify( datas )"
                        "        })"
                        "        .then(response => response.text())"
                        "        .then(response => {"
                        "        document.getElementById(\"status_label\").style.backgroundColor = (response == \"on\" ? \"#4cd964\" : \"#ff3b30\")"
                        "        });"
                        "    }"
                        "    </script>"
                        "    <h1 style=\"background-color: #ff9500;\" onclick='send_data(this, [{ \"digitalRead\": { \"pin\": 32 } }] );' id=\"status_label\">State</h1>"
                        "    <a style=\"background-color: #4cd964;\" onclick='send_data(this, [{ \"digitalWrite\": { \"pin\": 32, \"value\": 0 } }] )'>on</a>"
                        "    <a style=\"background-color: #ff3b30;\" onclick='send_data(this, [{ \"digitalWrite\": { \"pin\": 32, \"value\": 1 } }] )'>off</a>"
                        "    </body></html>";
    httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}
*/

esp_err_t post_handler(httpd_req_t* req)
{
    Serial.println("new client");
    char content[512];
    size_t recv_size = MIN(req->content_len, sizeof(content));

    int ret = httpd_req_recv(req, content, recv_size);
    if (ret <= 0) {
        if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
            httpd_resp_send_408(req);
        }
        return ESP_FAIL;
    }

    buffer = JSON.parse((char*)content);
    Serial.println(buffer);
    const char* resp = (const char*)exec_packet(buffer);

    httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

//pure ESP variant // encryption type multipart/form-data //temporary solutiuon
static esp_err_t http_handle_ota(httpd_req_t *req)
{
    const esp_partition_t *part;
    esp_ota_handle_t handle;
    char buf[1024];
    int total_size;
    int recv_size;
    int remain;
 
    Serial.println("OTA update begin");
 
    ESP_ERROR_CHECK(httpd_resp_set_type(req, "text/plain"));
 
    part = esp_ota_get_next_update_partition(NULL);
 
    total_size = req->content_len;
 
    ESP_LOGI(TAG, "Sent size: %d KB.", total_size / 1024);
 
    ESP_ERROR_CHECK(esp_ota_begin(part, total_size, &handle));
    remain = total_size;
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
    }
    ESP_ERROR_CHECK(esp_ota_end(handle));
    ESP_ERROR_CHECK(esp_ota_set_boot_partition(part));
 
    esp_restart();
 
    return ESP_OK;
}

//arduino-esp32 Update variant
esp_err_t update_handler(httpd_req_t* req)
{
    size_t recv_size = req->content_len;
    char content[recv_size];
    
    int ret = httpd_req_recv(req, content, recv_size);
    if (ret <= 0) {
        if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
            httpd_resp_send_408(req);
        }
        return ESP_FAIL;
    }

    uint8_t buff8[recv_size] = { 0 };
    esp_err_t res = ESP_OK;
    
//    res = httpd_resp_set_type(req, _STREAM_CONTENT_TYPE);
//    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");

    const esp_partition_t *next_partition = esp_ota_get_next_update_partition(NULL);
    const esp_partition_t *configured = esp_ota_get_boot_partition();
    const esp_partition_t *running  = esp_ota_get_running_partition();
    size_t ota_size = OTA_SIZE_UNKNOWN;
    esp_ota_handle_t ota_handle = 0;

    uint32_t maxSketchSpace = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
    Update.begin(maxSketchSpace, U_FLASH);

    for (uint32_t i = 0; i < recv_size; i++) {
        buff8[i] = uint8_t (content[i]);
       
    }

    if (Update.write(buff8, recv_size) != recv_size) {
        Serial.println("error");
    }
/*
    esp_err_t rz = esp_ota_begin(next_partition, ota_size, &ota_handle);
    esp_ota_write( ota_handle, (const void *)data, recv_size);

    if (rz == ESP_OK) {
        Serial.println("update ok");
        esp_restart();
    }
*/
    
    //updateFirmware(firmware, recv_size);

    const char* resp = "OK";
    httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);
    
    return ESP_OK;
}


/*
httpd_uri_t uri_get = {
    .uri = "/",
    .method = HTTP_GET,
    .handler = get_handler,
    .user_ctx = NULL
};
*/

httpd_uri_t uri_post = {
    .uri = "/",
    .method = HTTP_POST,
    .handler = post_handler,
    .user_ctx = NULL
};


httpd_uri_t update_post = {
    .uri = "/update",
    .method = HTTP_POST,
    .handler = http_handle_ota,
    .user_ctx = NULL
};


httpd_handle_t start_webserver(void)
{
    Serial.println("start_webserver");
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = PORT;

    httpd_handle_t server = NULL;

    if (httpd_start(&server, &config) == ESP_OK) {
        Serial.println("starting...");
    //    httpd_register_uri_handler(server, &uri_get);
        httpd_register_uri_handler(server, &uri_post);
        httpd_register_uri_handler(server, &update_post);
        Serial.println("all handlers in register");
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

    //    rtc.begin();
    Wire.begin();
    RTC.begin();
    get_time();

    pinMode(R1, OUTPUT);
    pinMode(R2, OUTPUT);
    pinMode(R3, OUTPUT);
    pinMode(R4, OUTPUT);
    pinMode(R5, OUTPUT);

    digitalWrite(R1, HIGH);
    digitalWrite(R2, HIGH);
    digitalWrite(R3, HIGH);
    digitalWrite(R4, HIGH);
    digitalWrite(R5, HIGH);

    //    pinMode(13, INPUT); //thermal;

    Serial.begin(115200);

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
    Serial.println(";");

    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);

    Serial.print("Connecting to WiFi ..");
    while (WiFi.status() != WL_CONNECTED) {
        Serial.print('.');
        delay(1000);
    }

    start_webserver();
    
    httpUpdater.setup(&httpServer);
    httpServer.begin();

    /*
    if (udp.listen(80)) {
        Serial.println("UDP connected");
        udp.onPacket([](AsyncUDPPacket packet) {
            buffer = JSON.parse( (char*) packet.data());
            Serial.println(buffer);
            char *ret = exec_packet(buffer);
            packet.print( ret);
        });
        //Send unicast
        udp.print("Hello Server!");
    }
*/
}


void loop()
{
    runner.execute();
    wifi_reconnect();
    httpServer.handleClient();
    //    print_time();
}


void wifi_reconnect()
{
    __status__ = WiFi.status();
    if ((__status__ != WL_CONNECTED)) {
        if ((__unixtime__ - __reconnect_time_prev__ >= __reconnect_time__)) {
            Serial.println("reconnecting...");
            WiFi.disconnect();
            WiFi.reconnect();

            __reconnect_time_prev__ = __unixtime__;
            __reconnect_time__ += 5;
            if (__reconnect_time__ >= __reconnect_time_max__) {
                ESP.restart();
            }
        }
    } else {
        __reconnect_time__ = 5;        
    }
}
//Task wifi_reconnect_task(15000, TASK_FOREVER, wifi_reconnect, &runner, true, NULL, NULL);


/// WARNING
/// HARDCODE ;)

Task time_toggler(30000, TASK_FOREVER, []() {

    if (hour == turn_off_hour && minute == turn_off_minute) {
        digitalWrite(32, HIGH);
        Serial.println("turned off");
    }
    else if (hour == turn_on_hour && minute == turn_on_minute) {
        digitalWrite(32, LOW);
        Serial.println("turned on");
    }

}, &runner, true, NULL, NULL);


void exec_scheduler()
{
    Serial.println("unixtime");
    if (scheduled_isset && scheduled_time > 0) {
        if (__unixtime__ >= scheduled_time) {

            Serial.println("Scheduled activation");
            Serial.println(sched_cmd);

            JSONVar cmd = sched_cmd;
            exec_packet(cmd);

            scheduled_clear();
        }
    }
}
Task exec_scheduler_task(1000, TASK_FOREVER, exec_scheduler, &runner, true, NULL, NULL);

void scheduled(JSONVar& command, int next_time)
{
    scheduled_time = __unixtime__ + next_time;
    Serial.println(command);

    sched_cmd = command;
    scheduled_isset = true;

    Serial.println("Scheduled is set");
    Serial.println(sched_cmd);
}

void scheduled_clear()
{
    scheduled_time = 0;
    sched_cmd = "";
    scheduled_isset = false;

    Serial.println("Scheduled is clear");
}

char* exec_packet(JSONVar& pack)
{
    Serial.println("exec packet begin");

    if (JSON.typeof(pack) == "undefined") {
        Serial.println("parser malfunction");
        return "parser malfunction";
    }

    if (pack.length() < 0) {
        return "data misfunction";
    }

    char* ret_ = "";

    Serial.print(pack.length());

    for (uint32_t i = 0; i < pack.length(); i++) {
        JSONVar data = pack[i];
        Serial.println(data);
        //    Serial.println(data["pin"]);
        //    Serial.println( data.hasOwnProperty("schedule") );
        //    Serial.println( data.hasOwnProperty("pin") );

        if (data.hasOwnProperty("schedule")) {
            Serial.println("schedule");

            if (JSON.typeof(data["schedule"]) == String("string")) {
                Serial.println("is string");

                if (data.hasPropertyEqual("schedule", "state")) {
                    Serial.println("getting state schedule");
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

                Serial.println("digitalWrite");
                ret_ = (char*)(val ? "off" : "on");
                continue;
            }
        }

        if (data.hasOwnProperty("digitalRead")) {
            if (JSONVar(data["digitalRead"]).hasOwnProperty("pin")) {

                int pin = data["digitalRead"]["pin"];
                int val = digitalRead(pin);
                ret_ = (char*)(val ? "off" : "on");

                Serial.println("digitalRead");
                continue;
            }
        }
    }
    return ret_;
}
