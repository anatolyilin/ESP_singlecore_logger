#include <WiFi.h>
#include <TimeLib.h>
#include <Time.h>
#include <WiFiUdp.h>
#include "FS.h"
#include "SD.h"
#include "SPI.h"
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ESP32WebServer.h>
#include <ESPmDNS.h>

#define TIME_MSG_LEN 11
#define TIME_HEADER 'T'
#define TIME_REQUEST 7

#define DHTPIN 15
#define DHTTYPE DHT21

#define OLED_RESET 4

DHT_Unified dht(DHTPIN, DHTTYPE);
Adafruit_SSD1306 display(OLED_RESET);

#define LOGO16_GLCD_HEIGHT 128
#define LOGO16_GLCD_WIDTH 16

// delete maybe
#define XPOS 0
#define YPOS 1
#define DELTAY 2

// NTP settings
static const char ntpServerName[] = "us.pool.ntp.org";
const int timeZone = 1; // Central European Time
const int NTP_PACKET_SIZE = 48; // NTP time is in the first 48 bytes of message
byte packetBuffer[NTP_PACKET_SIZE]; //buffer to hold incoming & outgoing packets

WiFiUDP Udp;
unsigned int localPort = 8888; // local port to listen for UDP packets
time_t getNtpTime();
void sendNTPpacket(IPAddress & address);

int lines_drown = 0;

float temp;
float humid;

int humid_read = 0;
int temp_read = 0;

char filename[] = "/measurements.txt";

char APlist[] = "/AP.txt";
int AP_index = 0;
//char filename[] = "/measurements.txt";

ESP32WebServer server(80);

unsigned long TimerScreen = 0;
unsigned long TimerSensor = 0;
unsigned long TimeUpdateSh = 0;
unsigned long TimeUpdateL = 0;

int screenRefRate = 100;
int sensorRefRate = 2000;
int TimeRefRateSh = 1000;
int TimeRefRateL = 1800000;

int TouchRefRate = 500;
unsigned long TouchUpdate = 0;

bool SDerror = false;
bool WifiError = false;
bool SensorError = false;

bool klok = true;
bool suspended = false;

bool ScreenOn = true;
bool manualmode = false;
bool ondemandScreen = false;
int screenOntime = 5000;

String boottime ="";

uint64_t cardSize;

int touchVal = 0;

