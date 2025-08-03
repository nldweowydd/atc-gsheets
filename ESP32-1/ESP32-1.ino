//ESP32C3 Super Mini Dev Module, USB CDC on Boot "Enabled", Integrated USB JTAG, Enabled, Default 4MB(1.2MB APP/1.5MB SPIFFS), 160MHz(WiFi), DIO, 80MHz, 4MB(32Mb), 921600, None, Disabled on COM4
//ESP32-1
//https://github.com/TCM14/atc-gsheets/ESP32-1.ino
#include <WiFi.h>
#include "time.h"
#include <TimeLib.h>
#include <ESP_Google_Sheet_Client.h>
#include "ATC_MiThermometer.h"
const String MYHOSTNAME = "ESP32-1TEMP";
const char* ssid = "your_wifi_ssid1";
const char* password = "your_wifi_password1";
const int scanTime = 5;  // BLE scan time in seconds
#define NUM_OF_SENSORS 2

int retries = 0;
int loopCount = 0;
int entries = 0;
unsigned long PrevMillis = 0;
typedef struct {
    String temp;
    String humidity;
    String bat_percentage;
    String bat_volt;
    String rssi;
} SensorData_t;
SensorData_t sensorData[NUM_OF_SENSORS];

// Google Project ID
#define PROJECT_ID "---"

// Service Account's client email
#define CLIENT_EMAIL "---"

// Service Account's private key
const char PRIVATE_KEY[] PROGMEM = "---";

// The ID of the spreadsheet where you'll publish the data
const char spreadsheetId[] = "---";

// Token Callback function
void tokenStatusCallback(TokenInfo info);

// NTP server to request epoch time
const char* ntpServer = "---";

// Variable to save current epoch time
char TimeStamp[20], sheetTag[10];
struct tm timeinfo;
bool RxSts = false;

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

    //Configure time
  configTime(0, 0, ntpServer);

  GSheet.printf("ESP Google Sheet Client v%s\n\n", ESP_GOOGLE_SHEET_CLIENT_VERSION);

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
        sensorData[i].temp = String(Temperature);
        sensorData[i].temp = sensorData[i].temp.substring(0, 5);
        sensorData[i].humidity = String(Humidity);
        sensorData[i].humidity = sensorData[i].humidity.substring(0, 5);
        sensorData[i].bat_volt = String(sbuff);
        sensorData[i].bat_volt = sensorData[i].bat_volt.substring(0, 5);
        sensorData[i].bat_percentage = String(round(Percent));
        sensorData[i].bat_percentage = sensorData[i].bat_percentage.substring(0, 3);
        sensorData[i].rssi = String(BLErssi);
        if (BLErssi > -99) {
          sensorData[i].rssi = sensorData[i].rssi.substring(0, 3);
        } else {
          sensorData[i].rssi = sensorData[i].rssi.substring(0, 4);
        }
        delay(50);
    }
    }
    miThermometer.clearScanResults();
  }


  delay(10);
  bool ready = GSheet.ready();
  unsigned long locTime = getTime();
  //Update the google sheet every 15 minutes(no data from MQTT) or update data from MQTT every 60 seconds
  // if (ready && (locTime % 900 == 0 || RxSts && locTime % 60 == 0)) {
  if (ready && (locTime % 60 == 0)) {
    RxSts = false;
    FirebaseJson response;
    FirebaseJson valueRange;
    // mqttClient.publish(SUB_SYNC, "1", true);
    // Serial.println("TX - SYNC.");
    // delay(100);
    // mqttClient.loop();
    // delay(100);
    offsetTime();  //subroutine to calculate local time from epoch time
    String StrA1 = "";
    bool GetSucess = GSheet.values.get(&response /* returned response */, spreadsheetId /* spreadsheet Id to read */, sheetTag /* range to read */);
    Serial.print("GetSucess res:");
    Serial.println(GetSucess);
    if (GetSucess) {
      response.toString(Serial, true);
      response.toString(StrA1, true);
      uint index1 = StrA1.indexOf("Time", 0);
      StrA1 = StrA1.substring(index1, index1 + 4);
      //Check if the sheet is blank, if blank sheet is detected, add the title on first row
      if (StrA1 == "Time") {
        Serial.print("GetSucess, A1 = Time");
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
    valueRange.set("values/[1]/[0]", sensorData[0].temp);
    valueRange.set("values/[2]/[0]", sensorData[0].humidity);
    valueRange.set("values/[3]/[0]", sensorData[0].bat_percentage);
    valueRange.set("values/[4]/[0]", sensorData[0].bat_volt);
    valueRange.set("values/[5]/[0]", sensorData[0].rssi);
    valueRange.set("values/[6]/[0]", sensorData[1].temp);
    valueRange.set("values/[7]/[0]", sensorData[1].humidity);
    valueRange.set("values/[8]/[0]", sensorData[1].bat_percentage);
    valueRange.set("values/[9]/[0]", sensorData[1].bat_volt);
    valueRange.set("values/[10]/[0]", sensorData[1].rssi);
    entries++;
    //Append row data to Google sheet (create new row)
    bool success = GSheet.values.append(&response /* returned response */, spreadsheetId /* spreadsheet Id to append */, sheetTag /* range to append */, &valueRange /* data range to append */);
    if (success) {
      response.toString(Serial, true);
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