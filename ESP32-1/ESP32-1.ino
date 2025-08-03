//ESP32C3 Super Mini Dev Module, USB CDC on Boot "Enabled", Integrated USB JTAG, Enabled, Default 4MB(1.2MB APP/1.5MB SPIFFS), 160MHz(WiFi), DIO, 80MHz, 4MB(32Mb), 921600, None, Disabled on COM4
//ESP32-1
//https://github.com/TCM14/atc-gsheets/ESP32-1.ino
#include <esp_task_wdt.h>
#include <WiFi.h>
#include "time.h"
#include <TimeLib.h>
#include <ESP_Google_Sheet_Client.h>
#include "ATC_MiThermometer.h"
const String MYHOSTNAME = "ESP32-1TEMP";
const char* ssid = "your_wifi_ssid1";
const char* password = "your_wifi_password1";
const int scanTime = 5;  // BLE scan time in seconds
#define NUM_OF_SENSORS 3
#define WDT_TIMEOUT_SECONDS 10

int retries = 0;
int entries = 0;
unsigned long PrevSensorMillis = 0;
unsigned long PrevGSheetMillis = 0;
unsigned long PrevDbgPrintMillis = 0;
typedef struct {
  String temp;
  String humidity;
  String bat_volt;
  String bat_percentage;
  String rssi;
} SensorData_t;
SensorData_t sensorData[NUM_OF_SENSORS];
String data_label[] = { "temp", "rh", "bat_volt", "bat_pct", "rssi" };

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
#define SENSOR_MAC_LIVING "a4:c1:38:xx:xx:xx"
#define SENSOR_MAC_STUDY "a4:c1:38:xx:xx:xx"
#define SENSOR_MAC_BED "a4:c1:38:xx:xx:xx"
String sensor_label[NUM_OF_SENSORS] = { "Living", "Bed", "Study"};
std::vector<std::string> knownBLEAddresses = { SENSOR_MAC_LIVING, SENSOR_MAC_BED, SENSOR_MAC_STUDY };
ATC_MiThermometer miThermometer(knownBLEAddresses);
bool data_ready = false;

void setup() {
  Serial.begin(115200);
  initWifiStation();
  delay(500);
  miThermometer.begin();

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

  // Initialise the watchdog
  esp_task_wdt_deinit();  // De-initialize if already enabled
  esp_task_wdt_config_t wdt_config = {
    .timeout_ms = WDT_TIMEOUT_SECONDS * 1000,
    .idle_core_mask = (1 << portNUM_PROCESSORS) - 1,  // Mask for all cores
    .trigger_panic = true                             // Restart ESP32 on timeout
  };
  esp_task_wdt_init(&wdt_config);
  esp_task_wdt_add(NULL);  // Add current task (loopTask) to watchdog

  delay(1000);  //delay 1000ms for internet connection and timesyc to be ready
  PrevSensorMillis = 0;
  PrevDbgPrintMillis = 0;
  PrevGSheetMillis = millis();
}

