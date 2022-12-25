#define R1 4
#define R2 0
#define R3 2
#define R4 15

#include "RTCDS1307.h"
#include "WiFi.h"
#include "AsyncUDP.h"
#include "JSON.h"

RTCDS1307 rtc(0x68);

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
    rtc.getDate(year, month, day, weekday);
    rtc.getTime(hour, minute, second, period);
}


void setup() {

    rtc.begin();
    get_time();

    pinMode(R1, OUTPUT);
    pinMode(R2, OUTPUT);
    pinMode(R3, OUTPUT);
    pinMode(R4, OUTPUT);

    digitalWrite(R1, HIGH);
    digitalWrite(R2, HIGH);
    digitalWrite(R3, HIGH);
    digitalWrite(R4, HIGH);

    pinMode(13, INPUT);


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
            packet.printf("Got %u bytes of data", packet.length());

            buffer = JSON.parse( (char*) packet.data());

            Serial.println(buffer);

            parse_packet(buffer, packet);


        });
        //Send unicast
        udp.print("Hello Server!");
    }

}

void loop() {
    exec_scheduler();
}




char* parse_packet(JSONVar data, AsyncUDPPacket packet) {
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


void scheduled(JSONVar command/*uint8_t command[128], size_t sz*/, int next_time) {
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


char * exec_packet(JSONVar data) {
    Serial.println("exec packet begin");

//    Serial.println(data["pin"]);
    Serial.println( data.hasOwnProperty("schedule") );
    Serial.println( data.hasOwnProperty("pin") );

    if ( data.hasOwnProperty("schedule") ) {
        Serial.println("schedule");

        if ( JSON.typeof(data["schedule"]) == String("string") ) {
            Serial.println("is string");

            if ( data.hasPropertyEqual("schedule", "state") ) {
                Serial.println("getting state schedule");
                return "scheduler state";
            }

            if ( data.hasPropertyEqual("schedule", "clear") ) {
                scheduled_clear();
                return "scheduler is clear";
            }

        } else if ( JSON.typeof(data["schedule"]) == String("object") ) {

            if ( JSONVar(data["schedule"]).hasOwnProperty("time") &&
                 JSONVar(data["schedule"]).hasOwnProperty("cmd") ) {
                    
                    int tme = data["schedule"]["time"];
                    JSONVar cmd = data["schedule"]["cmd"];
                    scheduled(cmd, tme);

                    return "scheduler is set";
            } else {
                return "no time or cmd parameter";
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
                return "digitalWrite";
            }
    }

    if ( data.hasOwnProperty("digitalRead") ) {
        if ( JSONVar(data["digitalRead"]).hasOwnProperty("pin") ) {
                                    
                int pin = data["digitalRead"]["pin"];
                int val = digitalRead(pin);

                Serial.println("digitalRead");
                return (char*) val;
            }
    }
}


