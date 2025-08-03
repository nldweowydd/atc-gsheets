//ESP32C3 Super Mini Dev Module, USB CDC on Boot "Enabled", Integrated USB JTAG, Enabled, Default 4MB(1.2MB APP/1.5MB SPIFFS), 160MHz(WiFi), DIO, 80MHz, 4MB(32Mb), 921600, None, Disabled on COM4
//ESP32-1
//https://github.com/TCM14/atc-gsheets/ESP32-1.ino
#include <WiFi.h>
#include "ATC_MiThermometer.h"
const String MYHOSTNAME = "ESP32-1TEMP";
const char* ssid = "your_wifi_ssid1";
const char* password = "your_wifi_password1";
const int scanTime = 5;  // BLE scan time in seconds

int retries = 0;
int loopCount = 0;
int entries = 0;

WiFiClient wifiClient;
float Temperature = 0.0;
float Humidity = 0.0;
float Voltage = 0.0;
float Percent = 0.0;
float BLErssi = 0.0;

// List of Mi sensors' BLE addresses
std::vector<std::string> knownBLEAddresses = { "a4:c1:38:xx:xx:xx", "a4:c1:38:xx:xx:xx" };
ATC_MiThermometer miThermometer(knownBLEAddresses);

void setup() {
  Serial.begin(115200);
  initWifiStation();
  delay(500);
  miThermometer.begin();
  loopCount = 180;
  PrevMillis = millis();
}

void loop() {
  delay(100);  //Leave some time for mqttClient.loop to process CallBack function
  loopCount++;
  // Check sensor data every 30,000ms
  if (loopCount >= 200 & (millis() - PrevMillis) >= 30000) {
    PrevMillis = millis();
    miThermometer.resetData();
    unsigned found = miThermometer.getData(scanTime);
    for (int i = 0; i < miThermometer.data.size(); i++) {
    if (miThermometer.data[i].valid) {
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
          t2 = t2.substring(0, 5);
          h2 = String(Humidity);
          h2 = h2.substring(0, 5);
          v2 = String(sbuff);
          v2 = v2.substring(0, 5);
          p2 = String(round(Percent));
          p2 = p2.substring(0, 3);
          s2 = String(BLErssi);
          if (BLErssi > -99) {
            s2 = s2.substring(0, 3);
          } else {
            s2 = s2.substring(0, 4);
          }
        }
        delay(50);
    }
    }
    miThermometer.clearScanResults();
  }
  }
}

void initWifiStation() {
  WiFi.mode(WIFI_AP_STA);
  WiFi.hostname(MYHOSTNAME);
  WiFi.begin(ssid, password);
  Serial.print("\nConnecting to WiFi ");
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
    retries++;
    if ((retries >= 10) and (retries < 20)) {
      if (retries == 10) {
        delay(500);
        Serial.println("\nFailed to connect to the WiFi network. Power cycling the device...");
        ESP.restart();
      }
    }
  }
  Serial.println(String("\nConnected to the WiFi network (") + WiFi.SSID() + ")[" + WiFi.RSSI() + "]");
}
