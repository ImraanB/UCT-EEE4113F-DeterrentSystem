#include <ESP32Servo.h>

Servo myservo;


void setup() {
  // put your setup code here, to run once:
  myservo.attach(33);

}

void loop() {
  // put your main code here, to run repeatedly:
  myservo.write(120);
  delay(1000);
  myservo.write(0);
  delay(1500);
  //myservo.write(20);
  //delay(100000);

}