void setup() {
  Serial.begin(9600);
  Serial.setDebugOutput(true);
  Serial.println("Starting... ");
  setupScreen();
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.println("Initializing: ...");
  display.display();
  setupSD();
  setupWifi();
  delay(500);
  setupTouch();
  writeScreen("Wifi connected");
  setupSensor();
  setupNTPtime();
  delay(2000);
  setupServer();

  printTime();
  Serial.print(WiFi.localIP());
  delay(500);
  delay(5000);
  TimerScreen = millis();
  TimerSensor = millis();

}
void setupTouch(){
  int tempT = 0;

  for(int i=0; i<20; i++){
    tempT += touchRead(T4);
  }
  touchVal = tempT/20;
}
void setupWifi() {

  char ap[30];
  char ap_pw[30];

  Serial.print("Scan start ... ");
  writeScreen("Scanning...");
  int n = WiFi.scanNetworks();
  Serial.print(n);
  Serial.println(" network(s) found");
  writeScreen("Networks detected...");

  for (int j = 0; j < readSSID_num(); j++) {
    for (int i = 0; i < n; i++) {
      if (readSSID(j).equals(WiFi.SSID(i))) {
        readSSID(j).toCharArray(ap, 30);
        readSSID_PW(j).toCharArray(ap_pw, 30);
        break;
      }
    }

  }
  for (int i = 0; i < n; i++) {
    writeScreen(WiFi.SSID(i));
    Serial.println(WiFi.SSID(i));
  }
  Serial.println();
  writeScreen("Resetting Wifi...");
  WiFi.disconnect(true);
  WiFi.setAutoConnect(false);
  writeScreenIN("Connection to AP: ");
  writeScreenIN(ap);
  WiFi.begin(ap, ap_pw);
  WiFi.printDiag(Serial);
  Serial.print("Connecting");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    writeScreenIN(".");
    Serial.print(".");
  }
  Serial.println();
  writeScreen("Connected, IP: ");
  Serial.print("Connected, IP address: ");
  writeScreenIN(String(WiFi.localIP()));
  Serial.println(WiFi.localIP());


    if (!MDNS.begin("sensor")) {
        writeScreenError("Error setting up mDNS");
        delay(2000);
    }


}
void setupNTPtime() {
  Serial.print("Starting UDP");
  Udp.begin(localPort);
  Serial.print("waiting for sync");
  setSyncProvider(getNtpTime);
  setSyncInterval(300);
  setTime(getNtpTime());
  updateTime();
}
void updateTime() {
  setSyncProvider(getNtpTime);
  writeScreen("Time updated !");
  if (boottime.equals("") && now() > 1515440167 ) {
    // boottime not yet set, perhaps time update failed
    boottime =  addZero(day()) + "-" + addZero(month()) + "-" + addZero(year()) + "  " + addZero(hour()) + ":" + addZero(minute()) + ":" + addZero(second());
  }
  if (hour() < 6 && !manualmode ){
    TurnScreenOff();
  } else if(!manualmode && !ondemandScreen) {
    TurnScreenOn();
  }
}
void printTime() {
  Serial.print(hour());
  Serial.print(":");
  Serial.print(minute());
  Serial.print(":");
  Serial.print(second());
  Serial.print("  - ");
  Serial.print(day());
  Serial.print(" ");
  Serial.print(month());
  Serial.print(" ");
  Serial.print(year());
  Serial.println();
  Serial.println(now());

  writeScreenIN(addZero(hour()) + ":" + addZero(minute()) + ":" + addZero(second()));
}
String addZero(int val) {
  String valr = "";
  if (val < 10) {
    valr = "0" + String(val);
  } else {
    valr = String(val);
  }
  return valr;
}
void SensorSc() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.println(addZero(hour()) + ":" + addZero(minute()) + ":" + addZero(second()));
  display.print("Temperature:  ");
  display.setTextSize(1);
  display.print(temp);
  display.println(" C");
  display.print("Humidity:     ");
  display.setTextSize(1);
  display.print(humid);
  display.println(" %");
  display.display();
}
void klokSc() {
  display.clearDisplay();
  display.setTextSize(3);
  display.setTextColor(WHITE);
  display.setCursor(25, 0);
  display.println(addZero(hour()) + ":" + addZero(minute()));
  display.setTextSize(1);
  display.print(String(temp) + "'C      " + String(humid) + " %");
  display.display();
}
void ReadHumidity() {
  sensors_event_t event;
  dht.humidity().getEvent( & event);

  if (isnan(event.relative_humidity) && humid_read < 15) {
    Serial.println("Error reading temperature!");
    // writeScreenError("Error reading temperature!");
    ReadHumidity();
    humid_read++;
  } else {
    humid = event.relative_humidity;
    humid_read = 0;
  }
}
void ReadTemperature() {
  sensors_event_t event;
  dht.temperature().getEvent( & event);

  if (isnan(event.temperature) && temp_read < 15) {
    Serial.println("Error reading temperature!");
    // writeScreenError("Error reading temperature!");
    ReadTemperature();
    temp_read++;
  } else {
    temp = event.temperature;
    temp_read = 0;
  }
}
void WriteMeasurement() {

  char content[64];
  char cstr[16];
  itoa(now(), cstr, 10);
  strcpy(content, cstr);
  strcat(content, ",");
  char buffer[8];
  int ret = snprintf(buffer, sizeof buffer, "%f", temp);
  strcat(content, buffer);
  strcat(content, ",");
  ret = snprintf(buffer, sizeof buffer, "%f", humid);
  strcat(content, buffer);
  strcat(content, ",");
  String timeread = addZero(day()) + "-" + addZero(month()) + "-" + addZero(year()) + "," + addZero(hour()) + ":" + addZero(minute()) + ":" + addZero(second());
  strcat(content, timeread.c_str());
  strcat(content, "\n");
  appendFile(SD, filename, content);
}
void loop() {

  server.handleClient();

  if (!suspended) {
        int tempval = 0;
        for(int i = 0; i < 4; i++){
          tempval += touchRead(T4);
        }
        tempval = tempval /4;

     if (millis() - TouchUpdate >= TouchRefRate && tempval < (touchVal - 30) ) {
        toggleScreen();
        TouchUpdate = millis();
     }

 if (millis() - TouchUpdate >= screenOntime && ondemandScreen  ) {
        TurnScreenOff();
        TouchUpdate = millis();
     }

    if (millis() - TimerScreen >= screenRefRate && !klok && ScreenOn) {
      SensorSc();
      TimerScreen = millis();
    }
    if (millis() - TimerScreen >= screenRefRate && klok && ScreenOn) {
      klokSc();
      TimerScreen = millis();
    }

    if (millis() - TimeUpdateSh >= TimeRefRateSh && now() < 1515440167) {
      updateTime();
      TimeUpdateSh = millis();
    }
    if (millis() - TimeUpdateL >= TimeRefRateL) {
      updateTime();
      TimeUpdateL = millis();
    }
    if (millis() - TimerSensor >= sensorRefRate) {
      ReadHumidity();
      ReadTemperature();
      WriteMeasurement();
      TimerSensor = millis();
    }

  }
}
time_t getNtpTime() {
  writeScreen("NTP requested ");
  IPAddress ntpServerIP; // NTP server's ip address

  while (Udp.parsePacket() > 0); // discard any previously received packets
  Serial.println("Transmit NTP Request");
  writeScreen("Transmitting NTP");
  // get a random server from the pool
  WiFi.hostByName(ntpServerName, ntpServerIP);
  writeScreen("Sending to:");
  Serial.print(ntpServerName);
  writeScreen(String(ntpServerName));
  Serial.print(": ");
  writeScreen(String(ntpServerIP));
  Serial.println(ntpServerIP);
  sendNTPpacket(ntpServerIP);
  uint32_t beginWait = millis();
  writeScreen("Awaiting reply");
  while (millis() - beginWait < 1500) {
    int size = Udp.parsePacket();
    if (size >= NTP_PACKET_SIZE) {
      writeScreen("NTP time received");
      Serial.println("Receive NTP Response");
      Udp.read(packetBuffer, NTP_PACKET_SIZE); // read packet into the buffer
      unsigned long secsSince1900;
      // convert four bytes starting at location 40 to a long integer
      secsSince1900 = (unsigned long) packetBuffer[40] << 24;
      secsSince1900 |= (unsigned long) packetBuffer[41] << 16;
      secsSince1900 |= (unsigned long) packetBuffer[42] << 8;
      secsSince1900 |= (unsigned long) packetBuffer[43];
      return secsSince1900 - 2208988800UL + timeZone * SECS_PER_HOUR;
    }
  }
  Serial.println("No NTP Response :-(");
  writeScreen("Failed to get time");
  return 0; // return 0 if unable to get the time
}
void sendNTPpacket(IPAddress & address) {
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011; // LI, Version, Mode
  packetBuffer[1] = 0; // Stratum, or type of clock
  packetBuffer[2] = 6; // Polling Interval
  packetBuffer[3] = 0xEC; // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12] = 49;
  packetBuffer[13] = 0x4E;
  packetBuffer[14] = 49;
  packetBuffer[15] = 52;
  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  Udp.beginPacket(address, 123); //NTP requests are to port 123
  Udp.write(packetBuffer, NTP_PACKET_SIZE);
  Udp.endPacket();
}
void listDir(fs::FS & fs,
  const char * dirname, uint8_t levels) {
  Serial.printf("Listing directory: %s\n", dirname);

  File root = fs.open(dirname);
  if (!root) {
    Serial.println("Failed to open directory");
    return;
  }
  if (!root.isDirectory()) {
    Serial.println("Not a directory");
    return;
  }

  File file = root.openNextFile();
  while (file) {
    if (file.isDirectory()) {
      Serial.print("  DIR : ");
      Serial.println(file.name());
      if (levels) {
        listDir(fs, file.name(), levels - 1);
      }
    } else {
      Serial.print("  FILE: ");
      Serial.print(file.name());
      Serial.print("  SIZE: ");
      Serial.println(file.size());
    }
    file = root.openNextFile();
  }
}
void createDir(fs::FS & fs,
  const char * path) {
  Serial.printf("Creating Dir: %s\n", path);
  if (fs.mkdir(path)) {
    Serial.println("Dir created");
  } else {
    Serial.println("mkdir failed");
  }
}
void removeDir(fs::FS & fs,
  const char * path) {
  Serial.printf("Removing Dir: %s\n", path);
  if (fs.rmdir(path)) {
    Serial.println("Dir removed");
  } else {
    Serial.println("rmdir failed");
  }
}
void readFile(fs::FS & fs,
  const char * path) {

  Serial.printf("Reading file: %s\n", path);

  File file = fs.open(path);
  if (!file) {
    Serial.println("Failed to open file for reading");
    return;
  }

  Serial.print("Read from file: ");
  while (file.available()) {
    Serial.write(file.read());
  }
  file.close();
}
String readLine(const char * path, int line_num) {
  int index = 0;
  File myFile = SD.open(path);

  if (!myFile) {
    Serial.println("Failed to open file for reading " + String(path));
    writeScreenError("Failed opening " + String(path));
    myFile.close();
    return "";
  }

  while (myFile.available()) {
    String list = myFile.readStringUntil('\n');
    if (index == line_num) {
      myFile.close();
      return list;
    }
    index++;
  }
  myFile.close();
  return "";

}
int file_line_num(const char * path) {
  int i = 0;
  while (readLine(path, i) != "") {
    i++;
  }
  return i;
}
int readSSID_num() {
  return file_line_num(APlist) / 2;
}
String readSSID(int ap_req_index) {
  return readLine(APlist, ap_req_index * 2);
}
String readSSID_PW(int ap_req_index) {
  return readLine(APlist, ap_req_index * 2 + 1);
}
void writeFile(fs::FS & fs,
  const char * path,
    const char * message) {
  Serial.printf("Writing file: %s\n", path);
  writeScreen("Writing file " + String(path));
  File file = fs.open(path, FILE_WRITE);
  if (!file) {
    Serial.println("Failed to open file for writing");
    writeScreen("Failed to open " + String(path));
    return;
  }
  if (file.print(message)) {
    Serial.println("File written");
  } else {
    Serial.println("Write failed");
    writeScreen("Writing FAILED");
  }
  file.close();
}
void appendFile(fs::FS & fs,
  const char * path,
    const char * message) {
  //Serial.printf("Appending to file: %s\n", path);
  File file = fs.open(path, FILE_APPEND);
  if (!file) {
    Serial.println("Failed to open file for appending");
    writeScreenError("Reading " + String(path) + " FAILED");
    SDerror = true;
    return;
  }
  if (file.print(message)) {
    //Serial.println("Message appended");
  } else {
    Serial.println("Append failed");
    writeScreenError("Append FAILED");
    SDerror = true;
  }
  file.close();
}
void renameFile(fs::FS & fs,
  const char * path1,
    const char * path2) {
  Serial.printf("Renaming file %s to %s\n", path1, path2);
  if (fs.rename(path1, path2)) {
    Serial.println("File renamed");
  } else {
    Serial.println("Rename failed");
  }
}
void deleteFile(fs::FS & fs,
  const char * path) {
  Serial.printf("Deleting file: %s\n", path);
  if (fs.remove(path)) {
    Serial.println("File deleted");
  } else {
    Serial.println("Delete failed");
  }
}
void testFileIO(fs::FS & fs,
  const char * path) {
  File file = fs.open(path);
  static uint8_t buf[512];
  size_t len = 0;
  uint32_t start = millis();
  uint32_t end = start;
  if (file) {
    len = file.size();
    size_t flen = len;
    start = millis();
    while (len) {
      size_t toRead = len;
      if (toRead > 512) {
        toRead = 512;
      }
      file.read(buf, toRead);
      len -= toRead;
    }
    end = millis() - start;
    Serial.printf("%u bytes read for %u ms\n", flen, end);
    file.close();
  } else {
    Serial.println("Failed to open file for reading");
  }

  file = fs.open(path, FILE_WRITE);
  if (!file) {
    Serial.println("Failed to open file for writing");
    return;
  }

  size_t i;
  start = millis();
  for (i = 0; i < 2048; i++) {
    file.write(buf, 512);
  }
  end = millis() - start;
  Serial.printf("%u bytes written for %u ms\n", 2048 * 512, end);
  file.close();
}
void setupSensor() {
  writeScreen("Starting sensor");
  dht.begin();
  sensor_t sensor;
  dht.temperature().getSensor( & sensor);
  dht.humidity().getSensor( & sensor);
  writeScreen("Initialized DHT");
  delay(500);
}
void setupSD() {
  if (!SD.begin()) {
    writeScreenError("Card Mount failure");
    Serial.println("Card Mount Failed");
    delay(5000);
    SDerror = true;
    return;
  }
  uint8_t cardType = SD.cardType();
  if (cardType == CARD_NONE) {
    writeScreenError("No SD found");
    Serial.println("No SD card attached");
    delay(5000);
    SDerror = true;
    return;
  }

  Serial.print("SD Card Type: ");
  writeScreen("SD Card type: ");
  if (cardType == CARD_MMC) {
    writeScreen("MMC");
    Serial.println("MMC");
  } else if (cardType == CARD_SD) {
    writeScreen("SDSC");
    Serial.println("SDSC");
  } else if (cardType == CARD_SDHC) {
    writeScreen("SDSC");
    Serial.println("SDHC");
  } else {
    writeScreen("unknown");
    Serial.println("UNKNOWN");
  }

  cardSize = SD.cardSize() / (1024 * 1024);
  Serial.printf("SD Card Size: %lluMB\n", cardSize);
  writeScreenIN("SD size: ");
  char buffer[64];
  int ret = snprintf(buffer, sizeof buffer, "%d", cardSize);
  writeScreenIN((buffer));
  writeScreenIN(" MB");
  // writeFile(SD, filename, "\n");
}
int fileSize(const char * path) {
  File myFile = SD.open(path);

  if (!myFile) {
    Serial.println("Failed to open file for reading " + String(path));
    writeScreenError("Failed opening " + String(path));
    myFile.close();
    return 0;
  }

  if (myFile.available()) {
    uint64_t val = myFile.size() / (1024);
    myFile.close();
    return val;
  }

  return 0;

}
void setupScreen() {
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.display();
  delay(2000);
  writeScreen("Initializing...");

}
void writeScreen(String str) {
  if (lines_drown < 4) {
    display.setTextColor(WHITE);
    display.println(str);
    display.display();
    lines_drown++;
  } else {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(0, 0);
    display.println(str);
    display.display();
    lines_drown = 1;
  }
}
void writeScreenError(String str) {
  testError();
  if (lines_drown < 4) {
    display.setTextColor(BLACK, WHITE);
    display.println(str);
    display.display();
    lines_drown++;
  } else {
    display.setTextColor(BLACK, WHITE);
    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.println(str);
    display.display();
    lines_drown = 1;
  }
}
void writeScreenIN(String str) {
  display.print(str);
  display.display();

}
void testScreen() {
  for (int i = 0; i < 20; i++) {
    writeScreen("test " + String(i));
    delay(2000);
  }
  for (int i = 0; i < 20; i++) {
    writeScreenIN(" " + String(i));
    delay(2000);
  }
}
void toggleScreen(){
  if (ScreenOn) {
    TurnScreenOff();
  } else {
    TurnScreenOn();
  }
}

