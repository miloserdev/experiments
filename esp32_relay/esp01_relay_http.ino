#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>

#include "JSON.h"

ESP8266WebServer server(8081);

#define RELAY_PIN 0
int PIN_STATE = 1;

const char* ssid = "********";
const char* password = "********";


JSONVar buffer;
/*
JSONVar sched_cmd;
uint32_t scheduled_time;
bool scheduled_isset = false;

int turn_on_hour = 18;
int turn_on_minute = 01;

int turn_off_hour = 1;
int turn_off_minute = 30;
*/


void handleNotFound()
{
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
}


void setup()
{

    pinMode(RELAY_PIN, OUTPUT);

    Serial.begin(115200);

    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
  
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("");
    Serial.print("Connected to ");
    Serial.println(ssid);
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());


     server.onNotFound(handleNotFound);

    server.on("/inline", []() {
        server.send(200, "text/plain", "this works as well");
    });

    server.on("/", []() {

        char cont2[512];
        char content[512];
        server.arg(0).toCharArray(cont2, server.arg(0).length() + 1);

        int e = 0;
        for (int i = 0; i < sizeof(content); i++) {
            if (cont2[i] != '\\') { content[e] = cont2[i]; e++; }
        }

        Serial.print("raw data -> ");
        Serial.println(content);

        buffer = JSON.parse((char*)content);

        Serial.print("json data -> ");
        Serial.println(buffer);

        const char* resp = (const char*)exec_packet(buffer);

        String tosend;
        tosend = resp;

        server.send(200, "text/plain", tosend);
    });


    server.begin();
}

void loop() {
    server.handleClient();
}


/*

void exec_scheduler()
{
    int now = millis();
    if (scheduled_isset && scheduled_time > 0) {
        if (now >= scheduled_time) {

            Serial.println("Scheduled activation");
            Serial.println(sched_cmd);

            JSONVar cmd = sched_cmd;
            exec_packet(cmd);

            scheduled_clear();
        }
    }
}


void scheduled(JSONVar& command, int next_time)
{
    int current_time = millis();
    scheduled_time = current_time + next_time;
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

*/


char* exec_packet(JSONVar& pack)
{
    Serial.println("exec packet begin");

    if (JSON.typeof(pack) == "undefined")
    {
        Serial.println("parser malfunction");
        return "parser malfunction";
    }

    if (pack.length() < 0)
    {
        return "data misfunction";
    }

    char* ret_ = "";

    Serial.print(pack.length());

    for (uint32_t i = 0; i < pack.length(); i++)
    {
        JSONVar data = pack[i];
        Serial.print("pack -> ");
        Serial.println(data);

        if (data.hasOwnProperty("relay"))
        {
            if ( JSON.typeof(data["relay"]) == String("string") )
            {
                data.hasPropertyEqual("relay", "state");
                ret_ = (char*)( PIN_STATE ? "off" : "on");
                return ret_;
            } else if ( JSON.typeof(data["relay"]) == String("number") )
            {
                int lh = data["relay"];
                PIN_STATE = lh >= 1 ? 1 : 0;
                ret_ = (char*)( PIN_STATE ? "off" : "on");
                digitalWrite(RELAY_PIN, PIN_STATE);
            }

            continue;
        }
    }
    return ret_;
}


/*

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
        Serial.println("unpacking");
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

*/
