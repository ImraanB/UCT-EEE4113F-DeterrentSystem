long randNumber;
int leds[] = {2,4,7};

void setup() {
  // put your setup code here, to run once:
  
  pinMode(2, OUTPUT);
  pinMode(4, OUTPUT);
  pinMode(7, OUTPUT);
  randomSeed(analogRead(0));
  //randNumber = random(0,1);
  randNumber = 0;
}

void loop() {
  // put your main code here, to run repeatedly:


  digitalWrite(2,HIGH), digitalWrite(4,HIGH), digitalWrite(7,HIGH) ;
  delay(2000);
  digitalWrite(2,LOW), digitalWrite(4,LOW), digitalWrite(7,LOW);
  delay(2000);

 

/*
  digitalWrite(2,HIGH);
  digitalWrite(7,LOW);
  digitalWrite(4,LOW);
  delay(2000);
  digitalWrite(4,HIGH);
  digitalWrite(2,LOW);
  digitalWrite(7,LOW);
  delay(2000);
  digitalWrite(7,HIGH);
  digitalWrite(4,LOW);
  digitalWrite(2,LOW);
  delay(2000);
*/


}