void TurnScreenOff(){
  display.ssd1306_command(SSD1306_DISPLAYOFF);
  ScreenOn = false;
}

void TurnScreenOn(){
  display.ssd1306_command(SSD1306_DISPLAYON);
  ScreenOn = true;
}


void setupServer() {
  server.on("/", rootPage);
  server.on("/temp", tempPage);
  server.on("/humid", humidPage);
  server.on("/getMeasurements", getMeasurements);
  server.on("/deleteMeasurements", deleteMeasurements);
    server.on("/screenoff", []() {
  suspended = true;
  display.ssd1306_command(SSD1306_DISPLAYOFF);
         server.sendHeader("Location", String("/"), true);
          server.send ( 302, "text/plain", "");
          delay(200);
          suspended = false;
      });

     server.on("/screenon", []() {
        suspended = true;
         display.ssd1306_command(SSD1306_DISPLAYON);
         server.sendHeader("Location", String("/"), true);
          server.send ( 302, "text/plain", "");
         delay(200);
          suspended = false;
      });
  server.on("/status", []() {
    int cardSize = SD.cardSize() / (1024 * 1024);
    server.send(200, "text/html", "<html>SD size: " + String(cardSize) + " MB<br> Measurements used: " + (fileSize("/measurements.txt") / 1024) + " MB " + (file_line_num("/measurements.txt")) + " measurements </html>");
  });
  server.on("/updatetime", []() {
    updateTime();
    server.sendHeader("Location", String("/"), true);
    server.send ( 302, "text/plain", "");

});
  server.on("/updates", []() {
    suspended = true;
    screenRefRate = (server.arg("screenRefRate")).toInt();
    sensorRefRate = (server.arg("sensorRefRate")).toInt();
    TimeRefRateSh = (server.arg("TimeRefRateSh")).toInt();
    TimeRefRateL = (server.arg("TimeRefRateL")).toInt();
touchVal = (server.arg("touchVal")).toInt();
ondemandScreen = (server.arg("ondemandScreen")).toInt();
screenOntime = (server.arg("screenOntime")).toInt();
    int dim = (server.arg("dim")).toInt();
    klok = (server.arg("klok")).toInt();
    display.dim(dim);
    manualmode = (server.arg("manualmode")).toInt();
    String newfilename = (server.arg("file"));
    strcpy(filename, newfilename.c_str());

    writeScreen("Config Updated..");
    Serial.println("config Updated");
    server.sendHeader("Location", String("/"), true);
    server.send ( 302, "text/plain", "");
    suspended = false;
  });
  server.on("/config", []() {
    // server.send(200, "text/html", "<html><form action='/updates' method='post'>First name: <input type='text' name='D' value='John'><br><input type='submit' value='Submit'></form></html>");
    // server.send(200, "text/html", "<html><form action='/updates' method='post'><h3>Update intervals in ms</h3>Klok: <input type='text' name='klok' value='0'><br>Dim: <input type='text' name='dim' value='0'><br>Screen Refresh rate: <input type='text' name='screenRefRate' value=" + String(screenRefRate) + "><br>Sensor Refresh rate: <input type='text' name='sensorRefRate' value=" + String(sensorRefRate) + "><br>Time update interval (fail): <input type='text' name='TimeRefRateSh' value=" + String(TimeRefRateSh) + "><br>Time update interval (normal): <input type='text' name='TimeRefRateL' value=" + String(TimeRefRateL) + "><br><input type='submit' value='Submit'></form></html>");
String page = "<!doctype html>"
    "<html lang=\"en\">"
    "<head>"
    "  <meta charset=\"utf-8\">"
    "  <title>Sensor Config</title>"
    "</head>"
    "<body>"
    "    <form action='/updates' method='post'>"
    "        Klok mode: <input type=\"text\" name=\"klok\" value=\"1\"><br>"
    "        Changes the screen layout from normal (1) to debug (0). <br>"
    "        Dim mode: <input type=\"text\" name=\"dim\" value=\"0\"><br>"
    "        Setting dim to 1, will decrease screen brightness. <br>"
    "        Screen Refresh rate: <input type=\"text\" name=\"screenRefRate\" value=" + String(screenRefRate) + "> ms<br>"
    "        The default screen refresh rate is 100 ms. <br>"
    "        Night off - Day on mode: <input type=\"text\" name=\"manualmode\" value=" + String(manualmode) + "> <br>"
    "        Calibration value TouchSensor: <input type=\"text\" name=\"touchVal\" value=" + String(touchVal) + "> <br>"
    "        Sensor Refresh rate: <input type=\"text\" name=\"sensorRefRate\" value=" + String(sensorRefRate) + "> ms<br>"
    "        The default sensor measurement rate is 2s = 2000ms. <br>"
        "        OnDemand Screen mode: <input type=\"text\" name=\"ondemandScreen\" value=" + String(ondemandScreen) + "> <br>"
         "        OnDemand Screen ON time: <input type=\"text\" name=\"screenOntime\" value=" + String(screenOntime) + "> <br>"
    "        Time update interval (fail): <input type=\"text\" name=\"TimeRefRateSh\" value=" + String(TimeRefRateSh) + ">ms<br>"
    "        Time update interval (normal): <input type=\"text\" name=\"TimeRefRateL\" value=" + String(TimeRefRateL) + ">ms<br>"
    "        Fail interval is the interval between clock update attempts in case the clock is not correctly initiliated on boot (e.g. boot without internet connection). Normal interval is the interval for clock updates in non-failed mode.<br>"
    "        Filename: <input type=\"text\" name=\"file\" value="+String(filename)+"><br>"
    "        <input type='submit' value='Submit'>"
    "    </form>"
    "    <form method=\"get\" action=\"/updatetime\">"
    "        <button type=\"submit\">Update Time</button>"
    "    </form>"
    "    <form method=\"get\" action=\"/getMeasurements\">"
    "        <button type=\"submit\">Get Measurements</button>"
    "    </form>"
    "    <form method=\"get\" action=\"/deleteMeasurements\">"
    "        <button type=\"submit\">Delete Measurements</button>"
    "    </form>"
    "</body>"
    "</html>";
    server.send(200, "text/html", page);
  });
  server.begin();
}
void rootPage() {

suspended = true ;
  File file = SD.open(filename);
  size_t len = 0;
  if (file) {
    len = file.size();
  }
  file.close();

char buffer[64];
int test = int(cardSize);
float size2 = test/1.0;
int ret = snprintf(buffer, sizeof buffer, "%d", cardSize);
suspended = false;
  String html = "<html lang=\"en\">"
"<head>"
"  <meta charset=\"utf-8\">"
"  <title>Sensor Config</title>"
"</head>"
"<body>"
"    <h1>"+String(hour()) + ":" + addZero(minute())+"</h1>"
"    <h4>"+String(day()) + " - " + String(month())+ " - " + String(year())+"</h4>"
"    <h3> "+humid + " %  <br> "
" "+   temp + " \'C </h3>"
"Up since " + boottime + "<br>"
"Screen on: " + ScreenOn + "<br>"
"Touch Calibration value " + touchVal +"<br>"
"Current touch value (onpage load) " +  String(touchRead(T4)) +"<br>"
"    SD cardsize: "+String(size2) +" MB <br>"
"    Measurement file size: "+len+" B <br>"
"    Measurement file size: "+len/1024 +" kB <br>"
"    Measurement file size: "+len/(1024*1024)+" MB <br>"
"    <form method=\"get\" action=\"/getMeasurements\">"
"        <button type=\"submit\">Get Measurements</button>"
"    </form>"
 "<a href=\"/config\">config</a><br>"
 "<a href=\"/screenoff\">screen OFF</a><br>"
"<a href=\"/screenon\">screen ON</a><br>"
 "Free HEAP: "+ ESP.getFreeHeap() + " "
"</body>"
"</html>";

  server.setContentLength(html.length());
  server.send(200, "text/html", html);
}

