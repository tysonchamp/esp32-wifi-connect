#include <WiFi.h>
#include <WebServer.h>
#include <time.h>
#include <AutoConnect.h>
#include <HTTPClient.h>
#include <Arduino_JSON.h>
#include <ESP32Ping.h>

#define DEBUG_SW 1

#define S1 13
#define D1 27

#define S2 26
#define D2 25

#define BUILTIN_LED  2  // backward compatibility

int switch_ON_Flag1_previous_I = 0;
int switch_ON_Flag2_previous_I = 0;

int MODE = 0;
// const char* ping_ip = "8.8.8.8";
IPAddress ping_ip (8, 8, 8, 8); // The remote ip to ping

const String endpoint = "http://homeautoci.erisetechnology.com/api/devices/devicelists/";
const String key = "jsdukjfnxvzxcxjhzxjgha";

AutoConnect portal;
AutoConnectConfig Config;       // Enable autoReconnect supported on v0.9.4

void handleRoot() {
  String page = PSTR(
"<html>"
"<head>"
  "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">"
  "<style type=\"text/css\">"
    "body {"
    "-webkit-appearance:none;"
    "-moz-appearance:none;"
    "font-family:'Arial',sans-serif;"
    "text-align:center;"
    "}"
    ".menu > a:link {"
    "position: absolute;"
    "display: inline-block;"
    "right: 12px;"
    "padding: 0 6px;"
    "text-decoration: none;"
    "}"
    ".button {"
    "display:inline-block;"
    "border-radius:7px;"
    "background:#73ad21;"
    "margin:0 10px 0 10px;"
    "padding:10px 20px 10px 20px;"
    "text-decoration:none;"
    "color:#000000;"
    "}"
  "</style>"
"</head>"
"<body>"
  "<div class=\"menu\">" AUTOCONNECT_LINK(BAR_32) "</div>"
  "BUILT-IN LED<br>"
  "GPIO(");
  page += String(BUILTIN_LED);
  page += String(F(") : <span style=\"font-weight:bold;color:"));
  page += digitalRead(BUILTIN_LED) ? String("Tomato\">HIGH") : String("SlateBlue\">LOW");
  page += String(F("</span>"));
  page += String(F("<p><a class=\"button\" href=\"/io?v=low\">low</a><a class=\"button\" href=\"/io?v=high\">high</a></p>"));
  page += String(F("</body></html>"));
  portal.host().send(200, "text/html", page);
}

void sendRedirect(String uri) {
  WebServerClass& server = portal.host();
  server.sendHeader("Location", uri, true);
  server.send(302, "text/plain", "");
  server.client().stop();
}

void handleGPIO() {
  WebServerClass& server = portal.host();
  if (server.arg("v") == "low")
    digitalWrite(BUILTIN_LED, LOW);
  else if (server.arg("v") == "high")
    digitalWrite(BUILTIN_LED, HIGH);
  sendRedirect("/");
}

bool atDetect(IPAddress& softapIP) {
  Serial.println("Captive portal started, SoftAP IP:" + softapIP.toString());
  return true;
}

void update_to_server(String VirtualDeviceID, String VirtualIDStatus){
  String serverAddress = "http://homeautoci.erisetechnology.com/api/device/toogle_device_from_esp/" + VirtualDeviceID + "/" + VirtualIDStatus;

  HTTPClient http;
  http.begin(serverAddress); //Specify the URL
  int httpCode = http.GET();  //Make the request
  Serial.println(serverAddress);
}

void without_internet(){
  Serial.println(digitalRead(S1));
  digitalWrite(D1, !digitalRead(S1));
  Serial.println(digitalRead(S2));
  digitalWrite(D2, !digitalRead(S2));
}

