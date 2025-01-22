//ESP32C3 Super Mini Dev Module, USB CDC on Boot "Enabled", Integrated USB JTAG, Enabled, Default 4MB(1.2MB APP/1.5MB SPIFFS), 160MHz(WiFi), DIO, 80MHz, 4MB(32Mb), 921600, None, Disabled on COM4
//ESP32-1 
//
#include <WiFi.h> 
#include <PubSubClient.h> 
#include "ATC_MiThermometer.h" 
const String MYHOSTNAME = "ESP32-1TEMP"; 
const char* ssid = "your_wifi_ssid1"; 
const char* password = "your_wifi_password1"; 
const char* ssid1 = "your_wifi_ssid2"; 
const char* password1 = "your_wifi_password2"; 
const char* ssid2 = "your_wifi_ssid3"; 
const char* password2 = "your_wifi_password3"; 
const int scanTime = 5; // BLE scan time in seconds 
const char* mqttServer = "test.mosquitto.org"; 
const char* mqttClientName = "ESP32-1TEMP"; 
const int mqttPort = 1883; 
const char* mqttUser = ""; 
const char* mqttPassword = "";

int retries = 0; 
int loopCount = 0; 
unsigned long PrevMillis = 0; 
String t = "25.5"; 
String tp = "25.5"; 
String h = "65.0"; 
String hp = "65.0"; 
String p = "80"; 
String pp = "80"; 
String v = "3.00"; 
String vp = "3.00"; 
String s = "-99.9"; 
String sp = "-99.9"; 
String t2 = "25.5"; 
String tp2 = "25.5"; 
String h2 = "65.0"; 
String hp2 = "65.0"; 
String p2 = "80"; 
String pp2 = "80"; 
String v2 = "3.00"; 
String vp2 = "3.00"; 
String s2 = "-99.9"; 
String sp2 = "-99.9"; 
#define SUB_TEMP "your_mqtt_path/esp32-1/temp" 
#define SUB_RH "your_mqtt_path/esp32-1/rh" 
#define SUB_V "your_mqtt_path/esp32-1/volt" 
#define SUB_P "your_mqtt_path/esp32-1/p" 
#define SUB_S "your_mqtt_path/esp32-1/rssi" 
#define SUB_TEMP2 "your_mqtt_path/esp32-1/temp2" 
#define SUB_RH2 "your_mqtt_path/esp32-1/rh2" 
#define SUB_V2 "your_mqtt_path/esp32-1/volt2" 
#define SUB_P2 "your_mqtt_path/esp32-1/p2" 
#define SUB_S2 "your_mqtt_path/esp32-1/rssi2" 
#define SUB_RST "your_mqtt_path/esp32-1/restart" 
#define SUB_SYNC "your_mqtt_path/esp32-1/sync" 
WiFiClient wifiClient; 
PubSubClient mqttClient(wifiClient); 
float Temperature = 0.0; 
float Humidity = 0.0; 
float Voltage = 0.0; 
float Percent = 0.0; 
float BLErssi = 0.0; 

// List of Mi sensors' BLE addresses 
std::vectorstd::string knownBLEAddresses = { "a4:c1:38:xx:xx:xx", "a4:c1:38:xx:xx:xx" }; 
ATC_MiThermometer miThermometer(knownBLEAddresses);

void setup() { 
  Serial.begin(115200); 
  initWifiStation();
  initMQTTClient();
  delay(500); 
  miThermometer.begin();
  loopCount = 180;
  PrevMillis = millis(); 
}

