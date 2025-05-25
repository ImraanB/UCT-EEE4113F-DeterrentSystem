long randNumber;

void setup() {
  // put your setup code here, to run once:
  
  pinMode(25, OUTPUT);
  pinMode(26, OUTPUT);
  pinMode(27, OUTPUT);
  randomSeed(analogRead(0));
  //randNumber = random(0,1);
  randNumber = 0;
}

void loop() {
  // put your main code here, to run repeatedly:


  //digitalWrite(25,HIGH), digitalWrite(26,HIGH), digitalWrite(27,HIGH) ;
  //%delay(2000);
  //%digitalWrite(25,LOW), digitalWrite(26,LOW), digitalWrite(27,LOW);
  //%delay(2000);

 


  digitalWrite(25,HIGH);
  digitalWrite(27,LOW);
  digitalWrite(26,LOW);
  delay(2000);
  digitalWrite(26,HIGH);
  digitalWrite(25,LOW);
  digitalWrite(27,LOW);
  delay(2000);
  digitalWrite(27,HIGH);
  digitalWrite(26,LOW);
  digitalWrite(25,LOW);
  delay(2000);



}
