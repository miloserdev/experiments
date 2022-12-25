#include <math.h>
#include <TaskScheduler.h>

Scheduler runner;

float brightness = 0;    // уставливаем начально значение яркости
float fadeAmount = 0.1;    // шаг приращения/убывания яркости
int key;

bool isOn = false;

void setup() {
  pinMode(A0, INPUT);
  pinMode(PC13, OUTPUT);
}

//(brightness - 150) + i / 10)
void loop() {
  runner.execute();
}

Task Blink(0, TASK_FOREVER, []() {

  key = analogRead(A0);
  if ( key <= 0 ) { isOn = !isOn; }
  if( !isOn ) { analogWrite(PC13, 255); return; }

    for (int i = 1; i < 255; i++){
        analogWrite(PC13, (brightness + i) );
    }

    
  brightness = brightness + fadeAmount;
  if (brightness <= -60 || brightness >= 200) {
    fadeAmount = -fadeAmount ;
  }
}, &runner, true, NULL , NULL );