void deleteMeasurements() {
  suspended = true;
  deleteFile(SD,  filename) ;
  delay(200);
  suspended = false;
  server.sendHeader("Location", String("/"), true);
  server.send ( 302, "text/plain", "");
}
void tempPage() {
  String html = String(temp);
  server.setContentLength(html.length());
  server.send(200, "text/html", html);
}
void humidPage() {
  String html = String(humid);
  server.setContentLength(html.length());
  server.send(200, "text/html", html);
}
void getMeasurements(){

  String filetitle = "measurement"+  String(year())+addZero(month())+addZero(day())+addZero(hour())+addZero(minute()) +".txt";
  downloadFile(filename, filetitle);
  server.sendHeader("Location", String("/"), true);
  server.send ( 302, "text/plain", "");
}
void downloadFile(const char * path, String filetitle) {

  suspended = true;
  display.clearDisplay();
  writeScreenError("Transfering file " + String(path));
  writeScreenError("Execution halted... ");
  File file = SD.open(path);
  static uint8_t buf[512];
  size_t len = 0;
  if (file) {
    delay(200);
    len = file.size();
    writeScreenError("filesize " + String(len));
    size_t flen = len;
    server.setContentLength(flen);
    server.sendHeader("Content-Type", "application/octet-stream");
    server.sendHeader("Content-Disposition", "attachment; filename="+ filetitle);
    bool first = true;

    while (len) {
      size_t toRead = len;
      if (toRead > 512) {
        toRead = 512;
      }
      file.read(buf, toRead);
      String str = (char * ) buf;
      if (first) {
        server.send(200, "text/plain", str);
        first = false;
      } else {
        server.sendContent(str);
      }

      len -= toRead;
    }
    file.close();
  } else {
    writeScreenError("Transfer failed...");
  }

  delay(200);
  suspended = false;

}
void testError() {
  display.invertDisplay(true);
  delay(1000);
  display.invertDisplay(false);
  delay(1000);
  display.clearDisplay();
}
