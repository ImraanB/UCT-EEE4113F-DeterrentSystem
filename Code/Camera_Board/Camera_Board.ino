// Libraries
#include <esp_now.h>
#include <WiFi.h>
#include "FS.h"
#include "SD_MMC.h"
#include "esp_camera.h"
#include "time.h"


//----------------------------------------------------------------
// SD card transfer mode
#define ONE_BIT_MODE false

// Camera module
#define CAMERA_MODEL_AI_THINKER // Has PSRAM
#include "camera_pins.h"

// Replace with your network credentials (STATION)
const char* ssid = "TP-Link-C80";
const char* password = "0216964450R";

// NTP server
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 2*3600;
const int   daylightOffset_sec = 0;

// Structure to receive data (**Must match the sender structure)
typedef struct struct_message {
  String state;
} struct_message;

struct_message incomingEvent;

// Local data structure 
typedef struct struct_event {
  String date;
  String time;
  String state;
} struct_event;

struct_event currentEvent;

//WebServer--------------------------------------------------------------


void setup() {
  // Initialize Serial Monitor
  Serial.begin(115200);

  // Wifi-------------------------------------------------------------
  // Set the device as a Station and Soft Access Point simultaneously
  WiFi.mode(WIFI_AP_STA);
  
  // Set device as a Wi-Fi Station
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Setting as a Wi-Fi Station..");
  }
  Serial.print("Station IP Address: ");
  Serial.println(WiFi.localIP());
  Serial.print("Wi-Fi Channel: ");
  Serial.println(WiFi.channel());

  // Configure NTP
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

  // Init ESP-NOW-----------------------------------------------------
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }  
  // Once ESPNow is successfully Init, we will register for recv CB to
  // get recv packer info
  esp_now_register_recv_cb(esp_now_recv_cb_t(OnDataRecv));

  // Camera config----------------------------------------------------
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;

  if (psramFound()) {
    config.frame_size = FRAMESIZE_UXGA;
    config.jpeg_quality = 10;
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }

  #if defined(CAMERA_MODEL_ESP_EYE)
  pinMode(13, INPUT_PULLUP);
  pinMode(14, INPUT_PULLUP);
  #endif

  // camera init
  esp_err_t err = esp_camera_init( & config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }

  sensor_t * s = esp_camera_sensor_get();
  // initial sensors are flipped vertically and colors are a bit saturated
  if (s -> id.PID == OV3660_PID) {
    s -> set_vflip(s, 1); // flip it back
    s -> set_brightness(s, 1); // up the brightness just a bit
    s -> set_saturation(s, -2); // lower the saturation
  }
  // drop down frame size for higher initial frame rate
  s -> set_framesize(s, FRAMESIZE_HD);

  #if defined(CAMERA_MODEL_M5STACK_WIDE) || defined(CAMERA_MODEL_M5STACK_ESP32CAM)
  s -> set_vflip(s, 1);
  s -> set_hmirror(s, 1);
  #endif

  // SD card init-------------------------------------------------------
  if (!SD_MMC.begin("/sdcard", ONE_BIT_MODE)) {
    Serial.println("Card Mount Failed");
    return;
  }
  uint8_t cardType = SD_MMC.cardType();

  if (cardType == CARD_NONE) {
    Serial.println("No SD_MMC card attached");
    return;
  }

  // If the data.csv file doesn't exist
  // Create a file on the SD card and write the data labels
  File file = SD_MMC.open("/data.csv");
  if(!file) {
    Serial.println("File doens't exist");
    Serial.println("Creating file...");
    writeFile(SD_MMC, "/data.csv", "Date, Time, State \r\n");
  }
  else {
    Serial.println("File already exists");  
  }
  file.close();
  //WebServer-----------------------------------------------------------------

}
 
void loop() {

}


// Functions-------------------------------------------------------------------------------

// Callback function that will be executed when data is received
void OnDataRecv(const uint8_t * mac_addr, const uint8_t *incomingData, int len) { 
  // Copies the sender mac address to a string
  char macStr[18];
  Serial.print("Packet received from: ");
  snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
           mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
  Serial.println(macStr);
  memcpy(&incomingEvent, incomingData, sizeof(incomingEvent));
  currentEvent.date=storeDate();
  currentEvent.time=storeTime();
  currentEvent.state=incomingEvent.state;
  logData();
  if (currentEvent.state=="highAlert"){
    takePicture();
  }
}

void takePicture(){
  camera_fb_t * fb = NULL;
  fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("Camera capture failed");
    return;
  }

  String photoFileName = "/"+currentEvent.date + "_"+ currentEvent.time + ".jpg";
  fs::FS & fs = SD_MMC;
  Serial.printf("Picture file name: %s\n", photoFileName.c_str());

  File file = fs.open(photoFileName.c_str(), FILE_WRITE);
  if (!file) {
    Serial.println("Failed to open file in writing mode");
  } else {
    file.write(fb -> buf, fb -> len);
    Serial.printf("Saved file to path: %s\n", photoFileName.c_str());
  }
  file.close();
  esp_camera_fb_return(fb);
}

void readFile(fs::FS &fs, const char *path) {   //eg. readFile(SD_MMC, "/data.csv")
  Serial.printf("Reading file: %s\n", path);

  File file = fs.open(path);
  if (!file) {
    Serial.println("Failed to open file for reading");
    return;
  }

  Serial.print("Read from file: ");
  while (file.available()) {
    Serial.write(file.read());
  }
}

void writeFile(fs::FS &fs, const char *path, const char *message) {
  Serial.printf("Writing file: %s\n", path);

  File file = fs.open(path, FILE_WRITE);
  if (!file) {
    Serial.println("Failed to open file for writing");
    return;
  }
  if (file.print(message)) {
    Serial.println("File written");
  } else {
    Serial.println("Write failed");
  }
}

void appendFile(fs::FS &fs, const char *path, const char *message) {
  Serial.printf("Appending to file: %s\n", path);

  File file = fs.open(path, FILE_APPEND);
  if (!file) {
    Serial.println("Failed to open file for appending");
    return;
  }
  if (file.print(message)) {
    Serial.println("Message appended");
  } else {
    Serial.println("Append failed");
  }
  file.close();
}

String storeDate(){   //returns date as String in format of: 20250425
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
    return "0";
  }
  String month="0";
  String day="0";
  if (timeinfo.tm_mon+1<10){
    month="0"+String(timeinfo.tm_mon+1);
  }else{
    month=String(timeinfo.tm_mon+1);
  }
  if (timeinfo.tm_mday<10){
    day="0"+String(timeinfo.tm_mday);
  }else{
    day=String(timeinfo.tm_mday);
  }

  String date=String(timeinfo.tm_year+1900)+month+day;
  return date;
}

String storeTime(){  //returns time as String in format of: 132659
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
    return "0";
  }
  String hour="0";
  String min="0";
  String sec="0";

  if (timeinfo.tm_hour<10){
    hour="0"+String(timeinfo.tm_hour);
  }else{
    hour=String(timeinfo.tm_hour);
  }

  if (timeinfo.tm_min<10){
    min="0"+String(timeinfo.tm_min);
  }else{
    min=String(timeinfo.tm_min);
  }

  if (timeinfo.tm_sec<10){
    sec="0"+String(timeinfo.tm_sec);
  }else{
    sec=String(timeinfo.tm_sec);
  }

  String time=hour+min+sec;
  return time;
}

void logData(){   // saves data to SD card
  String data = currentEvent.date + "," + currentEvent.time + "," +currentEvent.state + " \r\n";
  appendFile(SD_MMC, "/data.csv", data.c_str());
}

