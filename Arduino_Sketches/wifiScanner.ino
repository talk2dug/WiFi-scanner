#include <TinyGPS++.h>
#include "WiFi.h"
#include "FS.h"
#include "SD.h"
#include "SPI.h"
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <Adafruit_NeoPixel.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
 #include <WebServer.h>
 #include <SDConfigFile.h>
 
const char CONFIG_FILE[] = "/example.cfg";
const char* ssid = "";
const char* password = "";
const char* mqtt_server = "";
const char* host = "esp32";
String dataString = "";
const char *filename = "/wifiLogs.txt";
const char *configfilename = "/config.txt"; 
int atBase;
int gpsLock;
int APConnected = 0;

boolean didReadConfig;
char *clientId;
boolean doDelay;


boolean readConfiguration();

const char* loginIndex = 
 "<form name='loginForm'>"
    "<table width='20%' bgcolor='A09F9F' align='center'>"
        "<tr>"
            "<td colspan=2>"
                "<center><font size=4><b>ESP32 Login Page</b></font></center>"
                "<br>"
            "</td>"
            "<br>"
            "<br>"
        "</tr>"
        "<td>Username:</td>"
        "<td><input type='text' size=25 name='userid'><br></td>"
        "</tr>"
        "<br>"
        "<br>"
        "<tr>"
            "<td>Password:</td>"
            "<td><input type='Password' size=25 name='pwd'><br></td>"
            "<br>"
            "<br>"
        "</tr>"
        "<tr>"
            "<td><input type='submit' onclick='check(this.form)' value='Login'></td>"
        "</tr>"
    "</table>"
"</form>"
"<script>"
    "function check(form)"
    "{"
    "if(form.userid.value=='admin' && form.pwd.value=='admin')"
    "{"
    "window.open('/serverIndex')"
    "}"
    "else"
    "{"
    " alert('Error Password or Username')/*displays error message*/"
    "}"
    "}"
"</script>";
 
/*
 * Server Index Page
 */
 
const char* serverIndex = 
"<script src='https://ajax.googleapis.com/ajax/libs/jquery/3.2.1/jquery.min.js'></script>"
"<form method='POST' action='#' enctype='multipart/form-data' id='upload_form'>"
   "<input type='file' name='update'>"
        "<input type='submit' value='Update'>"
    "</form>"
 "<div id='prg'>progress: 0%</div>"
 "<script>"
  "$('form').submit(function(e){"
  "e.preventDefault();"
  "var form = $('#upload_form')[0];"
  "var data = new FormData(form);"
  " $.ajax({"
  "url: '/update',"
  "type: 'POST',"
  "data: data,"
  "contentType: false,"
  "processData:false,"
  "xhr: function() {"
  "var xhr = new window.XMLHttpRequest();"
  "xhr.upload.addEventListener('progress', function(evt) {"
  "if (evt.lengthComputable) {"
  "var per = evt.loaded / evt.total;"
  "$('#prg').html('progress: ' + Math.round(per*100) + '%');"
  "}"
  "}, false);"
  "return xhr;"
  "},"
  "success:function(d, s) {"
  "console.log('success!')" 
 "},"
 "error: function (a, b, c) {"
 "}"
 "});"
 "});"
 "</script>";
#define RXD2 16
#define TXD2 17
#define uS_TO_S_FACTOR 1000000ULL  /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP 7200        /* Time ESP32 will go to sleep (in seconds) */
#define BUTTON_PIN_BITMASK 0x200000000 // 2^33 in hex
#define LED_PIN    26
#define LED_COUNT 1

WiFiClient espClient;
PubSubClient client2(espClient);
TinyGPSPlus gps;
WiFiServer server2(8080);
WebServer server(80);
WiFiClient client = server2.available();
Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);


void setup(){
  Serial.begin(115200);
  didReadConfig = false;
  Serial.println("Calling SD.begin()...");
  if (!SD.begin()) {
    Serial.println("SD.begin() failed. Check: ");
    Serial.println("  card insertion,");
    Serial.println("  SD shield I/O pins and chip select,");
    Serial.println("  card formatting.");
    return;
  }
  Serial.println("...succeeded.");

  // Read our configuration from the SD card file.
  didReadConfig = readConfiguration();
  // Over the Air updating
  ArduinoOTA
    .onStart([]() {
      String type;
      if (ArduinoOTA.getCommand() == U_FLASH)
        type = "sketch";
      else // U_SPIFFS
        type = "filesystem";

      // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
      Serial.println("Start updating " + type);
    })
    .onEnd([]() {
      Serial.println("\nEnd");
    })
    .onProgress([](unsigned int progress, unsigned int total) {
      Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    })
    .onError([](ota_error_t error) {
      Serial.printf("Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
      else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
      else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
      else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
      else if (error == OTA_END_ERROR) Serial.println("End Failed");
    });

    // wakeup reason
  esp_sleep_wakeup_cause_t wakeup_reason;
  wakeup_reason = esp_sleep_get_wakeup_cause();
  
  //GPS serial connection
  Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2);
  // Set WiFi to station mode and disconnect from an AP if it was previously connected
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  if(!SD.begin()){
    Serial.println("Card Mount Failed");
    return;
  }
  delay(10);
  server.begin();
  //Setting up the sleep timer
  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_32,1); //1 = High, 0 = Low
  
  //LED light
  strip.begin();           // INITIALIZE NeoPixel strip object (REQUIRED)
  strip.show();            // Turn OFF all pixels ASAP
  strip.setBrightness(50); // Set BRIGHTNESS to about 1/5 (max = 255)
  Serial.println("Setup done");
    if (!MDNS.begin(host)) { 
    Serial.println("Error setting up MDNS responder!");
    while (1) {
      delay(1000);
    }
  }
  Serial.println("mDNS responder started");
  
  /*return index page which is stored in serverIndex */
  server.on("/", HTTP_GET, []() {
    server.sendHeader("Connection", "close");
    server.send(200, "text/html", loginIndex);
  });
  server.on("/serverIndex", HTTP_GET, []() {
    server.sendHeader("Connection", "close");
    server.send(200, "text/html", serverIndex);
  });
  /*handling uploading firmware file */
  server.on("/update", HTTP_POST, []() {
    server.sendHeader("Connection", "close");
    server.send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
    ESP.restart();
  }, []() {
    HTTPUpload& upload = server.upload();
    if (upload.status == UPLOAD_FILE_START) {
      Serial.printf("Update: %s\n", upload.filename.c_str());
      if (!Update.begin(UPDATE_SIZE_UNKNOWN)) { //start with max available size
        Update.printError(Serial);
      }
    } else if (upload.status == UPLOAD_FILE_WRITE) {
      /* flashing firmware to ESP*/
      if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
        Update.printError(Serial);
      }
    } else if (upload.status == UPLOAD_FILE_END) {
      if (Update.end(true)) { //true to set the size to the current progress
        Serial.printf("Update Success: %u\nRebooting...\n", upload.totalSize);
      } else {
        Update.printError(Serial);
      }
    }
  });
  server.begin();
 ArduinoOTA.begin();