void loop() { 
mqttClient.loop();
delay(100); //Leave some time for mqttClient.loop to process CallBack function 
loopCount++; 
if (loopCount >= 6000 && (millis() - PrevMillis) >= 3600000) {
  loopCount = 0; 
  PrevMillis = millis();
  retries++; initMQTTClient();
  tp = "0.0"; hp = "0.0"; vp = "0.0"; pp = "0.0"; sp = "0.0"; tp2 = "0.0"; hp2 = "0.0"; vp2 = "0.0"; pp2 = "0.0"; sp2 = "0.0";
} 
 // Check sensor data every 30,000ms 
if (loopCount >= 200 & (millis() - PrevMillis) >= 30000) {
  PrevMillis = millis(); 
  miThermometer.resetData();
  unsigned found = miThermometer.getData(scanTime); 
  for (int i = 0; i < miThermometer.data.size(); i++) { 
    if (miThermometer.data[i].valid { 
      delay(50);
      Serial.printf("i = %d, ", i); 
      Serial.printf("Temp(°C):%.2f, ", miThermometer.data[i].temperature / 100.0);
      Temperature = miThermometer.data[i].temperature / 100.0; 
      Serial.printf("RH(%%):%.2f, ", miThermometer.data[i].humidity / 100.0); 
      Humidity = miThermometer.data[i].humidity / 100.0;
      Serial.printf("Volt:%.3f, ", miThermometer.data[i].batt_voltage / 1000.0); 
      Voltage = miThermometer.data[i].batt_voltage / 1000.0; 
      char sbuff[8]; 
      dtostrf(Voltage, 4, 3, sbuff); 
      Serial.printf("batt(%%):%d, ", miThermometer.data[i].batt_level); 
      Percent = miThermometer.data[i].batt_level; 
      Serial.printf("rssi(dBm):%d\n", miThermometer.data[i].rssi); 
      BLErssi = miThermometer.data[i].rssi;
      if (i < 1) { 
        t = String(Temperature);
        t = t.substring(0, 5); 
        h = String(Humidity);
        h = h.substring(0, 5); 
        v = String(sbuff);
        v = v.substring(0, 5);
        p = String(round(Percent));
        p = p.substring(0, 3);
        s = String(BLErssi);
        if (BLErssi > -99) {
          s = s.substring(0, 3);
        } else {
          s = s.substring(0, 4);
        }
      } else { 
        t2 = String(Temperature);
        t2 = t2.substring(0, 5); h2 = String(Humidity); h2 = h2.substring(0, 5); v2 = String(sbuff); v2 = v2.substring(0, 5); p2 = String(round(Percent)); p2 = p2.substring(0, 3); s2 = String(BLErssi); if (BLErssi > -99) { s2 = s2.substring(0, 3); } else { s2 = s2.substring(0, 4); } } delay(50); } } miThermometer.clearScanResults(); if (mqttClient.connected()) retries = 0; // Below lines compare the current BLE value h with previous value hp, h: humidity, t: temperature, etc... if (mqttClient.connected() and ((h.toFloat() >= hp.toFloat() * 1.01) or (h.toFloat() <= hp.toFloat() * 0.99))) { mqttClient.publish(SUB_RH, (char*)h.c_str(), true); hp = h; } if (mqttClient.connected() and ((t.toFloat() >= tp.toFloat() + 0.01)) or (t.toFloat() <= tp.toFloat() - 0.01)) { mqttClient.publish(SUB_TEMP, (char*)t.c_str(), true); tp = t; } if (mqttClient.connected() and ((v.toFloat() >= vp.toFloat() + 0.01)) or (v.toFloat() <= vp.toFloat() - 0.01)) { mqttClient.publish(SUB_V, (char*)v.c_str(), true); vp = v; } if (mqttClient.connected() and (p.toFloat() != pp.toFloat())) { mqttClient.publish(SUB_P, (char*)p.c_str(), true); pp = p; } if (mqttClient.connected() and (s.toFloat() != sp.toFloat())) { mqttClient.publish(SUB_S, (char*)s.c_str(), true); sp = s; } if (mqttClient.connected() and ((h2.toFloat() >= hp2.toFloat() * 1.01) or (h2.toFloat() <= hp2.toFloat() * 0.99))) { mqttClient.publish(SUB_RH2, (char*)h2.c_str(), true); hp2 = h2; } if (mqttClient.connected() and ((t2.toFloat() >= tp2.toFloat() + 0.01)) or (t2.toFloat() <= tp2.toFloat() - 0.01)) { mqttClient.publish(SUB_TEMP2, (char*)t2.c_str(), true); tp2 = t2; } if (mqttClient.connected() and ((v2.toFloat() >= vp2.toFloat() + 0.01)) or (v2.toFloat() <= vp2.toFloat() - 0.01)) { mqttClient.publish(SUB_V2, (char*)v2.c_str(), true); vp2 = v2; } if (mqttClient.connected() and (p2.toFloat() != pp2.toFloat())) { mqttClient.publish(SUB_P2, (char*)p2.c_str(), true); pp2 = p2; } if (mqttClient.connected() and (s2.toFloat() != sp2.toFloat())) { mqttClient.publish(SUB_S2, (char*)s2.c_str(), true); sp2 = s2; } delay(100); //delay some time for MQTT to publish and callback } }

void initWifiStation() { WiFi.mode(WIFI_AP_STA); WiFi.hostname(MYHOSTNAME); WiFi.begin(ssid, password); Serial.print("\nConnecting to WiFi "); while (WiFi.status() != WL_CONNECTED) { delay(1000); Serial.print("."); retries++; if ((retries >= 10) and (retries < 20)) { if (retries == 10) { WiFi.mode(WIFI_OFF); WiFi.mode(WIFI_AP_STA); delay(500); WiFi.begin(ssid1, password1); Serial.println(String("\nTrying to Connect WiFi network (") + String(ssid1) + ")"); } } if ((retries >= 20) and (retries < 30)) { if (retries == 20) { WiFi.mode(WIFI_OFF); WiFi.mode(WIFI_AP_STA); delay(500); WiFi.begin(ssid2, password2); Serial.println(String("\nTrying to Connect WiFi network (") + String(ssid2) + ")"); } } if (retries >= 30) { ESP.restart(); } } Serial.println(String("\nConnected to the WiFi network (") + WiFi.SSID() + ")[" + WiFi.RSSI() + "]"); }

void initMQTTClient() { // Connecting to MQTT server mqttClient.setServer(mqttServer, mqttPort); mqttClient.setCallback(PubSubCallback); while (!mqttClient.connected()) { Serial.println(String("Connecting to MQTT (") + mqttServer + ")..."); if (mqttClient.connect(mqttClientName, mqttUser, mqttPassword)) { Serial.println(String("MQTT client (") + mqttClientName + ") connected"); } else { Serial.print("\nFailed with state "); Serial.println(mqttClient.state()); if (WiFi.status() != WL_CONNECTED) { initWifiStation(); } delay(2000); retries++; if (retries >= 5) ESP.restart(); } } mqttClient.publish(SUB_RST, "0", true); delay(50); mqttClient.loop(); delay(50); mqttClient.subscribe(SUB_TEMP); mqttClient.subscribe(SUB_RH); mqttClient.subscribe(SUB_V); mqttClient.subscribe(SUB_P); mqttClient.subscribe(SUB_S); mqttClient.subscribe(SUB_TEMP2); mqttClient.subscribe(SUB_RH2); mqttClient.subscribe(SUB_V2); mqttClient.subscribe(SUB_P2); mqttClient.subscribe(SUB_S2); mqttClient.subscribe(SUB_RST); mqttClient.subscribe(SUB_SYNC); delay(50); retries = 0; }

void PubSubCallback(char* topic, byte* payload, unsigned int length) { String strTopicTemp = SUB_TEMP; String strTopicRH = SUB_RH; String strTopicV = SUB_V; String strTopicP = SUB_P; String strTopicS = SUB_S; String strTopicTemp2 = SUB_TEMP2; String strTopicRH2 = SUB_RH2; String strTopicV2 = SUB_V2; String strTopicP2 = SUB_P2; String strTopicS2 = SUB_S2; String strTopicRST = SUB_RST; String strTopicSYNC = SUB_SYNC; String strPayload = ""; //Serial.print("RX - Topic : "); //Serial.print(topic); //Serial.print(", Message : "); for (int i = 0; i < length; i++) { //Serial.print((char)payload[i]); strPayload += (char)payload[i]; } //Serial..print(" ---> "); if (strTopicSYNC == topic) { if (strPayload == "1") { PrevMillis = millis() + 10000; Serial.print("RX - SYNC, PrevMillis = "); Serial.print(PrevMillis); Serial.println("."); mqttClient.publish(SUB_SYNC, "0", true); delay(100); mqttClient.loop(); delay(100); } } if (strTopicTemp == topic) { t = strPayload; //Serial..println("Temperature = " + strPayload); } else if (strTopicRH == topic) { h = strPayload; //Serial..println("Humidity = " + strPayload); } else if (strTopicV == topic) { v = strPayload; //Serial..println("Voltage = " + strPayload); } else if (strTopicP == topic) { p = strPayload; //Serial..println("Percent = " + strPayload); } else if (strTopicS == topic) { s = strPayload; //Serial..println("RSSI = " + strPayload); } else if (strTopicTemp2 == topic) { t2 = strPayload; //Serial..println("Temperature2 = " + strPayload); } else if (strTopicRH2 == topic) { h2 = strPayload; //Serial..println("Humidity2 = " + strPayload); } else if (strTopicV2 == topic) { v2 = strPayload; //Serial..println("Voltage2 = " + strPayload); } else if (strTopicP2 == topic) { p2 = strPayload; //Serial..println("Percent2 = " + strPayload); } else if (strTopicS2 == topic) { s2 = strPayload; //Serial..println("RSSI2 = " + strPayload); } else if (strTopicRST == topic) { delay(100); //Serial..println("Restart = " + strPayload); if (strPayload == "1") { Serial.println("Remote restart..."); ESP.restart(); } } } 
