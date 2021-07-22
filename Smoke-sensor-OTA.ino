/**
 * ESP8266 12F + CS2105GO-S10 Smoke Sersor with Vera integration. 
 * To be powered by MP2307-based step down converter.
 *
 *Create vera device :
 *curl "http://VERAIP:3480/data_request?id=action&serviceId=urn:micasaverde-com:serviceId:HomeAutomationGateway1&action=CreateDevice&deviceType=urn:schemas-micasaverde-com:device:SmokeSensor:1&internalID=&Description=WiFiSmokeSensor&UpnpDevFilename=D_SmokeSensor1.xml&UpnpImplFilename=&MacAddress=&RoomNum=0&Reload=1&IpAddress="
 *curl "http://VERAIP:3480/data_request?id=variableset&serviceId=urn:micasaverde-com:serviceId:SecuritySensor1&Variable=AutoUntrip&Value=300&DeviceNum=DEVICEID
 **/

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>

//Atomic OTA Updates
#define ATOMIC_FS_UPDATE

//Pull PIN 4 LOW for OTA
#define OTA_TRIGGER_PIN  4

#include "config.h"

String ProgramVersion  = "0.1";

WiFiClient client;
HTTPClient http;

long glb_rssi=0;

void gotoDeepSleep(int SleepDelay){
  Serial.print("Sleeping " + String(SleepDelay) + " seconds\n");
  ESP.deepSleepInstant(SleepDelay * 1000000,WAKE_RF_DEFAULT);
}

void update_started() {
  Serial.println("HTTP update process started");
}

void update_finished() {
  Serial.println("HTTP update process finished");
}

void update_progress(int cur, int total) {
  Serial.printf("HTTP update process at %d of %d bytes...\n", cur, total);
}

void update_error(int err) {
  Serial.printf("HTTP update fatal error code %d\n", err);
}

int checkForUpdates() {
    ESPhttpUpdate.setLedPin(LED_BUILTIN, LOW);

    pinMode(OTA_TRIGGER_PIN, INPUT_PULLUP);
    delay(100);
    
    if ( digitalRead(OTA_TRIGGER_PIN) != LOW ) {
      return 1;
    }

    // Add optional callback notifiers
    ESPhttpUpdate.onStart(update_started);
    ESPhttpUpdate.onEnd(update_finished);
    ESPhttpUpdate.onProgress(update_progress);
    ESPhttpUpdate.onError(update_error);

    Serial.print("\nChecking for update from " + String(UpdateURL) + ". Current FW version " + String(FWVersion) + "\n");
    t_httpUpdate_return ret = ESPhttpUpdate.update(client, String(UpdateURL),String(FWVersion));

    switch (ret) {
      case HTTP_UPDATE_FAILED:
        Serial.printf("HTTP_UPDATE_FAILD Error (%d): %s\n", ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str());
        break;

      case HTTP_UPDATE_NO_UPDATES:
        Serial.println("HTTP_UPDATE_NO_UPDATES");
        break;

      case HTTP_UPDATE_OK:
        Serial.println("HTTP_UPDATE_OK");
        delay(5000);
        ESP.restart();
        break;
    }

  return 0;
}

int connect_wifi (){
  int retries = 0;
  int wifiStatus = WiFi.status();

  WiFi.forceSleepWake();
  delay(1);
  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  WiFi.config(ip, gateway, subnet);

  WiFi.begin( WifiSSID, WifiPass, WifiChannel, WifiBSSID, true );
  Serial.printf("[WIFI] ... quick connect, status: %d\nConnecting.", wifiStatus);

  while( wifiStatus != WL_CONNECTED ) {
    retries++;
    Serial.write('.');
    if( retries == 100 ) {
       Serial.printf("\n[WIFI] ... normal connect, status: %d\nConnecting.", wifiStatus);
       // Quick connect is not working, reset WiFi and try regular connection
      WiFi.disconnect();
      delay( 10 );
      WiFi.forceSleepBegin();
      delay( 10 );
      WiFi.forceSleepWake();
      delay( 10 );
      WiFi.begin( WifiSSID, WifiPass );
    }
    
    if( retries >= 600 ) {
      // Giving up after 30 seconds and going back to sleep
      WiFi.disconnect( true );
      delay( 1 );
      WiFi.mode( WIFI_OFF );
      gotoDeepSleep(sleepTimeS);
      return 1; // Not expecting this to be called, the previous call will never return.
    }
    delay( 50 );
    wifiStatus = WiFi.status();
  }

  Serial.print("Connected\n");

return wifiStatus;
}

int getRSSI(){
  glb_rssi=WiFi.RSSI();

  Serial.println("WiFi RSSI: " + String(glb_rssi) + "dBm");
  
  return 0; 
}

int GetHttpURL(String MyURL){
  Serial.print("[HTTP] begin...\n");

  http.begin(client,String(VeraBaseURL) + MyURL);
    
  int httpCode = http.GET();

   if (httpCode > 0) {
     Serial.printf("[HTTP] GET... code: %d\n", httpCode);
    
     if (httpCode == HTTP_CODE_OK) {
        http.writeToStream(&Serial);
        }
  }
  else {
    Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
    }
 
 return httpCode;
}

void setup() {
  Serial.begin(115200);
  Serial.print("\n" + String(ESPName) + " started\n");

  //Debug WiFi off
  Serial.setDebugOutput(false);

  //Disable WiFi status LED to save battery
  wifi_status_led_uninstall();

  // Connect WiFi
  connect_wifi();
  
  http.setReuse(true);
  
  GetHttpURL("data_request?id=variableset&DeviceNum=" + String(VeraDeviceID) + "&serviceId=urn:micasaverde-com:serviceId:SecuritySensor1&Variable=Tripped&Value=1");

  getRSSI();

  GetHttpURL("data_request?id=variableset&DeviceNum=" + String(VeraDeviceID) + "&serviceId=urn:micasaverde-com:serviceId:SecuritySensor1&Variable=CurrentRSSI&Value=" + String(glb_rssi));
  GetHttpURL("data_request?id=variableset&DeviceNum=" + String(VeraDeviceID) + "&serviceId=urn:micasaverde-com:serviceId:SecuritySensor1&Variable=AutoUntrip&Value=300");

  http.end();

  //Check for OTA update and update if OTA_TRIGGER_PIN is LOW.
  checkForUpdates();
  
  gotoDeepSleep(sleepTimeS);
}

void loop() {
// Nothing Here
}
//EOF