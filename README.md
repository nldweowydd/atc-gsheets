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
[https://github.com/TCM14/atc-gsheets/blob/main/ESP32-1.ino](https://github.com/TCM14/atc-gsheets/blob/main/ESP32-1.ino)

### Step 4
Setup the ESP32-2 to display data on 0.96" OLED diaply and use Google Sheets API:
Set up Google Sheets API to allow the ESP32 to send data to a specified spreadsheet.
**Arduino IDE codes for ESP32-2:**
[https://github.com/TCM14/atc-gsheets/blob/main/ESP32-2.ino](https://github.com/TCM14/atc-gsheets/blob/main/ESP32-2.ino)
Testing: Verify the data logging process and ensure that the data appears correctly in the spreadsheet.
