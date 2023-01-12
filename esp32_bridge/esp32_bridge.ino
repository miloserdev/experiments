#include <SoftwareSerial.h>
#include <TaskScheduler.h>

SoftwareSerial EspSerial(10, 11);
Scheduler runner;

void setup() {
  Serial.begin(115200);
  while (!Serial) {}
  Serial.print("Serial init OK\r\n");
  EspSerial.begin(9600);
}

void loop() {
  runner.execute();
}


Task SerialDataIO(0, TASK_FOREVER, []() {
  // Wifi -> Serial Bridge
  if (EspSerial.available()) {
    String serialMSG = EspSerial.readStringUntil('\n');
    Serial.println(serialMSG);
  }
  // Serial -> WiFi Bridge
  if (Serial.available()) {
    String serialMSG = Serial.readStringUntil('\n');
    EspSerial.println(serialMSG);
  }
}, &runner, true, NULL , NULL );

Task TemperatureSender(5700, TASK_FOREVER, []() {
  //Serial.println(1024 - analogRead(A0));
  EspSerial.println("temp="+String(1024 - analogRead(A0)));
}, &runner, true, NULL , NULL );
