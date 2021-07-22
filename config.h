/* ESP8266 12F + CS2105GO-S10 Smoke Sersor with Vera integration config*/

IPAddress ip(192,168,1,42);
IPAddress gateway(192,168,1,1);
IPAddress subnet(255,255,255,0);

const char* WifiSSID      = "SSID";
const char* WifiPass      = "PASSWORD"; 
uint8_t WifiBSSID[6]      = {0x31,0xE3,0x3D,0x1F,0xF2,0x77};
uint8_t WifiChannel       = 11;

const int sleepTimeS      = 21600; // Sleep time in seconds, smoke sensor is not supposed to ever wake up using a timer.

const char* ESPName = "ESP8266-smoke-sensor";
int VeraDeviceID = 142;
const char* VeraBaseURL = "http://192.168.1.42:3480/";
const char* UpdateURL = "http://192.168.1.254:4080/8266OTA.php";
const char* FWVersion = "22072021";
