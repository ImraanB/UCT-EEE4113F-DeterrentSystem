void setup() {
  // put your setup code here, to run once:
pinMode(25, OUTPUT);
 pinMode(26, OUTPUT);
  pinMode(27, OUTPUT);
pinMode(32, OUTPUT);
pinMode(12,OUTPUT);
}

void loop() {
  // put your main code here, to run repeatedly:
 digitalWrite(25,HIGH), digitalWrite(26,LOW),digitalWrite(27,LOW), digitalWrite(32,HIGH),digitalWrite(12,HIGH);
 /* delay(3000);
  digitalWrite(32,LOW),digitalWrite(12,LOW);
  delay(3000);*/

}