void loop() {
  esp_task_wdt_reset();
  // Check sensor data every 30s
  if (millis() - PrevSensorMillis >= 30000) {
    PrevSensorMillis = millis();
    miThermometer.resetData();
    unsigned found = miThermometer.getData(scanTime);
    for (int i = 0; i < miThermometer.data.size(); i++) {
      if (miThermometer.data[i].valid) {
        Serial.printf("i = %d: %10s, ", i, sensor_label[i].c_str());
        Serial.printf("%s: %.2f, ", data_label[0].c_str(), miThermometer.data[i].temperature / 100.0);
        Temperature = miThermometer.data[i].temperature / 100.0;
        Serial.printf("%s: %.2f, ", data_label[1].c_str(), miThermometer.data[i].humidity / 100.0);
        Humidity = miThermometer.data[i].humidity / 100.0;
        Serial.printf("%s: %.3f, ", data_label[2].c_str(), miThermometer.data[i].batt_voltage / 1000.0);
        Voltage = miThermometer.data[i].batt_voltage / 1000.0;
        char sbuff[8];
        dtostrf(Voltage, 4, 3, sbuff);
        Serial.printf("%s: %3d, ", data_label[3].c_str(), miThermometer.data[i].batt_level);
        Percent = miThermometer.data[i].batt_level;
        Serial.printf("%s: %d\n", data_label[4].c_str(), miThermometer.data[i].rssi);
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
        data_ready = true;
      }
    }
    Serial.println();
    miThermometer.clearScanResults();
  }


  if (millis() - PrevGSheetMillis >= 200) {
    PrevGSheetMillis = millis();
    bool gsheet_ready = GSheet.ready();
    unsigned long locTime = getTime();
    //Update the google sheet every 60 seconds
    if (data_ready && gsheet_ready && (locTime % 60 == 0)) {
      data_ready = false;
      RxSts = false;
      FirebaseJson response;
      FirebaseJson valueRange;
      offsetTime();  //subroutine to calculate local time from epoch time
      String StrA1 = "";
      bool GetSucess = GSheet.values.get(&response /* returned response */, spreadsheetId /* spreadsheet Id to read */, sheetTag /* range to read */);
      // Serial.print("GetSucess res:");
      // Serial.println(GetSucess);
      if (GetSucess) {
        // response.toString(Serial, true);
        // response.toString(StrA1, true);
        uint index1 = StrA1.indexOf("Time", 0);
        StrA1 = StrA1.substring(index1, index1 + 4);
        //Check if the sheet is blank, if blank sheet is detected, add the title on first row
        if (StrA1 == "Time") {
          Serial.println("GetSucess, A1 = Time");
        } else {
          String label;
          Serial.println("GetSucess, A1 <> Time");
          valueRange.add("majorDimension", "COLUMNS");
          valueRange.set("values/[0]/[0]", "Time");
          // sensor 0 temperature
          label = sensor_label[0] + String("") + data_label[0];
          valueRange.set("values/[1]/[0]", label.c_str());
          // sensor 0 humidity
          label = sensor_label[0] + String("") + data_label[1];
          valueRange.set("values/[2]/[0]", label.c_str());
          // sensor 0 battery percentage
          label = sensor_label[0] + String("") + data_label[2];
          valueRange.set("values/[3]/[0]", label.c_str());
          // sensor 0 battery voltage
          label = sensor_label[0] + String("") + data_label[3];
          valueRange.set("values/[4]/[0]", label.c_str());
          // sensor 0 RSSI
          label = sensor_label[0] + String("") + data_label[4];
          valueRange.set("values/[5]/[0]", label.c_str());
          // sensor 1 temperature
          label = sensor_label[1] + String("") + data_label[0];
          valueRange.set("values/[6]/[0]", label.c_str());
          // sensor 1 humidity
          label = sensor_label[1] + String("") + data_label[1];
          valueRange.set("values/[7]/[0]", label.c_str());
          // sensor 1 battery percentage
          label = sensor_label[1] + String("") + data_label[2];
          valueRange.set("values/[8]/[0]", label.c_str());
          // sensor 1 battery voltage
          label = sensor_label[1] + String("") + data_label[3];
          valueRange.set("values/[9]/[0]", label.c_str());
          // sensor 1 RSSI
          label = sensor_label[1] + String("") + data_label[4];
          valueRange.set("values/[10]/[0]", label.c_str());
          // sensor 2 temperature
          label = sensor_label[2] + String("") + data_label[0];
          valueRange.set("values/[11]/[0]", label.c_str());
          // sensor 2 humidity
          label = sensor_label[2] + String("") + data_label[1];
          valueRange.set("values/[12]/[0]", label.c_str());
          // sensor 2 battery percentage
          label = sensor_label[2] + String("") + data_label[2];
          valueRange.set("values/[13]/[0]", label.c_str());
          // sensor 2 battery voltage
          label = sensor_label[2] + String("") + data_label[3];
          valueRange.set("values/[14]/[0]", label.c_str());
          // sensor 2 RSSI
          label = sensor_label[2] + String("") + data_label[4];
          valueRange.set("values/[15]/[0]", label.c_str());
          bool success = GSheet.values.update(&response /* returned response */, spreadsheetId /* spreadsheet Id to append */, sheetTag /*"Sheet1!A1"*/ /* range to append */, &valueRange /* data range to append */);
          if (success) {
            // response.toString(Serial, true);
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
      valueRange.set("values/[11]/[0]", sensorData[2].temp);
      valueRange.set("values/[12]/[0]", sensorData[2].humidity);
      valueRange.set("values/[13]/[0]", sensorData[2].bat_percentage);
      valueRange.set("values/[14]/[0]", sensorData[2].bat_volt);
      valueRange.set("values/[15]/[0]", sensorData[2].rssi);
      entries++;
      //Append row data to Google sheet (create new row)
      bool success = GSheet.values.append(&response /* returned response */, spreadsheetId /* spreadsheet Id to append */, sheetTag /* range to append */, &valueRange /* data range to append */);
      if (success) {
        // response.toString(Serial, true);
      } else {
        Serial.println(GSheet.errorReason());
        retries++;
        if (retries >= 3) {
          ESP.restart();
        }
      }
    }
  }
  if (millis() - PrevDbgPrintMillis >= 300000) {
    // print the heap usage every 5 minutes
    PrevDbgPrintMillis = millis();
    //Show the current heap state of ESP32 for debug
    Serial.print("Size:");
    Serial.print(ESP.getHeapSize());
    Serial.print(", Free:");
    Serial.print(ESP.getFreeHeap());
    Serial.print(", Min:");
    Serial.println(ESP.getMinFreeHeap());
  }
  delay(100);
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
  WiFi.setTxPower(WIFI_POWER_2dBm);
  delay(500);
  int currentPower = WiFi.getTxPower();
  Serial.println(String("\nConnected to the WiFi network (") + WiFi.SSID() + ")[" + WiFi.RSSI() + "], TX power: " + String(currentPower) + " dBm");
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
  // Serial.print("TimeStamp = ");
  // Serial.print(TimeStamp);
  // Serial.print(" , sheetTag = ");
  // Serial.println(sheetTag);
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