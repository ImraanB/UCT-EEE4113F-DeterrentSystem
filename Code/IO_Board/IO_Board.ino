// Libraries
#include <esp_now.h>
#include <esp_wifi.h>
#include <WiFi.h>
#include <LD2450.h>
#include <ESP32Servo.h> 

// Pin assignments
int PIR_Pin = 4;
#define radarRX 16
#define radarTX 17 
int stateReset=15;  //connect to camera board pin 16

// Variables
String state = "idle";
unsigned long currentMillis =0;
unsigned long previousMillis = 0;    
const long interval = 5000;        
int distToFence =100; //mm
Servo myservo; //variable to control servo
long randNumber; //random number to alternate between deterrent methods

// SENSOR INSTANCE
LD2450 ld2450;



// Event Structure 
//Must match the receiver structure
typedef struct struct_message {
  String state;
} struct_message;

//Create a struct_message called eventInfo
struct_message eventInfo;

// ESP-NOW--------------------------------------------
// Set your Board ID (ESP32 Sender #1 = BOARD_ID 1, ESP32 Sender #2 = BOARD_ID 2, etc)
#define BOARD_ID 1

//MAC Address of the receiver 
uint8_t broadcastAddress[] = {0xEC, 0xE3, 0x34, 0xD7, 0x77, 0x64};//ec:e3:34:d7:77:64

// Insert your SSID
constexpr char WIFI_SSID[] = "ESP_D77765";

esp_now_peer_info_t peerInfo;
//----------------------------------------------------




void setup() {
  //Init Serial Monitor
  Serial.begin(115200);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB
  }

  // Set pins
  pinMode(PIR_Pin, INPUT);     // declare sensor as input
  pinMode(stateReset, INPUT);

  myservo.attach(33); //ensure that pwm for servo motor is attached to pin 33
  pinMode(32, OUTPUT); //audio 1 is connected to pin 32
  pinMode(12,OUTPUT); //audio 2 is connected to pin 12
  pinMode(25, OUTPUT);//light 1 connected to pin 25
  pinMode(26, OUTPUT);//light 2 connected to pin 26
  pinMode(27, OUTPUT);//light 3 connected to pin 27


  //LD2450--------------------------------------------------------------------
  Serial1.begin(256000, SERIAL_8N1, radarRX, radarTX);  //Init radar comms
  Serial.println("LD2450_SETUP_STARTED");
  ld2450.begin(Serial1, false);
  if(!ld2450.waitForSensorMessage()){
    Serial.println("SENSOR CONNECTION SEEMS OK");
  }else{
    Serial.println("SENSOR TEST: GOT NO VALID SENSORDATA - PLEASE CHECK CONNECTION!");
  }
  Serial.println("LD2450_SETUP_FINISHED");

  //ESP_NOW-------------------------------------------------------------- 
  // Set device as a Wi-Fi Station and set channel
  WiFi.mode(WIFI_STA);

  int32_t channel = getWiFiChannel(WIFI_SSID);

  //WiFi.printDiag(Serial); // Uncomment to verify channel number before
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
  esp_wifi_set_promiscuous(false);
  //WiFi.printDiag(Serial); // Uncomment to verify channel change after

  // Init ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // Once ESPNow is successfully Init, we will register for Send CB to
  // get the status of Trasnmitted packet
  esp_now_register_send_cb(OnDataSent);
  
  // Register peer
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.encrypt = false;
  
  // Add peer        
  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    Serial.println("Failed to add peer");
    return;
  }
  //setup-----------------------------------------------------------
    
}
 