// If the button was pushed
  if(wakeup_reason==ESP_SLEEP_WAKEUP_EXT0){
    gointoConfig();
  }
  
}

unsigned long previousMillis = 0;
const long interval = 60000;

void loop(){
  unsigned long currentMillis = millis();
  //Handles Over the air update
  ArduinoOTA.handle();
  
  //Handles webserver requests
  server.handleClient();

  //Why did we wake up
  esp_sleep_wakeup_cause_t wakeup_reason;
  wakeup_reason = esp_sleep_get_wakeup_cause();
  if(wakeup_reason==ESP_SLEEP_WAKEUP_EXT0){
    if (currentMillis - previousMillis >= interval) {
    // save the last time you blinked the LED
    previousMillis = currentMillis;
    Serial.println("Ive been up too long, rebooting");
     ESP.restart();
   
  }
    
    
    
    }
  if(wakeup_reason!=ESP_SLEEP_WAKEUP_EXT0){
    
 //Listens for GPS packet
  while (Serial2.available())
  gps.encode(Serial2.read());
  //AT HOUSE
  static const double Home_LAT = 38.926397, Home_LON = -77.6963497;
  //AWAY FROM HOUSE
    //static const double Home_LAT = 38.626221, Home_LON = -77.702514;

    //Math to determine if were close to home base
  double distanceToHome = TinyGPSPlus::distanceBetween(gps.location.lat(),gps.location.lng(),Home_LAT, Home_LON);
  double distenceFromBase = distanceToHome/1000 * 0.6213;
  if(distenceFromBase<.5){
    atBase = 1;
  }
  else{
    atBase = 0;
  }  
  if(atBase == 0){
   scanNetworks();
  }    
  if(atBase == 1 && APConnected == 0){
    setup_wifi();
    APConnected = 1;
   }
 
  if(atBase == 1 && APConnected == 1){
    strip.setPixelColor(0, 0,   255,   255);         //  Set pixel's color (in RAM)
    strip.show();
    client2.setServer(mqtt_server, 1883);
    client2.setCallback(callback);
    if (!client2.connected()) {
      reconnect();
    }
    client2.loop();
     if (client) {
      sendHTMLtoClient();
      }
     uploadtoServer();
  }
  
  }
}

void gointoConfig(){
  setup_wifi();
    strip.setPixelColor(0, 255,   0,   0);         //  Set pixel's color (in RAM)
    strip.show();
    
  
  
  }




boolean readConfiguration() {
  /*
   * Length of the longest line expected in the config file.
   * The larger this number, the more memory is used
   * to read the file.
   * You probably won't need to change this number.
   */
  const uint8_t CONFIG_LINE_LENGTH = 127;
  
  // The open configuration file.
  SDConfigFile cfg;
  
  // Open the configuration file.
  if (!cfg.begin(CONFIG_FILE, CONFIG_LINE_LENGTH)) {
    Serial.print("Failed to open configuration file: ");
    Serial.println(CONFIG_FILE);
    return false;
  }
  
  // Read each setting from the file.
  while (cfg.readNextSetting()) {
    
    // Put a nameIs() block here for each setting you have.
    //clientId ="asdfhtr"
//Home_LAT = 38.926397
//Home_LON = -77.6963497
//ssid = "Sorrento2"
//password = "0000011111"
//mqtt_server = "192.168.86.57"
    // doDelay
    if (cfg.nameIs("clientId")) {
      
      clientId = cfg.copyValue();
      
     Serial.println(clientId);
  
    } else if (cfg.nameIs("ssid")) {
      
      ssid = cfg.copyValue();
   

    } else if (cfg.nameIs("password")) {
      password = cfg.copyValue();
      

    }else if (cfg.nameIs("Home_LAT")) {
      

    } else if (cfg.nameIs("Home_LON")) {
     

    }else if (cfg.nameIs("mqtt_server")) {
      mqtt_server = cfg.copyValue();

    } else{
      // report unrecognized names.
      Serial.print("Unknown name in config: ");
      Serial.println(cfg.getName());
    }
  }
  
  // clean up
  cfg.end();
  
  return true;
}  
