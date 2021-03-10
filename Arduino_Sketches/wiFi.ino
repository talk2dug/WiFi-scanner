void setup_wifi(){

  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  
}

void sendHTMLtoClient(){
  int n = WiFi.scanNetworks();
  static char lattoutstr[15];
  static char lonoutstr[15];
  dtostrf(gps.location.lat(),7, 6, lattoutstr);
  dtostrf(gps.location.lng(),7, 6, lonoutstr);
  for (int i = 0; i < n; ++i) {
    dataString += String(WiFi.BSSIDstr(i));
    dataString += ",";
    dataString +=WiFi.SSID(i);
    dataString += ",";
    dataString +=lattoutstr;
    dataString += ",";
    dataString += lonoutstr;
    dataString += ",";
    dataString += WiFi.RSSI(i);
    dataString += ",";
    dataString += WiFi.encryptionType(i);
    dataString += "<br />";
  }
  boolean currentLineIsBlank = true;
  while (client.connected()) {
    if (client.available()) {
      client.println("HTTP/1.1 200 OK");
      client.println("Content-Type: text/html");
      client.println("Connection: close");
      client.println("Refresh: 5");
      client.println();
      client.println("<!DOCTYPE HTML>");
      client.println("<html>");
      client.print("<h1>Wifi Networks</h1>");
      client.print(dataString);
      client.println("<br />");
      client.println("</html>");
    }
    delay(500);
    // close the connection:
    client.stop();
    dataString = "";
  }
}

void scanNetworks(){
  Serial.println("startRecording");
  WiFi.disconnect();
  APConnected = 0;
  // WiFi.scanNetworks will return the number of networks found
  int n = WiFi.scanNetworks();
  if (n == 0) {
  } 
  else {
    String dataString = "";
    String encryptiontype = "";
    static char lattoutstr[15];
    static char lonoutstr[15];
    dtostrf(gps.location.lat(),7, 6, lattoutstr);
    dtostrf(gps.location.lng(),7, 6, lonoutstr);
    for (int i = 0; i < n; ++i) {
     dataString += String(WiFi.BSSIDstr(i));
      dataString += ",";
      dataString +=WiFi.SSID(i);
      dataString += ",";
      dataString +=lattoutstr;
      dataString += ",";
      dataString += lonoutstr;
      dataString += ",";
      dataString += WiFi.RSSI(i);
      dataString += ",";
      dataString += WiFi.encryptionType(i);
      Serial.println(dataString);
      strip.setPixelColor(0, 0,   0,   0);         //  Set pixel's color (in RAM)
    strip.show();
    delay(50);
    strip.setPixelColor(0, 255,   255,   255);         //  Set pixel's color (in RAM)
    strip.show();
      File file = SD.open(filename, FILE_APPEND);
      if (file) {
        file.println(dataString);
        dataString = "";
        file.close();
      }
      else {
        Serial.println("error opening datalog.txt");
      }
      delay(10);
    }
  }
  Serial.println("");
  // Wait a bit before scanning again
  delay(250);
}
