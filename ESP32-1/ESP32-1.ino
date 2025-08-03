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
#define WDT_TIMEOUT_SECONDS 10
#define ARRAY_SIZE(a) (sizeof(a)/sizeof(a[0]))

int retries = 0;
int entries = 0;
unsigned long prevSensorMillis = 0;
unsigned long prevGSheetMillis = 0;
unsigned long prevDbgPrintMillis = 0;
enum SensorLoc_e {
  SENSOR_LOC_LIVING = 0,
  SENSOR_LOC_BED,
  SENSOR_LOC_STUDY,
  SENSOR_LOC_NUM
};
enum SensorType_e {
  SENSOR_TEMP = 0,
  SENSOR_HUMIDITY,
  SENSOR_BATTERY_VOLTAGE,
  SENSOR_BATTERY_LEVEL,
  SENSOR_RSSI,
  SENSOR_NUM
};
typedef struct {
  String data[SENSOR_NUM];
} SensorDataSet_t;
SensorDataSet_t sensorData[SENSOR_LOC_NUM];
String sensorLabel[] = { "temp", "rh", "bat_volt", "bat_pct", "rssi" };
static_assert(ARRAY_SIZE(sensorLabel) == SENSOR_NUM, "array size of sensorLabel does not match SENSOR_NUM");

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

// Function that gets the Firebase JSON label
void getFireBaseJsonLabel(char *label, int max_len, int loc_idx, int sensor_idx) {
  snprintf(label, max_len, "values/[%d]/[0]", SENSOR_NUM * loc_idx + sensor_idx + 1);
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
String locationLabel[] = { "Living", "Bed", "Study"};
static_assert(ARRAY_SIZE(locationLabel) == SENSOR_LOC_NUM, "array size of locationLabel does not match SENSOR_LOC_NUM");

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
  prevSensorMillis = 0;
  prevDbgPrintMillis = 0;
  prevGSheetMillis = millis();
}

void loop() {
  esp_task_wdt_reset();
  // Check sensor data every 30s
  if (millis() - prevSensorMillis >= 30000) {
    prevSensorMillis = millis();
    miThermometer.resetData();
    unsigned found = miThermometer.getData(scanTime);
    for (int i = 0; i < miThermometer.data.size(); i++) {
      if (miThermometer.data[i].valid) {
        Serial.printf("i = %d: %10s, ", i, locationLabel[i].c_str());
        Serial.printf("%s: %.2f, ", sensorLabel[SENSOR_TEMP].c_str(), miThermometer.data[i].temperature / 100.0);
        Temperature = miThermometer.data[i].temperature / 100.0;
        Serial.printf("%s: %.2f, ", sensorLabel[SENSOR_HUMIDITY].c_str(), miThermometer.data[i].humidity / 100.0);
        Humidity = miThermometer.data[i].humidity / 100.0;
        Serial.printf("%s: %.3f, ", sensorLabel[SENSOR_BATTERY_VOLTAGE].c_str(), miThermometer.data[i].batt_voltage / 1000.0);
        Voltage = miThermometer.data[i].batt_voltage / 1000.0;
        char sbuff[8];
        dtostrf(Voltage, 4, 3, sbuff);
        Serial.printf("%s: %3d, ", sensorLabel[SENSOR_BATTERY_LEVEL].c_str(), miThermometer.data[i].batt_level);
        Percent = miThermometer.data[i].batt_level;
        Serial.printf("%s: %d\n", sensorLabel[SENSOR_RSSI].c_str(), miThermometer.data[i].rssi);
        BLErssi = miThermometer.data[i].rssi;
        sensorData[i].data[SENSOR_TEMP] = String(Temperature);
        sensorData[i].data[SENSOR_TEMP] = sensorData[i].data[SENSOR_TEMP].substring(0, 5);
        sensorData[i].data[SENSOR_HUMIDITY] = String(Humidity);
        sensorData[i].data[SENSOR_HUMIDITY] = sensorData[i].data[SENSOR_HUMIDITY].substring(0, 5);
        sensorData[i].data[SENSOR_BATTERY_VOLTAGE] = String(sbuff);
        sensorData[i].data[SENSOR_BATTERY_VOLTAGE] = sensorData[i].data[SENSOR_BATTERY_VOLTAGE].substring(0, 5);
        sensorData[i].data[SENSOR_BATTERY_LEVEL] = String(round(Percent));
        sensorData[i].data[SENSOR_BATTERY_LEVEL] = sensorData[i].data[SENSOR_BATTERY_LEVEL].substring(0, 3);
        sensorData[i].data[SENSOR_RSSI] = String(BLErssi);
        if (BLErssi > -99) {
          sensorData[i].data[SENSOR_RSSI] = sensorData[i].data[SENSOR_RSSI].substring(0, 3);
        } else {
          sensorData[i].data[SENSOR_RSSI] = sensorData[i].data[SENSOR_RSSI].substring(0, 4);
        }
        data_ready = true;
      }
    }
    Serial.println();
    miThermometer.clearScanResults();
  }


  if (millis() - prevGSheetMillis >= 200) {
    prevGSheetMillis = millis();
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
          char _sensorLabel[32];
          char _fireBaseJsonLabel[32];
          Serial.println("GetSucess, A1 <> Time");
          valueRange.add("majorDimension", "COLUMNS");
          valueRange.set("values/[0]/[0]", "Time");
          for (int loc_idx = 0; loc_idx < SENSOR_LOC_NUM; loc_idx++) {
            for (int sensor_idx = 0; sensor_idx < SENSOR_NUM; sensor_idx++) {
              getFireBaseJsonLabel(_fireBaseJsonLabel, sizeof(_fireBaseJsonLabel), loc_idx, sensor_idx);
              snprintf(_sensorLabel, sizeof(_sensorLabel), "%s-%s", locationLabel[loc_idx].c_str(), sensorLabel[sensor_idx].c_str());
              valueRange.set(_fireBaseJsonLabel, _sensorLabel);
            }
          }
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
        }
      }
      //Prepare the Google sheet values to fit a ROW
      valueRange.add("majorDimension", "COLUMNS");
      valueRange.set("values/[0]/[0]", TimeStamp);
      char label[32];
      for (int loc_idx = 0; loc_idx < SENSOR_LOC_NUM; loc_idx++) {
        for (int sensor_idx = 0; sensor_idx < SENSOR_NUM; sensor_idx++) {
          getFireBaseJsonLabel(label, sizeof(label), loc_idx, sensor_idx);
          valueRange.set(label, sensorData[loc_idx].data[sensor_idx]);
        }
      }
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
  if (millis() - prevDbgPrintMillis >= 300000) {
    // print the heap usage every 5 minutes
    prevDbgPrintMillis = millis();
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
  int txPower = WiFi.getTxPower();
  Serial.println(String("\nConnected to the WiFi network (") + WiFi.SSID() + ")[" + WiFi.RSSI() + "], TX power: " + String(txPower) + " dBm");
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