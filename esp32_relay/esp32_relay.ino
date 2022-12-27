#define R1 32
#define R2 33
#define R3 25
#define R4 26
#define R5 27

// #include "RTCDS1307.h"
#include "WiFi.h"
#include "AsyncUDP.h"
#include "JSON.h"

#include <SD.h>
#include <Wire.h>
#include "RTClib.h"
RTC_DS1307 RTC;

DateTime _now;

//RTCDS1307 rtc(0x68);

AsyncUDP udp;
const char* ssid = "********";
const char* password = "********";


JSONVar buffer;
JSONVar sched_cmd;
uint32_t scheduled_time;
bool scheduled_isset = false;


uint8_t year, month, weekday, day, hour, minute, second;
bool period = 0;

void get_time() {
    char row[128];
    _now = RTC.now();
    Serial.print("print=");
    Serial.println(_now.unixtime());
    snprintf(row,sizeof(row), "printf=%lu", _now.unixtime());
    Serial.println(row);
}


void setup() {

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
    if (WiFi.waitForConnectResult() != WL_CONNECTED) {
        Serial.println("WiFi Failed");
        while (1) {
            delay(1000);
        }
    }
    if (udp.listen(64000)) {
        Serial.println("UDP connected");
        udp.onPacket([](AsyncUDPPacket packet) {
            //reply to the client
        //    packet.printf("Got %u bytes of data", packet.length());

            buffer = JSON.parse( (char*) packet.data());

            Serial.println(buffer);

            char *ret = exec_packet(buffer);
            packet.print( ret);

//            parse_packet(buffer, packet);


        });
        //Send unicast
        udp.print("Hello Server!");
    }

}

void loop() {
    exec_scheduler();
//    print_time();
}

/*
int prttm = 0;
void print_time () {
    int now = _now.unixtime();
    if (now > prttm) {
        prttm = now + 5;
        Serial.println(now);
    }
}
*/



char* parse_packet(JSONVar &data, AsyncUDPPacket &packet) {
    Serial.println("parse packet");
    char *ret = exec_packet(data);
    packet.print( ret);
}


void exec_scheduler() {
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


void scheduled(JSONVar &command, int next_time) {
    int current_time = millis();
    scheduled_time = current_time + next_time;
    Serial.println(command);

    sched_cmd = command;
    scheduled_isset = true;

    Serial.println("Scheduled is set");
    Serial.println(sched_cmd);
}


void scheduled_clear() {
    scheduled_time = 0;
    sched_cmd = "";
    scheduled_isset = false;

    Serial.println("Scheduled is clear");
}


char * exec_packet(JSONVar &pack) {
    Serial.println("exec packet begin");

    if (JSON.typeof(pack) == "undefined") {
        Serial.println("parser malfunction");
        return "parser malfunction"; 
    }

    if (pack.length() < 0) {
        return "data misfunction";
    }

    char *ret_ = "";

    Serial.print(pack.length());

    for (uint32_t i = 0; i < pack.length(); i++) {
        JSONVar data = pack[i];
        Serial.println(data);
    //    Serial.println(data["pin"]);
    //    Serial.println( data.hasOwnProperty("schedule") );
    //    Serial.println( data.hasOwnProperty("pin") );

        if ( data.hasOwnProperty("schedule") ) {
            Serial.println("schedule");

            if ( JSON.typeof(data["schedule"]) == String("string") ) {
                Serial.println("is string");

                if ( data.hasPropertyEqual("schedule", "state") ) {
                    Serial.println("getting state schedule");
                    ret_ = (char*) (scheduled_isset ? "scheduled" : "not scheduled");
                    continue;
                }

                if ( data.hasPropertyEqual("schedule", "clear") ) {
                    scheduled_clear();
                    ret_ = "ok; scheduler is clear";
                    continue;
                }

            } else if ( JSON.typeof(data["schedule"]) == String("object") ) {

                if ( JSONVar(data["schedule"]).hasOwnProperty("time") &&
                    JSONVar(data["schedule"]).hasOwnProperty("cmd") ) {
                        
                        int tme = data["schedule"]["time"];
                        JSONVar cmd = data["schedule"]["cmd"];
                        scheduled(cmd, tme);

                        ret_ = "ok; scheduler is set";
                        continue;
                } else {
                    return "error; no time or cmd parameter";
                }
            }

        }

        if ( data.hasOwnProperty("digitalWrite") ) {
            if ( JSONVar(data["digitalWrite"]).hasOwnProperty("pin") &&
                JSONVar(data["digitalWrite"]).hasOwnProperty("value") ) {
                                        
                    int pin = data["digitalWrite"]["pin"];
                    int val = data["digitalWrite"]["value"];
                    digitalWrite(pin, val);

                    Serial.println("digitalWrite");
                    ret_ = "ok; digitalWrite";
                    continue;
                }
        }

        if ( data.hasOwnProperty("digitalRead") ) {
            if ( JSONVar(data["digitalRead"]).hasOwnProperty("pin") ) {
                                        
                    int pin = data["digitalRead"]["pin"];
                    int val = digitalRead(pin);
                    ret_ = (char*) (val ? "off" : "on");

                    Serial.println("digitalRead");
                    continue;
                }
        }
    }
    return ret_;
}

