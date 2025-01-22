//ESP32C3 Super Mini Dev Module, USB CDC on Boot "Enabled", Integrated USB JTAG, Enabled, Default 4MB(1.2MB APP/1.5MB SPIFFS), 160MHz(WiFi), DIO, 80MHz, 4MB(32Mb), 921600, None, Disabled on COM5
//ESP32-2
//

#include <Arduino.h>
#include <WiFi.h>
#include "time.h"
#include <TimeLib.h>
#include <ESP_Google_Sheet_Client.h>
#include <PubSubClient.h>

//Define pins connected to ssd1306 display
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