void with_internet(){
  
  if(digitalRead(S1) == HIGH){
    switch_ON_Flag1_previous_I = 1;
  }
  
  if(digitalRead(S2) == HIGH){
    switch_ON_Flag2_previous_I = 1;
  }
  
  HTTPClient http;
  http.begin(endpoint + key); //Specify the URL
  int httpCode = http.GET();  //Make the request
  
  String payload = http.getString();  
  JSONVar myArray = JSON.parse(payload);
  
  for (int i = 0; i < myArray["devices"].length(); i++) {
    
    String deviceID = JSON.stringify(myArray["devices"][i]["nickname"]);
    Serial.println("Device ID: " + deviceID);
    int deviceID_to_int = ( deviceID == "\"25\"" ? 25 : 27 );
    
    String deviceVirtualID = JSON.stringify(myArray["devices"][i]["id"]);
    
    String deviceStatus = JSON.stringify(myArray["devices"][i]["status"]);
    Serial.println("Device Status: " + deviceStatus);
    int deviceStatus_to_int = ( deviceStatus == "\"0\"" ? 0 : 1 );
    
    digitalWrite(deviceID_to_int, deviceStatus_to_int);

    // Switch 1 Start
    if(deviceID_to_int == D1){
      
      Serial.print("S1 Status: ");
      Serial.println(digitalRead(S1));
      Serial.print("S1 Previous Status: ");
      Serial.println(switch_ON_Flag1_previous_I);
      
      if(digitalRead(S1) == HIGH && switch_ON_Flag1_previous_I == 0){

        if(deviceStatus_to_int == 1){
          switch_ON_Flag1_previous_I = 1;
          digitalWrite(deviceID_to_int, LOW);
          update_to_server(deviceVirtualID, "0");
        }else{
          switch_ON_Flag1_previous_I = 1;
          digitalWrite(deviceID_to_int, HIGH);
          update_to_server(deviceVirtualID, "1");
        }
        
      }else if(digitalRead(S1) == LOW && switch_ON_Flag1_previous_I == 1){
        
        if(deviceStatus_to_int == 1){
          switch_ON_Flag1_previous_I = 0;
          digitalWrite(deviceID_to_int, LOW);
          update_to_server(deviceVirtualID, "0");
        }else{
          switch_ON_Flag1_previous_I = 0;
          digitalWrite(deviceID_to_int, HIGH);
          update_to_server(deviceVirtualID, "1");
        }
        
      }
    }
    // Switch 1 End
    
    // Switch 2 Start
    if(deviceID_to_int == D2){
      
      Serial.print("S2 Status: ");
      Serial.println(digitalRead(S2));
      Serial.print("S2 Previous Status: ");
      Serial.println(switch_ON_Flag2_previous_I);
      
      if(digitalRead(S2) == HIGH && switch_ON_Flag2_previous_I == 0){
        
        if(deviceStatus_to_int == 1){
          switch_ON_Flag2_previous_I = 1;
          digitalWrite(deviceID_to_int, LOW);
          update_to_server(deviceVirtualID, "0");
        }else{
          switch_ON_Flag2_previous_I = 1;
          digitalWrite(deviceID_to_int, HIGH);
          update_to_server(deviceVirtualID, "1");
        }
        
      }else if(digitalRead(S2) == LOW && switch_ON_Flag2_previous_I == 1){
        
        if(deviceStatus_to_int == 1){
          switch_ON_Flag2_previous_I = 0;
          digitalWrite(deviceID_to_int, LOW);
          update_to_server(deviceVirtualID, "0");
        }else{
          switch_ON_Flag2_previous_I = 0;
          digitalWrite(deviceID_to_int, HIGH);
          update_to_server(deviceVirtualID, "1");
        }
        
      }
    }
    // Switch 2 End
    
  }
}

void setup() {
  delay(1000);
  // Debug console
  if (DEBUG_SW) Serial.begin(115200);
  pinMode(S1, INPUT_PULLUP);
  pinMode(D1, OUTPUT);

  pinMode(S2, INPUT_PULLUP);
  pinMode(D2, OUTPUT);
  
  pinMode(BUILTIN_LED, OUTPUT);

  // Put the home location of the web site.
  // But in usually, setting the home uri is not needed cause default location is "/".
  //portal.home("/");
  
  Config.ota = AC_OTA_BUILTIN;
  Config.hostName = "yotouch-01";
  portal.config(Config);

  // Starts user web site included the AutoConnect portal.
  portal.onDetect(atDetect);
  if (portal.begin()) {
    WebServerClass& server = portal.host();
    server.on("/", handleRoot);
    server.on("/io", handleGPIO);
    Serial.println("Started, IP:" + WiFi.localIP().toString());
  }else {
    Serial.println("Connection failed.");
    while (true) { yield(); }
  }
  
  digitalWrite(D1, LOW);
  update_to_server("\"23\"", "0");
  digitalWrite(D2, LOW);
  update_to_server("\"22\"", "0");

}

void loop() {
  portal.handleClient();
  // put your main code here, to run repeatedly:
  if (WiFi.status() != WL_CONNECTED){
    //if (DEBUG_SW) Serial.println("Not Connected");
    // WiFi.begin(ssid, password);
    // delay(4000);
    MODE = 0;
  }else{
    //if (DEBUG_SW) Serial.println("Connected");
    if(Ping.ping(ping_ip, 1)) {
      //Serial.println("Ping Success!!");
      MODE = 1;
    } else {
      //Serial.println("Ping Error :(");
      MODE = 0;
    }
  }

  if (MODE == 1)
    with_internet();
  else
    without_internet();
}