void loop() {
  const int sensor_got_valid_targets = ld2450.read();
  const LD2450::RadarTarget result_target = ld2450.getTarget(0);
  if (state=="idle"){
    if (PIR()){
      state="warning";
      previousMillis=millis();
    }
    if(Target()){
      state="highAlert";
    }
  }else if (state=="warning"){
    // if no further movement, go back to idle after 10 seconds
    currentMillis = millis();
    if (currentMillis - previousMillis >= interval) {
      state="idle";
      return;
    }
    if (PIR()){
      previousMillis=millis();
    }
    if(Target()){
      state="highAlert";
    }

    // deterrents for warning
    Serial.println("execute");
    light_all();

    


  }
  
  else if (state=="highAlert"){
    if (noTarget()){
      state="idle";
    }
    if (gotThrough()){
      state="emergency";
    }
    // deterrents for high alert
    light_all(), audio(), motor();
    
  }
  else if (state=="emergency"){
    if(digitalRead(stateReset)){
      state="idle";
    }
    //during emergency

  }

  Serial.println(result_target.x);  // debugging
  // Send event upon state change
  if (state!=eventInfo.state){
    eventInfo.state=state;
    Serial.println(state);
    ESPNOWsend();
  }
  }


// Function definitions---------------------------------------------
bool PIR(){
  return digitalRead(PIR_Pin);
}


int32_t getWiFiChannel(const char *ssid) {
  if (int32_t n = WiFi.scanNetworks()) {
      for (uint8_t i=0; i<n; i++) {
          if (!strcmp(ssid, WiFi.SSID(i).c_str())) {
              return WiFi.channel(i);
          }
      }
  }
  return 0;
}

void ESPNOWsend(){
  esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &eventInfo, sizeof(eventInfo));
  if (result == ESP_OK) {
    Serial.println("Sent with success");
  }
  else {
    Serial.println("Error sending the data");
  }
}
// callback when data is sent
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("\r\nLast Packet Send Status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}

// If target is within 100mm of fence
bool Target(){
  const int sensor_got_valid_targets = ld2450.read();
  const LD2450::RadarTarget result_target = ld2450.getTarget(0);
  if (result_target.valid&&(result_target.x <= distToFence && result_target.x>0)){
    int i = 0;
    for (i; i < 4; i++) {
      LD2450::RadarTarget result_target = ld2450.getTarget(0);
      if (result_target.x>distToFence){
        break;
      }
      delay(250);
    }
    if(i==4){
      return HIGH;
    }
    else{
      return LOW;
    }
  }else{
    return LOW;
  }
}

// If target leaves fence 
bool noTarget(){
  const int sensor_got_valid_targets = ld2450.read();
  const LD2450::RadarTarget result_target = ld2450.getTarget(0);
  if (result_target.valid&&result_target.x > distToFence+100){
    int i = 0;
    for (i; i < 4; i++) {
      LD2450::RadarTarget result_target = ld2450.getTarget(0);
      if (result_target.x<=distToFence){
        break;
      }
      delay(250);
    }
    if(i==4){
      return HIGH;
    }
    else{
      return LOW;
    }
  }else{
    return LOW;
  }
}

// If target got trough the fence
bool gotThrough(){
  const int sensor_got_valid_targets = ld2450.read();
  const LD2450::RadarTarget result_target = ld2450.getTarget(0);
  if (result_target.valid&&result_target.x < -50){
    int i = 0;
    for (i; i < 5; i++) {
      LD2450::RadarTarget result_target = ld2450.getTarget(0);
      if (result_target.x>-50){
        break;
      }
      delay(250);
    }
    if(i==5){
      return HIGH;
    }
    else{
      return LOW;
    }
  }else{
    return LOW;
  }
}

void motor(){ //function to activate the motor
  myservo.write(120);
  delay(1000);
  myservo.write(0);
  delay(1500);
}

void audio(){ //function to activate the auditory deterrents
  digitalWrite(32,HIGH),digitalWrite(12,HIGH);
  delay(3000);
  digitalWrite(32,LOW),digitalWrite(12,LOW);
  delay(3000);
}

void light_all(){ //function to ensure that all the lights flash
   digitalWrite(25,HIGH), digitalWrite(26,HIGH), digitalWrite(27,HIGH) ;
  delay(600);
  digitalWrite(25,LOW), digitalWrite(26,LOW), digitalWrite(27,LOW);
  delay(600);

}

void light_alternate(){ //function to ensure that lights alternate flashing
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







