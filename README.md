###Tong-man's MiSensor to Google Sheets project
The MiSensor to Google Sheets project enables real-time data logging from MiSensor devices using the ESP32 microcontrollers. By leveraging the power of cloud technology and automation, this project allows users to easily monitor and analyze sensor data directly in Google Sheets.

### **Components Used**
### ESP32 Supermini (2 pcs):
ESP32-1 to receive BLE beacon packets, convert them to data and send to MQTT broker every 30-second(data changed) or 5-minute(no data change).
ESP32-2 to subscribe MQTT messages and convert to data to a 0.96" OLED display for debugging and then send the real time data to Google sheets with time tag every 60-second(data change detected) or 15-minute(no data change).
### Xiaomi Thermometer LYWSD03MMC (Later refer as MiSensor)(2 pcs):
Two MiSensor modules were loaded with customized firmware "atc1441". They sends the sensor data via BLE beacons periodically with "Advertising interval" configured in the fimware.
*Firmware tool: [https://atc1441.github.io/TelinkFlasher.html](https://atc1441.github.io/TelinkFlasher.html)
*ATC_MiThermometer projects: [https://github.com/atc1441/ATC_MiThermometer](https://github.com/atc1441/ATC_MiThermometer)
### Google Sheets:
 A cloud-based spreadsheet application used for data visualization and analysis.
Manual for Google Sheets Client for ESP32 in Arduino IDE:
[https://randomnerdtutorials.com/esp32-datalogging-google-sheets/](https://randomnerdtutorials.com/esp32-datalogging-google-sheets/)
### Key Features
### Real-Time Data Transfer: 
The ESP32-1 collects data from the MiSensor and sends it to a MQTT for ESP32-2 to collect the data. I use MQTT tools to debug and MQTT/IoT dashboard to show the data in HMI.
### Data Logging: 
ESP32-2 receive the sensor data from MQTT and send them to Google Sheet at specified intervals, making it easy to track changes over time.
### Easy Access: 
The data stored in Google Sheets can be accessed from anywhere, enabling remote monitoring and reporting.
### Visualization Tools: 
Utilize Google Sheets’ built-in features to create graphs and charts for better data representation.

### Implementation Steps
### Step 1
Follow below links to configure your MiSensor modules with custom atc1441 firmware.
*Firmware tool:
[https://atc1441.github.io/TelinkFlasher.html](https://atc1441.github.io/TelinkFlasher.html)
*ATC_MiThermometer projects:
[https://github.com/atc1441/ATC_MiThermometer](https://github.com/atc1441/ATC_MiThermometer)

### Step 2
Follow the link to setup your Google Sheet account and private keys ready for Google Sheet API
[https://randomnerdtutorials.com/esp32-datalogging-google-sheets/](https://randomnerdtutorials.com/esp32-datalogging-google-sheets/)

### Step 3
Setup the ESP32-1 to collect the MiSensor BLE beacons from ESP32-1(ESP32 Supermini) and configure it to gather required data.
**Arduino IDE codes for ESP32-1:**
ESP32-1.ino
`//ESP32C3 Super Mini Dev Module, USB CDC on Boot "Enabled", Integrated USB JTAG, Enabled, Default 4MB(1.2MB APP/1.5MB SPIFFS), 160MHz(WiFi), DIO, 80MHz, 4MB(32Mb), 921600, None, Disabled on COM4
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
const int scanTime = 5;  // BLE scan time in seconds
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
std::vector<std::string> knownBLEAddresses = { "a4:c1:38:xx:xx:xx", "a4:c1:38:xx:xx:xx" };
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
  delay(100);  //Leave some time for mqttClient.loop to process CallBack function
  loopCount++;
  if (loopCount >= 6000 && (millis() - PrevMillis) >= 3600000) {
    loopCount = 0;
    PrevMillis = millis();
    retries++;
    initMQTTClient();
    tp = "0.0";
    hp = "0.0";
    vp = "0.0";
    pp = "0.0";
    sp = "0.0";
    tp2 = "0.0";
    hp2 = "0.0";
    vp2 = "0.0";
    pp2 = "0.0";
    sp2 = "0.0";
  }
  if (loopCount >= 200 & (millis() - PrevMillis) >= 30000) {  // Check sensor data every 30,000ms
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
    if (mqttClient.connected()) retries = 0;
    // Below lines compare the current BLE value h with previous value hp, h: humidity, t: temperature, etc...
    if (mqttClient.connected() and ((h.toFloat() >= hp.toFloat() * 1.01) or (h.toFloat() <= hp.toFloat() * 0.99))) {
      mqttClient.publish(SUB_RH, (char*)h.c_str(), true);
      hp = h;
    }
    if (mqttClient.connected() and ((t.toFloat() >= tp.toFloat() + 0.01)) or (t.toFloat() <= tp.toFloat() - 0.01)) {
      mqttClient.publish(SUB_TEMP, (char*)t.c_str(), true);
      tp = t;
    }
    if (mqttClient.connected() and ((v.toFloat() >= vp.toFloat() + 0.01)) or (v.toFloat() <= vp.toFloat() - 0.01)) {
      mqttClient.publish(SUB_V, (char*)v.c_str(), true);
      vp = v;
    }
    if (mqttClient.connected() and (p.toFloat() != pp.toFloat())) {
      mqttClient.publish(SUB_P, (char*)p.c_str(), true);
      pp = p;
    }
    if (mqttClient.connected() and (s.toFloat() != sp.toFloat())) {
      mqttClient.publish(SUB_S, (char*)s.c_str(), true);
      sp = s;
    }
    if (mqttClient.connected() and ((h2.toFloat() >= hp2.toFloat() * 1.01) or (h2.toFloat() <= hp2.toFloat() * 0.99))) {
      mqttClient.publish(SUB_RH2, (char*)h2.c_str(), true);
      hp2 = h2;
    }
    if (mqttClient.connected() and ((t2.toFloat() >= tp2.toFloat() + 0.01)) or (t2.toFloat() <= tp2.toFloat() - 0.01)) {
      mqttClient.publish(SUB_TEMP2, (char*)t2.c_str(), true);
      tp2 = t2;
    }
    if (mqttClient.connected() and ((v2.toFloat() >= vp2.toFloat() + 0.01)) or (v2.toFloat() <= vp2.toFloat() - 0.01)) {
      mqttClient.publish(SUB_V2, (char*)v2.c_str(), true);
      vp2 = v2;
    }
    if (mqttClient.connected() and (p2.toFloat() != pp2.toFloat())) {
      mqttClient.publish(SUB_P2, (char*)p2.c_str(), true);
      pp2 = p2;
    }
    if (mqttClient.connected() and (s2.toFloat() != sp2.toFloat())) {
      mqttClient.publish(SUB_S2, (char*)s2.c_str(), true);
      sp2 = s2;
    }
    delay(100);  //delay some time for MQTT to publish and callback
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
        WiFi.mode(WIFI_OFF);
        WiFi.mode(WIFI_AP_STA);
        delay(500);
        WiFi.begin(ssid1, password1);
        Serial.println(String("\nTrying to Connect WiFi network (") + String(ssid1) + ")");
      }
    }
    if ((retries >= 20) and (retries < 30)) {
      if (retries == 20) {
        WiFi.mode(WIFI_OFF);
        WiFi.mode(WIFI_AP_STA);
        delay(500);
        WiFi.begin(ssid2, password2);
        Serial.println(String("\nTrying to Connect WiFi network (") + String(ssid2) + ")");
      }
    }
    if (retries >= 30) {
      ESP.restart();
    }
  }
  Serial.println(String("\nConnected to the WiFi network (") + WiFi.SSID() + ")[" + WiFi.RSSI() + "]");
}

void initMQTTClient() {
  // Connecting to MQTT server
  mqttClient.setServer(mqttServer, mqttPort);
  mqttClient.setCallback(PubSubCallback);
  while (!mqttClient.connected()) {
    Serial.println(String("Connecting to MQTT (") + mqttServer + ")...");
    if (mqttClient.connect(mqttClientName, mqttUser, mqttPassword)) {
      Serial.println(String("MQTT client (") + mqttClientName + ") connected");
    } else {
      Serial.print("\nFailed with state ");
      Serial.println(mqttClient.state());
      if (WiFi.status() != WL_CONNECTED) {
        initWifiStation();
      }
      delay(2000);
      retries++;
      if (retries >= 5) ESP.restart();
    }
  }
  mqttClient.publish(SUB_RST, "0", true);
  delay(50);
  mqttClient.loop();
  delay(50);
  mqttClient.subscribe(SUB_TEMP);
  mqttClient.subscribe(SUB_RH);
  mqttClient.subscribe(SUB_V);
  mqttClient.subscribe(SUB_P);
  mqttClient.subscribe(SUB_S);
  mqttClient.subscribe(SUB_TEMP2);
  mqttClient.subscribe(SUB_RH2);
  mqttClient.subscribe(SUB_V2);
  mqttClient.subscribe(SUB_P2);
  mqttClient.subscribe(SUB_S2);
  mqttClient.subscribe(SUB_RST);
  mqttClient.subscribe(SUB_SYNC);
  delay(50);
  retries = 0;
}

void PubSubCallback(char* topic, byte* payload, unsigned int length) {
  String strTopicTemp = SUB_TEMP;
  String strTopicRH = SUB_RH;
  String strTopicV = SUB_V;
  String strTopicP = SUB_P;
  String strTopicS = SUB_S;
  String strTopicTemp2 = SUB_TEMP2;
  String strTopicRH2 = SUB_RH2;
  String strTopicV2 = SUB_V2;
  String strTopicP2 = SUB_P2;
  String strTopicS2 = SUB_S2;
  String strTopicRST = SUB_RST;
  String strTopicSYNC = SUB_SYNC;
  String strPayload = "";
  //Serial.print("RX - Topic : ");
  //Serial.print(topic);
  //Serial.print(", Message : ");
  for (int i = 0; i < length; i++) {
    //Serial.print((char)payload[i]);
    strPayload += (char)payload[i];
  }
  //Serial..print("  --->   ");
  if (strTopicSYNC == topic) {
    if (strPayload == "1") {
      PrevMillis = millis() + 10000;
      Serial.print("RX - SYNC, PrevMillis = ");
      Serial.print(PrevMillis);
      Serial.println(".");
      mqttClient.publish(SUB_SYNC, "0", true);
      delay(100);
      mqttClient.loop();
      delay(100);
    }
  }
  if (strTopicTemp == topic) {
    t = strPayload;
    //Serial..println("Temperature = " + strPayload);
  } else if (strTopicRH == topic) {
    h = strPayload;
    //Serial..println("Humidity = " + strPayload);
  } else if (strTopicV == topic) {
    v = strPayload;
    //Serial..println("Voltage = " + strPayload);
  } else if (strTopicP == topic) {
    p = strPayload;
    //Serial..println("Percent = " + strPayload);
  } else if (strTopicS == topic) {
    s = strPayload;
    //Serial..println("RSSI = " + strPayload);
  } else if (strTopicTemp2 == topic) {
    t2 = strPayload;
    //Serial..println("Temperature2 = " + strPayload);
  } else if (strTopicRH2 == topic) {
    h2 = strPayload;
    //Serial..println("Humidity2 = " + strPayload);
  } else if (strTopicV2 == topic) {
    v2 = strPayload;
    //Serial..println("Voltage2 = " + strPayload);
  } else if (strTopicP2 == topic) {
    p2 = strPayload;
    //Serial..println("Percent2 = " + strPayload);
  } else if (strTopicS2 == topic) {
    s2 = strPayload;
    //Serial..println("RSSI2 = " + strPayload);
  } else if (strTopicRST == topic) {
    delay(100);
    //Serial..println("Restart = " + strPayload);
    if (strPayload == "1") {
      Serial.println("Remote restart...");
      ESP.restart();
    }
  }
}
`
### Step 4
Setup the ESP32-2 to display data on 0.96" OLED diaply and use Google Sheets API:
Set up Google Sheets API to allow the ESP32 to send data to a specified spreadsheet.
**Arduino IDE codes for ESP32-2:**
`//ESP32C3 Super Mini Dev Module, USB CDC on Boot "Enabled", Integrated USB JTAG, Enabled, Default 4MB(1.2MB APP/1.5MB SPIFFS), 160MHz(WiFi), DIO, 80MHz, 4MB(32Mb), 921600, None, Disabled on COM5
//ESP32-2
//

#include <Arduino.h>
#include <WiFi.h>
#include "time.h"
#include <TimeLib.h>
#include <ESP_Google_Sheet_Client.h>
#include <PubSubClient.h>

#define I2C_SDA 9
#define I2C_SCL 8
#define VCC_PIN 7
#define GND_PIN 6
#include "SSD1306Wire.h"
// Initialize the OLED display using Wire library
SSD1306Wire display(0x3c, I2C_SDA, I2C_SCL);

const String MYHOSTNAME = "ESP32-2OLED";
const char* ssid = "your_wifi_ssid1";
const char* password = "your_wifi_password1";
const char* ssid1 = "your_wifi_ssid2";
const char* password1 = "your_wifi_password2";
const char* ssid2 = "your_wifi_ssid3";
const char* password2 = "your_wifi_password3";

const char* mqttServer = "test.mosquitto.org";
const char* mqttClientName = "ESP32-2OLED";
const int mqttPort = 1883;
const char* mqttUser = "";
const char* mqttPassword = "";

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

// Google Project ID
#define PROJECT_ID "your-project-id"

// Service Account's client email
#define CLIENT_EMAIL "user@your-project-id.iam.gserviceaccount.com"

// Service Account's private key
const char PRIVATE_KEY[] PROGMEM = "-----BEGIN PRIVATE KEY-----\n[COPY FROM GOOGLE WEB SITE]\n-----END PRIVATE KEY-----\n";

// The ID of the spreadsheet where you'll publish the data
const char spreadsheetId[] = "[COPY FROM GOOGLE WEB SITE]";

// Token Callback function
void tokenStatusCallback(TokenInfo info);

// NTP server to request epoch time
const char* ntpServer = "your_ntp_server";

// Variable to save current epoch time
char TimeStamp[20], sheetTag[10];
struct tm timeinfo;
bool RxSts = false;

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
int retries = 0;
int entries = 0;

// Function that gets current epoch time
unsigned long getTime() {
  time_t now;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
    ESP.restart();  //ESP force to restart, Google sheet cannot be operated without synchronized time or internet
  }
  time(&now);
  return now;
}
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println("Initializing OLED Display");
  pinMode(VCC_PIN, OUTPUT);
  digitalWrite(VCC_PIN, HIGH);
  pinMode(GND_PIN, OUTPUT);
  digitalWrite(GND_PIN, LOW);
  delay(300);
  display.init();
  display.flipScreenVertically();

  //Configure time
  configTime(0, 0, ntpServer);

  GSheet.printf("ESP Google Sheet Client v%s\n\n", ESP_GOOGLE_SHEET_CLIENT_VERSION);

  //Show current status on OLED screen for debug
  display.clear();
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.setFont(ArialMT_Plain_10);
  display.drawString(0, 0, "ESP GSheet Ver.:");
  display.drawString(80, 0, ESP_GOOGLE_SHEET_CLIENT_VERSION);
  display.drawString(0, 12, "Setting up WiFi ...");
  display.display();
  delay(200);
  initWifiStation();
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(1000);
  }
  initMQTTClient();
  display.setColor(BLACK);
  display.fillRect(0, 12, 128, 13);
  display.setColor(WHITE);
  display.drawString(0, 12, "Comm. IP:");
  display.drawString(60, 12, WiFi.localIP().toString());
  display.display();
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());

  // Set the callback for Google API access token generation status (for debug only)
  GSheet.setTokenCallback(tokenStatusCallback);

  // Set the seconds to refresh the auth token before expire (60 to 3540, default is 300 seconds)
  GSheet.setPrerefreshSeconds(10 * 60);

  // Begin the access token generation for Google API authentication
  GSheet.begin(CLIENT_EMAIL, PROJECT_ID, PRIVATE_KEY);
  Serial.print("Connecting Google ");

  delay(1000);  //delay 1000ms for internet connection and timesyc to be ready
}

void loop() {
  mqttClient.loop();
  delay(10);
  bool ready = GSheet.ready();
  unsigned long locTime = getTime();
  //Update the google sheet every 15 minutes(no data from MQTT) or update data from MQTT every 60 seconds
  if (ready && (locTime % 900 == 0 || RxSts && locTime % 60 == 0)) {
    RxSts = false;
    FirebaseJson response;
    FirebaseJson valueRange;
    mqttClient.publish(SUB_SYNC, "1", true);
    Serial.println("TX - SYNC.");
    delay(100);
    mqttClient.loop();
    delay(100);
    offsetTime();  //subroutine to calculate local time from epoch time
    String StrA1 = "";
    bool GetSucess = GSheet.values.get(&response /* returned response */, spreadsheetId /* spreadsheet Id to read */, sheetTag /* range to read */);
    if (GetSucess) {
      response.toString(Serial, true);
      response.toString(StrA1, true);
      uint index1 = StrA1.indexOf("Time", 0);
      StrA1 = StrA1.substring(index1, index1 + 4);
      //Check if the sheet is blank, if blank sheet is detected, add the title on first row
      if (StrA1 == "Time") {
        //Serial.print("GetSucess, A1 = Time");
      } else {
        Serial.print("GetSucess, A1 <> Time");
        valueRange.add("majorDimension", "COLUMNS");
        valueRange.set("values/[0]/[0]", "Time");
        valueRange.set("values/[1]/[0]", "Temp1(C)");
        valueRange.set("values/[2]/[0]", "RH1(%)");
        valueRange.set("values/[3]/[0]", "Batt Lvl1(%");
        valueRange.set("values/[4]/[0]", "Batt Volt1");
        valueRange.set("values/[5]/[0]", "RSSI(db)");
        valueRange.set("values/[6]/[0]", "Temp2(C)");
        valueRange.set("values/[7]/[0]", "RH2(%)");
        valueRange.set("values/[8]/[0]", "BattLvl2(%");
        valueRange.set("values/[9]/[0]", "Batt Volt2");
        valueRange.set("values/[10]/[0]", "RSSI2(db)");
        bool success = GSheet.values.update(&response /* returned response */, spreadsheetId /* spreadsheet Id to append */, sheetTag /*"Sheet1!A1"*/ /* range to append */, &valueRange /* data range to append */);
        if (success) {
          response.toString(Serial, true);
          retries = 0;
        } else {
          Serial.println(GSheet.errorReason());
          retries++;
          if (retries >= 3) {
            ESP.restart();
          }
        }
        //Show the current heap state of ESP32 for debug
        //Serial.print("Size:");
        //Serial.print(ESP.getHeapSize());
        //Serial.print(", Free:");
        //Serial.print(ESP.getFreeHeap());
        //Serial.print(", Min:");
        //Serial.println(ESP.getMinFreeHeap());
      }
    }
    //Prepare the Google sheet values to fit a ROW
    valueRange.add("majorDimension", "COLUMNS");
    valueRange.set("values/[0]/[0]", TimeStamp);
    valueRange.set("values/[1]/[0]", t);
    valueRange.set("values/[2]/[0]", h);
    valueRange.set("values/[3]/[0]", p);
    valueRange.set("values/[4]/[0]", v);
    valueRange.set("values/[5]/[0]", s);
    valueRange.set("values/[6]/[0]", t2);
    valueRange.set("values/[7]/[0]", h2);
    valueRange.set("values/[8]/[0]", p2);
    valueRange.set("values/[9]/[0]", v2);
    valueRange.set("values/[10]/[0]", s2);
    //Feed back on the OLED screen
    display.setColor(BLACK);
    display.fillRect(0, 24, 128, 40);
    display.setColor(WHITE);
    display.drawString(0, 24, TimeStamp);
    display.drawString(0, 37, t);
    display.drawString(30, 37, h);
    display.drawString(61, 37, p);
    display.drawString(80, 37, v);
    display.drawString(111, 37, s);
    display.drawString(0, 50, t2);
    display.drawString(30, 50, h2);
    display.drawString(61, 50, p2);
    display.drawString(80, 50, v2);
    display.drawString(111, 50, s2);
    display.display();
    delay(100);
    entries++;
    //Append row data to Google sheet (create new row)
    bool success = GSheet.values.append(&response /* returned response */, spreadsheetId /* spreadsheet Id to append */, sheetTag /* range to append */, &valueRange /* data range to append */);
    if (success) {
      //response.toString(Serial, true);
    } else {
      Serial.println(GSheet.errorReason());
      retries++;
      if (retries >= 3) {
        ESP.restart();
      }
    }
    //Show the current heap state of ESP32 for debug
    //Serial.print("Size:");
    //Serial.print(ESP.getHeapSize());
    //Serial.print(", Free:");
    //Serial.print(ESP.getFreeHeap());
    //Serial.print(", Min:");
    //Serial.println(ESP.getMinFreeHeap());
  }
  if (entries >= 20000) {
    ESP.restart();
  }
}

void offsetTime() {
  int yy, mm, dd, hh;
  yy = timeinfo.tm_year + 1900;
  mm = timeinfo.tm_mon + 1;
  dd = timeinfo.tm_mday;
  hh = timeinfo.tm_hour + 8;
  if (hh >= 24) {
    hh = hh - 24;
    dd = dd + 1;
    if (mm == 1 || 3 || 5 || 7 || 8 || 10 || 12) {
      if (dd > 31) {
        dd = 1;
        mm = mm + 1;
        if (mm > 12) {
          yy = yy + 1;
          mm = 1;
        }
      }
    } else {
      if (mm == 2) {
        if ((yy % 4 == 0 && yy % 100 != 0) || (yy % 400 == 0)) {
          if (dd > 29) {
            dd = 1;
            mm = mm + 1;
          }
        } else {
          if (dd > 28) {
            dd = 1;
            mm = mm + 1;
          }
        }
      } else {
        if (dd > 30) {
          dd = 1;
          mm = mm + 1;
        }
      }
    }
  }
  sprintf(TimeStamp, "%04d-%02d-%02d %02d:%02d:%02d", yy, mm, dd, hh, timeinfo.tm_min, timeinfo.tm_sec);
  sprintf(sheetTag, "%04d%02d!A1", yy, mm);
  Serial.print("TimeStamp = ");
  Serial.print(TimeStamp);
  Serial.print(" , sheetTag = ");
  Serial.println(sheetTag);
}

void initWifiStation() {
  WiFi.mode(WIFI_AP_STA);
  WiFi.hostname(MYHOSTNAME);
  WiFi.begin(ssid, password);
  Serial.print("\nConnecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
    retries++;
    if ((retries >= 10) and (retries < 20)) {
      if (retries == 10) {
        WiFi.mode(WIFI_OFF);
        WiFi.mode(WIFI_AP_STA);
        delay(500);
        WiFi.begin(ssid1, password1);
        Serial.println(String("\nTrying to Connect WiFi network (") + WiFi.SSID() + ")[" + WiFi.RSSI() + "]");
      }
    }
    if ((retries >= 20) and (retries < 30)) {
      if (retries == 20) {
        WiFi.mode(WIFI_OFF);
        WiFi.mode(WIFI_AP_STA);
        delay(500);
        WiFi.begin(ssid2, password2);
        Serial.println(String("\nTrying to Connect WiFi network (") + WiFi.SSID() + ")[" + WiFi.RSSI() + "]");
      }
    }
    if (retries >= 30) {
      ESP.restart();
    }
  }
  Serial.println(String("\nConnected to the WiFi network (") + WiFi.SSID() + ")[" + WiFi.RSSI() + "]");
}

void initMQTTClient() {
  // Connecting to MQTT server
  mqttClient.setServer(mqttServer, mqttPort);
  mqttClient.setCallback(PubSubCallback);
  while (!mqttClient.connected()) {
    Serial.println(String("Connecting to MQTT (") + mqttServer + ")...");
    if (mqttClient.connect(mqttClientName, mqttUser, mqttPassword)) {
      Serial.println(String("MQTT client (") + mqttClientName + ") connected");
    } else {
      Serial.print("\nFailed with state ");
      Serial.println(mqttClient.state());
      if (WiFi.status() != WL_CONNECTED) {
        initWifiStation();
      }
      delay(1000);
      retries++;
      if (retries >= 30) {
        ESP.restart();
      }
    }
  }
  mqttClient.publish(SUB_RST, "0", true);
  delay(50);
  mqttClient.loop();
  delay(50);
  mqttClient.subscribe(SUB_TEMP);
  mqttClient.subscribe(SUB_RH);
  mqttClient.subscribe(SUB_V);
  mqttClient.subscribe(SUB_P);
  mqttClient.subscribe(SUB_S);
  mqttClient.subscribe(SUB_TEMP2);
  mqttClient.subscribe(SUB_RH2);
  mqttClient.subscribe(SUB_V2);
  mqttClient.subscribe(SUB_P2);
  mqttClient.subscribe(SUB_S2);
  mqttClient.subscribe(SUB_RST);
  delay(50);
}

void PubSubCallback(char* topic, byte* payload, unsigned int length) {
  String strTopicTemp = SUB_TEMP;
  String strTopicRH = SUB_RH;
  String strTopicV = SUB_V;
  String strTopicP = SUB_P;
  String strTopicS = SUB_S;
  String strTopicTemp2 = SUB_TEMP2;
  String strTopicRH2 = SUB_RH2;
  String strTopicV2 = SUB_V2;
  String strTopicP2 = SUB_P2;
  String strTopicS2 = SUB_S2;
  String strTopicRST = SUB_RST;
  String strPayload = "";
  Serial.print("RX - Topic : ");
  Serial.print(topic);
  Serial.print(", Message : ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
    strPayload += (char)payload[i];
  }
  Serial.print("  --->   ");
  if (strTopicTemp == topic) {
    tp = t;
    t = strPayload;
    Serial.println("Temperature = " + strPayload);
    if (t != tp) RxSts = true;
  } else if (strTopicRH == topic) {
    hp = h;
    h = strPayload;
    Serial.println("Humidity = " + strPayload);
    if (h != hp) RxSts = true;
  } else if (strTopicV == topic) {
    vp = v;
    v = strPayload;
    Serial.println("Voltage = " + strPayload);
    if (v != vp) RxSts = true;
  } else if (strTopicP == topic) {
    pp = p;
    p = strPayload;
    Serial.println("Percent = " + strPayload);
    if (p != pp) RxSts = true;
  } else if (strTopicS == topic) {
    s = sp;
    s = strPayload;
    Serial.println("RSSI = " + strPayload);
    if (s.toInt() > sp.toInt() + 2 || s.toInt() < sp.toInt() - 2) RxSts = true;
  } else if (strTopicTemp2 == topic) {
    tp2 = t2;
    t2 = strPayload;
    Serial.println("Temperature2 = " + strPayload);
    if (t2 != tp2) RxSts = true;
  } else if (strTopicRH2 == topic) {
    hp2 = h2;
    h2 = strPayload;
    Serial.println("Humidity2 = " + strPayload);
    if (h2 != hp2) RxSts = true;
  } else if (strTopicV2 == topic) {
    vp2 = v2;
    v2 = strPayload;
    Serial.println("Voltage2 = " + strPayload);
    if (v2 != vp2) RxSts = true;
  } else if (strTopicP2 == topic) {
    pp2 = p2;
    p2 = strPayload;
    Serial.println("Percent2 = " + strPayload);
    if (p2 != pp2) RxSts = true;
  } else if (strTopicS2 == topic) {
    sp2 = s2;
    s2 = strPayload;
    Serial.println("RSSI2 = " + strPayload);
    if (s2.toInt() > sp2.toInt() + 2 || s2.toInt() < sp2.toInt() - 2) RxSts = true;
  } else if (strTopicRST == topic) {
    Serial.println("Restart = " + strPayload);
    if (strPayload == "1") {
      ESP.restart();
    }
  }
}

void tokenStatusCallback(TokenInfo info) {
  if (info.status == token_status_error) {
    GSheet.printf("Token info: type = %s, status = %s\n", GSheet.getTokenType(info).c_str(), GSheet.getTokenStatus(info).c_str());
    GSheet.printf("Token error: %s\n", GSheet.getTokenError(info).c_str());
    retries++;
    if (retries >= 5) {
      ESP.restart();
    }
  } else {
    GSheet.printf("Token info: type = %s, status = %s\n", GSheet.getTokenType(info).c_str(), GSheet.getTokenStatus(info).c_str());
    retries = 0;
  }
}
`
Testing: Verify the data logging process and ensure that the data appears correctly in the spreadsheet.
