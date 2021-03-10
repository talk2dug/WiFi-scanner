unsigned long lastMsg = 0;
const int Analog_channel_pin= 35;
long ADC_VALUE = 0;
float voltage_value = 0; 
const int Analog_channel_pin2= 34;
long ADC_VALUE2 = 0;
float voltage_value2 = 0; 


void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();  
}



void reconnect() {
  // Loop until we're reconnected
  while (!client2.connected()) {
    Serial.print("Attempting MQTT connection...");
    

    // Attempt to connect
    if (client2.connect(clientId)) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client2.publish("test", "hello world");
      // ... and resubscribe
      client2.subscribe("inTopic");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client2.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}



void uploadtoServer(){
  int index = 0;
  char stringArray[500];
  File file = SD.open(filename);
  String finalString = "";
  ADC_VALUE = analogRead(Analog_channel_pin);
ADC_VALUE2 = analogRead(Analog_channel_pin2);
voltage_value = (ADC_VALUE * 3.3 ) / (4095);
voltage_value2 = (ADC_VALUE2 * 3.3 ) / (4095);
  if (file) {
    while (file.available()) {
      DynamicJsonDocument obj(2048);
      unsigned long now = millis();
      if (now - lastMsg > 100) {
        lastMsg = now;
        obj["BSSIDstr"] = String(file.readStringUntil(','));
        obj["SSIDstr"] = file.readStringUntil(',');
        obj["latstr"] = file.readStringUntil(',');
        obj["lonstr"] = file.readStringUntil(',');
        obj["RSSIstr"] = file.readStringUntil(',');
        obj["Encryption"] = file.readStringUntil('\n');
        obj["ESPid"] = clientId;
        obj["batt"] = voltage_value * 2.16;
        obj["solar"] = voltage_value2 * 2.16;
        char buffer[1024];
        size_t n = serializeJson(obj, buffer);
        client2.publish("test", buffer, n);
        strip.setPixelColor(0, 0,   0,   0);         //  Set pixel's color (in RAM)
    strip.show();
    delay(50);
    strip.setPixelColor(0, 255,   0,   255);         //  Set pixel's color (in RAM)
    strip.show();
    delay(50);
    strip.setPixelColor(0, 0,   0,   0);         //  Set pixel's color (in RAM)
    strip.show();
    delay(50);
    strip.setPixelColor(0, 255,   0,   255);         //  Set pixel's color (in RAM)
    strip.show();
        obj.clear();
      }
    }
    if(SD.remove(filename)){
        Serial.println("File deleted and im going to sleep");
        Serial.flush(); 
        Serial2.flush(); 
        strip.setPixelColor(0, 0,   0,   0);         //  Set pixel's color (in RAM)
    strip.show();
    delay(50);
    strip.setPixelColor(0, 0,   0,   255);         //  Set pixel's color (in RAM)
    strip.show();
        esp_deep_sleep_start();
    } else {
        Serial.println("Delete failed");
    }
  }
}
