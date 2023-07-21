//#############################################################################
//-------- AUTHOR  ------------------------------------------------------------
//#############################################################################
//#### Gardening the Cybernetic Meadow by Matthew Halpenny (2023)           ###
//#### Modified from OWM Weather Display                                    ###
//#### https://github.com/Xinyuan-LilyGO/LilyGo-EPD-4-7-OWM-Weather-Display ###
//#############################################################################

#include <Arduino.h>            // Arduino Basics 
#include <esp_task_wdt.h>       // ESP Task Watchdog
#include "freertos/FreeRTOS.h"  // Real-Time [Kernel] OS
#include "freertos/task.h"      // FreeRTOS Task Watchdog
#include "esp_adc_cal.h"        // Analog to Digital Converter calibration
#include <time.h>               // Built In

#include "epd_driver.h"         // https://github.com/Xinyuan-LilyGO/LilyGo-EPD47
#include <pcf8563.h>            // Required for NXP PCF8563 RTC chip

#include <ArduinoJson.h>        // Better JSON handler - https://github.com/bblanchon/ArduinoJson
#include <HTTPClient.h>         // Arduino HTTP Client
#include <WiFi.h>               // Arduino Wifi
#include <SPI.h>                // SPI Communication

#include "credentials.h"        // WiFi & server credentials
#include "forecast_record.h"    // Struct for all data (to be changed)
#include "lang.h"               // Defines displayed variables

#define SCREEN_WIDTH   EPD_WIDTH
#define SCREEN_HEIGHT  EPD_HEIGHT

//#############################################################################
//-------- VERSION  -----------------------------------------------------------
//#############################################################################

String version = "2.5 / 4.7in";  // Programme version

//#############################################################################
//-------- EPD VARIABLES  -----------------------------------------------------
//#############################################################################

enum alignment {LEFT, RIGHT, CENTER};
#define White         0xFF
#define LightGrey     0xBB
#define Grey          0x88
#define DarkGrey      0x44
#define Black         0x00

#define autoscale_on  true
#define autoscale_off false
#define barchart_on   true
#define barchart_off  false

boolean LargeIcon   = true;
boolean SmallIcon   = false;
#define Large  20           // For icon drawing
#define Small  8            // For icon drawing
String  Time_str = "--:--:--";
String  Date_str = "-- --- ----";
int     wifi_signal, CurrentHour = 0, CurrentMin = 0, CurrentSec = 0, EventCnt = 0, vref = 1100;

//#############################################################################
//-------- TEXT VARIABLES & OBJECTS  ---------------------------------------
//#############################################################################

#define max_readings 24 // Limited to 3-days here, but could go to 5-days = 40  

Forecast_record_type  WxConditions[1];
Forecast_record_type  WxForecast[max_readings];

float pressure_readings[max_readings]    = {0};
float temperature_readings[max_readings] = {0};
float humidity_readings[max_readings]    = {0};
float rain_readings[max_readings]        = {0};
float snow_readings[max_readings]        = {0};

//fonts
#include "opensans8b.h"
#include "opensans10b.h"
#include "opensans12b.h"
#include "opensans18b.h"
#include "opensans24b.h"

GFXfont  currentFont;
uint8_t *framebuffer;

//#############################################################################
//-------- SLEEP  -------------------------------------------------------------
//#############################################################################

long SleepDuration   = 60; // Sleep time in minutes, aligned to the nearest minute boundary, so if 30 will always update at 00 or 30 past the hour
int  WakeupHour      = 8;  // Don't wakeup until after 07:00 to save battery power
int  SleepHour       = 23; // Sleep after 23:00 to save battery power
long StartTime       = 0;
long SleepTimer      = 0;
long Delta           = 30; // ESP32 rtc speed compensation, prevents display at xx:59:yy and then xx:00:yy (one minute later) to save power

void BeginSleep() {
  epd_poweroff_all();
  UpdateLocalTime();
  SleepTimer = (SleepDuration * 60 - ((CurrentMin % SleepDuration) * 60 + CurrentSec)) + Delta; //Some ESP32 have a RTC that is too fast to maintain accurate time, so add an offset
  esp_sleep_enable_timer_wakeup(SleepTimer * 1000000LL); // in Secs, 1000000LL converts to Secs as unit = 1uSec
  Serial.println("Awake for : " + String((millis() - StartTime) / 1000.0, 3) + "-secs");
  Serial.println("Entering " + String(SleepTimer) + " (secs) of sleep time");
  Serial.println("Starting deep-sleep period...");
  esp_deep_sleep_start();  // Sleep for e.g. 30 minutes
}

//#############################################################################
//-------- TIME & RTC  --------------------------------------------------------
//#############################################################################

boolean SetupTime() {
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer, "time.nist.gov"); //(gmtOffset_sec, daylightOffset_sec, ntpServer)
  setenv("TZ", Timezone, 1);  //setenv()adds the "TZ" variable to the environment with a value TimeZone, only used if set to 1, 0 means no change
  tzset(); // Set the TZ environment variable
  delay(100);
  return UpdateLocalTime();
}

boolean UpdateLocalTime() {
  struct tm timeinfo;
  char   time_output[30], day_output[30], update_time[30];
  while (!getLocalTime(&timeinfo, 5000)) { // Wait for 5-sec for time to synchronise
    Serial.println("Failed to obtain time");
    return false;
  }
  CurrentHour = timeinfo.tm_hour;
  CurrentMin  = timeinfo.tm_min;
  CurrentSec  = timeinfo.tm_sec;
  //See http://www.cplusplus.com/reference/ctime/strftime/
  Serial.println(&timeinfo, "%a %b %d %Y   %H:%M:%S");      // Displays: Saturday, June 24 2017 14:05:49
  if (Units == "M") {
    sprintf(day_output, "%s, %02u %s %04u", weekday_D[timeinfo.tm_wday], timeinfo.tm_mday, month_M[timeinfo.tm_mon], (timeinfo.tm_year) + 1900);
    strftime(update_time, sizeof(update_time), "%H:%M:%S", &timeinfo);  // Creates: '@ 14:05:49'   and change from 30 to 8 <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
    sprintf(time_output, "%s", update_time);
  }
  else
  {
    strftime(day_output, sizeof(day_output), "%a %b-%d-%Y", &timeinfo); // Creates  'Sat May-31-2019'
    strftime(update_time, sizeof(update_time), "%r", &timeinfo);        // Creates: '@ 02:05:49pm'
    sprintf(time_output, "%s", update_time);
  }
  Date_str = day_output;
  Time_str = time_output;
  return true;
}

//#############################################################################
//-------- WIFI  --------------------------------------------------------------
//#############################################################################

uint8_t StartWiFi() {
  Serial.println("\r\nConnecting to: " + String(ssid));
  IPAddress dns(8, 8, 8, 8); // Use Google DNS
  WiFi.disconnect();
  WiFi.mode(WIFI_STA); // switch off AP
  WiFi.setAutoConnect(true);
  WiFi.setAutoReconnect(true);
  WiFi.begin(ssid, password);
  if (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.printf("STA: Failed!\n");
    WiFi.disconnect(false);
    delay(500);
    WiFi.begin(ssid, password);
  }
  if (WiFi.status() == WL_CONNECTED) {
    wifi_signal = WiFi.RSSI(); // Get Wifi Signal strength now, because the WiFi will be turned off to save power!
    Serial.println("WiFi connected at: " + WiFi.localIP().toString());
  }
  else Serial.println("WiFi connection *** FAILED ***");
  return WiFi.status();
}

void StopWiFi() {
  WiFi.disconnect();
  WiFi.mode(WIFI_OFF);
  Serial.println("WiFi switched Off");
}

//#############################################################################
//-------- INITIALIZATION  ----------------------------------------------------
//#############################################################################

void InitialiseSystem() {
  StartTime = millis();
  Serial.begin(115200);
  while (!Serial);
  Serial.println(String(__FILE__) + "\nStarting...");
  epd_init();
  framebuffer = (uint8_t *)ps_calloc(sizeof(uint8_t), EPD_WIDTH * EPD_HEIGHT / 2);
  if (!framebuffer) Serial.println("Memory alloc failed!");
  memset(framebuffer, 0xFF, EPD_WIDTH * EPD_HEIGHT / 2);
}

void setup() {
  InitialiseSystem();
  if (StartWiFi() == WL_CONNECTED && SetupTime() == true) {
    bool WakeUp = false;                
    if (WakeupHour > SleepHour)
      WakeUp = (CurrentHour >= WakeupHour || CurrentHour <= SleepHour); 
    else                             
      WakeUp = (CurrentHour >= WakeupHour && CurrentHour <= SleepHour);                              
    if (WakeUp) {
      byte Attempts = 1;
      bool RxWeather  = false;
      bool RxForecast = false;
      WiFiClient client;   // wifi client object
      while ((RxWeather == false || RxForecast == false) && Attempts <= 2) { // Try up-to 2 time for Weather and Forecast data
        if (RxWeather  == false) RxWeather  = obtainWeatherData(client, "weather");
        if (RxForecast == false) RxForecast = obtainWeatherData(client, "forecast");
        Attempts++;
      }
      Serial.println("Received all weather data...");
      if (RxWeather && RxForecast) { // Only if received both Weather or Forecast proceed
        StopWiFi();         // Reduces power consumption
        epd_poweron();      // Switch on EPD display
        epd_clear();        // Clear the screen
        DisplayWeather();   // Display the weather data
        edp_update();       // Update the display to show the information
        epd_poweroff_all(); // Switch off all power to EPD
      }
    }
  }
  BeginSleep();
}

//#############################################################################
//-------- LOOP  --------------------------------------------------------------
//#############################################################################

void loop() {
  // Nothing to do here
}

//#############################################################################
//-------- CLIENT <-> SERVER <-> DATA  ----------------------------------------
//#############################################################################

bool obtainWeatherData(WiFiClient & client, const String & RequestType) {
  const String units = (Units == "M" ? "metric" : "imperial");
  client.stop(); // close connection before sending a new request
  HTTPClient http;
  String uri = "/data/2.5/" + RequestType + "?q=" + City + "," + Country + "&APPID=" + apikey + "&mode=json&units=" + units + "&lang=" + Language;
  if (RequestType != "weather")
  {
    uri += "&cnt=" + String(max_readings);
  }
  http.begin(client, server, 80, uri); //http.begin(uri,test_root_ca); //HTTPS example connection
  int httpCode = http.GET();
  if (httpCode == HTTP_CODE_OK) {
    if (!DecodeWeather(http.getStream(), RequestType)) return false;
    client.stop();
    http.end();
    return true;
  }
  else
  {
    Serial.printf("connection failed, error: %s", http.errorToString(httpCode).c_str());
    client.stop();
    http.end();
    return false;
  }
  http.end();
  return true;
}

//#############################################################################
//-------- JSON / DATA PROCESSING ---------------------------------------------
//#############################################################################

void Convert_Readings_to_Imperial() { // Only the first 3-hours are used
  WxConditions[0].Pressure = hPa_to_inHg(WxConditions[0].Pressure);
  WxForecast[0].Rainfall   = mm_to_inches(WxForecast[0].Rainfall);
  WxForecast[0].Snowfall   = mm_to_inches(WxForecast[0].Snowfall);
}

bool DecodeWeather(WiFiClient& json, String Type) {
  Serial.print(F("\nCreating object...and "));
  DynamicJsonDocument doc(64 * 1024);                      // allocate the JsonDocument
  DeserializationError error = deserializeJson(doc, json); // Deserialize the JSON document
  if (error) {                                             // Test if parsing succeeds.
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.c_str());
    return false;
  }
  // convert it to a JsonObject
  JsonObject root = doc.as<JsonObject>();
  Serial.println(" Decoding " + Type + " data");
  if (Type == "weather") {
    // All Serial.println statements are for diagnostic purposes and some are not required, remove if not needed with //
    //WxConditions[0].lon         = root["coord"]["lon"].as<float>();              Serial.println(" Lon: " + String(WxConditions[0].lon));
    //WxConditions[0].lat         = root["coord"]["lat"].as<float>();              Serial.println(" Lat: " + String(WxConditions[0].lat));
    WxConditions[0].Main0       = root["weather"][0]["main"].as<char*>();        Serial.println("Main: " + String(WxConditions[0].Main0));
    WxConditions[0].Forecast0   = root["weather"][0]["description"].as<char*>(); Serial.println("For0: " + String(WxConditions[0].Forecast0));
    //WxConditions[0].Forecast1   = root["weather"][1]["description"].as<char*>(); Serial.println("For1: " + String(WxConditions[0].Forecast1));
    //WxConditions[0].Forecast2   = root["weather"][2]["description"].as<char*>(); Serial.println("For2: " + String(WxConditions[0].Forecast2));
    WxConditions[0].Icon        = root["weather"][0]["icon"].as<char*>();        Serial.println("Icon: " + String(WxConditions[0].Icon));
    WxConditions[0].Temperature = root["main"]["temp"].as<float>();              Serial.println("Temp: " + String(WxConditions[0].Temperature));
    WxConditions[0].Pressure    = root["main"]["pressure"].as<float>();          Serial.println("Pres: " + String(WxConditions[0].Pressure));
    WxConditions[0].Humidity    = root["main"]["humidity"].as<float>();          Serial.println("Humi: " + String(WxConditions[0].Humidity));
    WxConditions[0].Low         = root["main"]["temp_min"].as<float>();          Serial.println("TLow: " + String(WxConditions[0].Low));
    WxConditions[0].High        = root["main"]["temp_max"].as<float>();          Serial.println("THig: " + String(WxConditions[0].High));
    WxConditions[0].Windspeed   = root["wind"]["speed"].as<float>();             Serial.println("WSpd: " + String(WxConditions[0].Windspeed));
    WxConditions[0].Winddir     = root["wind"]["deg"].as<float>();               Serial.println("WDir: " + String(WxConditions[0].Winddir));
    WxConditions[0].Cloudcover  = root["clouds"]["all"].as<int>();               Serial.println("CCov: " + String(WxConditions[0].Cloudcover)); // in % of cloud cover
    WxConditions[0].Visibility  = root["visibility"].as<int>();                  Serial.println("Visi: " + String(WxConditions[0].Visibility)); // in metres
    WxConditions[0].Rainfall    = root["rain"]["1h"].as<float>();                Serial.println("Rain: " + String(WxConditions[0].Rainfall));
    WxConditions[0].Snowfall    = root["snow"]["1h"].as<float>();                Serial.println("Snow: " + String(WxConditions[0].Snowfall));
    //WxConditions[0].Country     = root["sys"]["country"].as<char*>();            Serial.println("Ctry: " + String(WxConditions[0].Country));
    WxConditions[0].Sunrise     = root["sys"]["sunrise"].as<int>();              Serial.println("SRis: " + String(WxConditions[0].Sunrise));
    WxConditions[0].Sunset      = root["sys"]["sunset"].as<int>();               Serial.println("SSet: " + String(WxConditions[0].Sunset));
    WxConditions[0].Timezone    = root["timezone"].as<int>();                    Serial.println("TZon: " + String(WxConditions[0].Timezone));
  }
  if (Type == "forecast") {
    //Serial.println(json);
    Serial.print(F("\nReceiving Forecast period - ")); //------------------------------------------------
    JsonArray list                  = root["list"];
    for (byte r = 0; r < max_readings; r++) {
      Serial.println("\nPeriod-" + String(r) + "--------------");
      WxForecast[r].Dt                = list[r]["dt"].as<int>();
      WxForecast[r].Temperature       = list[r]["main"]["temp"].as<float>();              Serial.println("Temp: " + String(WxForecast[r].Temperature));
      WxForecast[r].Low               = list[r]["main"]["temp_min"].as<float>();          Serial.println("TLow: " + String(WxForecast[r].Low));
      WxForecast[r].High              = list[r]["main"]["temp_max"].as<float>();          Serial.println("THig: " + String(WxForecast[r].High));
      WxForecast[r].Pressure          = list[r]["main"]["pressure"].as<float>();          Serial.println("Pres: " + String(WxForecast[r].Pressure));
      WxForecast[r].Humidity          = list[r]["main"]["humidity"].as<float>();          Serial.println("Humi: " + String(WxForecast[r].Humidity));
      //WxForecast[r].Forecast0         = list[r]["weather"][0]["main"].as<char*>();        Serial.println("For0: " + String(WxForecast[r].Forecast0));
      //WxForecast[r].Forecast1         = list[r]["weather"][1]["main"].as<char*>();        Serial.println("For1: " + String(WxForecast[r].Forecast1));
      //WxForecast[r].Forecast2         = list[r]["weather"][2]["main"].as<char*>();        Serial.println("For2: " + String(WxForecast[r].Forecast2));
      WxForecast[r].Icon              = list[r]["weather"][0]["icon"].as<char*>();        Serial.println("Icon: " + String(WxForecast[r].Icon));
      //WxForecast[r].Description       = list[r]["weather"][0]["description"].as<char*>(); Serial.println("Desc: " + String(WxForecast[r].Description));
      //WxForecast[r].Cloudcover        = list[r]["clouds"]["all"].as<int>();               Serial.println("CCov: " + String(WxForecast[r].Cloudcover)); // in % of cloud cover
      //WxForecast[r].Windspeed         = list[r]["wind"]["speed"].as<float>();             Serial.println("WSpd: " + String(WxForecast[r].Windspeed));
      //WxForecast[r].Winddir           = list[r]["wind"]["deg"].as<float>();               Serial.println("WDir: " + String(WxForecast[r].Winddir));
      WxForecast[r].Rainfall          = list[r]["rain"]["3h"].as<float>();                Serial.println("Rain: " + String(WxForecast[r].Rainfall));
      WxForecast[r].Snowfall          = list[r]["snow"]["3h"].as<float>();                Serial.println("Snow: " + String(WxForecast[r].Snowfall));
      WxForecast[r].Period            = list[r]["dt_txt"].as<char*>();                    Serial.println("Peri: " + String(WxForecast[r].Period));
    }
    //------------------------------------------
    float pressure_trend = WxForecast[0].Pressure - WxForecast[2].Pressure; // Measure pressure slope between ~now and later
    pressure_trend = ((int)(pressure_trend * 10)) / 10.0; // Remove any small variations less than 0.1
    WxConditions[0].Trend = "=";
    if (pressure_trend > 0)  WxConditions[0].Trend = "+";
    if (pressure_trend < 0)  WxConditions[0].Trend = "-";
    if (pressure_trend == 0) WxConditions[0].Trend = "0";

    if (Units == "I") Convert_Readings_to_Imperial();
  }
  return true;
}

String ConvertUnixTime(int unix_time) {
  // Returns either '21:12  ' or ' 09:12pm' depending on Units mode
  time_t tm = unix_time;
  struct tm *now_tm = localtime(&tm);
  char output[40];
  if (Units == "M") {
    strftime(output, sizeof(output), "%H:%M %d/%m/%y", now_tm);
  }
  else {
    strftime(output, sizeof(output), "%I:%M%P %m/%d/%y", now_tm);
  }
  return output;
}

float mm_to_inches(float value_mm) {
  return 0.0393701 * value_mm;
}

float hPa_to_inHg(float value_hPa) {
  return 0.02953 * value_hPa;
}

int JulianDate(int d, int m, int y) {
  int mm, yy, k1, k2, k3, j;
  yy = y - (int)((12 - m) / 10);
  mm = m + 9;
  if (mm >= 12) mm = mm - 12;
  k1 = (int)(365.25 * (yy + 4712));
  k2 = (int)(30.6001 * mm + 0.5);
  k3 = (int)((int)((yy / 100) + 49) * 0.75) - 38;
  // 'j' for dates in Julian calendar:
  j = k1 + k2 + d + 59 + 1;
  if (j > 2299160) j = j - k3; // 'j' is the Julian date at 12h UT (Universal Time) For Gregorian calendar:
  return j;
}

float SumOfPrecip(float DataArray[], int readings) {
  float sum = 0;
  for (int i = 0; i <= readings; i++) {
    sum += DataArray[i];
  }
  return sum;
}

String TitleCase(String text) {
  if (text.length() > 0) {
    String temp_text = text.substring(0, 1);
    temp_text.toUpperCase();
    return temp_text + text.substring(1); // Title-case the string
  }
  else return text;
}

double NormalizedMoonPhase(int d, int m, int y) {
  int j = JulianDate(d, m, y);
  //Calculate approximate moon phase
  double Phase = (j + 4.867) / 29.53059;
  return (Phase - (int) Phase);
}

//#############################################################################
//-------- DISPLAY SECTIONS ---------------------------------------------------
//#############################################################################

void DisplayWeather() {                          // 4.7" e-paper display is 960x540 resolution
  DisplayStatusSection(600, 20, wifi_signal);    // Wi-Fi signal strength and Battery voltage
  DisplayGeneralInfoSection();                   // Top line of the display
  DisplayMainWeatherSection(320, 110);           // Centre section of display for Location, temperature, Weather report, current Wx Symbol
  DisplayForecastSection(320, 220);              // 3hr forecast boxes
}

void DisplayStatusSection(int x, int y, int rssi) {
  setFont(OpenSans8B);
  DrawRSSI(x + 305, y + 15, rssi);
  DrawBattery(x + 150, y);
}

void DrawRSSI(int x, int y, int rssi) {
  int WIFIsignal = 0;
  int xpos = 1;
  for (int _rssi = -100; _rssi <= rssi; _rssi = _rssi + 20) {
    if (_rssi <= -20)  WIFIsignal = 30; //            <-20dbm displays 5-bars
    if (_rssi <= -40)  WIFIsignal = 24; //  -40dbm to  -21dbm displays 4-bars
    if (_rssi <= -60)  WIFIsignal = 18; //  -60dbm to  -41dbm displays 3-bars
    if (_rssi <= -80)  WIFIsignal = 12; //  -80dbm to  -61dbm displays 2-bars
    if (_rssi <= -100) WIFIsignal = 6;  // -100dbm to  -81dbm displays 1-bar
    fillRect(x + xpos * 8, y - WIFIsignal, 6, WIFIsignal, Black);
    xpos++;
  }
}

void DrawBattery(int x, int y) {
  uint8_t percentage = 100;
  esp_adc_cal_characteristics_t adc_chars;
  esp_adc_cal_value_t val_type = esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_11, ADC_WIDTH_BIT_12, 1100, &adc_chars);
  if (val_type == ESP_ADC_CAL_VAL_EFUSE_VREF) {
    Serial.printf("eFuse Vref:%u mV", adc_chars.vref);
    vref = adc_chars.vref;
  }
  float voltage = analogRead(36) / 4096.0 * 6.566 * (vref / 1000.0);
  if (voltage > 1 ) { // Only display if there is a valid reading
    Serial.println("\nVoltage = " + String(voltage));
    percentage = 2836.9625 * pow(voltage, 4) - 43987.4889 * pow(voltage, 3) + 255233.8134 * pow(voltage, 2) - 656689.7123 * voltage + 632041.7303;
    if (voltage >= 4.20) percentage = 100;
    if (voltage <= 3.20) percentage = 0;  // orig 3.5
    drawRect(x + 25, y - 14, 40, 15, Black);
    fillRect(x + 65, y - 10, 4, 7, Black);
    fillRect(x + 27, y - 12, 36 * percentage / 100.0, 11, Black);
    drawString(x + 85, y - 14, String(percentage) + "%  " + String(voltage, 1) + "v", LEFT);
  }
}

void DisplayGeneralInfoSection() {
  setFont(OpenSans10B);
  drawString(5, 2, City, LEFT);
  setFont(OpenSans8B);
  drawString(500, 2, Date_str + "  @   " + Time_str, LEFT);
}

void DisplayWeatherIcon(int x, int y) {
  DisplayConditionsSection(x, y, WxConditions[0].Icon, LargeIcon);
}

void DisplayMainWeatherSection(int x, int y) {
  setFont(OpenSans8B);
  DisplayTemperatureSection(x, y - 40);
  DisplayForecastTextSection(x - 55, y + 25);
  DisplayPressureSection(x - 25, y + 90, WxConditions[0].Pressure, WxConditions[0].Trend);
}

void DisplayForecastTextSection(int x, int y) {
#define lineWidth 34
  setFont(OpenSans12B);
  //Wx_Description = WxConditions[0].Main0;          // e.g. typically 'Clouds'
  String Wx_Description = WxConditions[0].Forecast0; // e.g. typically 'overcast clouds' ... you choose which
  Wx_Description.replace(".", ""); // remove any '.'
  int spaceRemaining = 0, p = 0, charCount = 0, Width = lineWidth;
  while (p < Wx_Description.length()) {
    if (Wx_Description.substring(p, p + 1) == " ") spaceRemaining = p;
    if (charCount > Width - 1) { // '~' is the end of line marker
      Wx_Description = Wx_Description.substring(0, spaceRemaining) + "~" + Wx_Description.substring(spaceRemaining + 1);
      charCount = 0;
    }
    p++;
    charCount++;
  }
  if (WxForecast[0].Rainfall > 0) Wx_Description += " (" + String(WxForecast[0].Rainfall, 1) + String((Units == "M" ? "mm" : "in")) + ")";
  //Wx_Description = wordWrap(Wx_Description, lineWidth);
  String Line1 = Wx_Description.substring(0, Wx_Description.indexOf("~"));
  String Line2 = Wx_Description.substring(Wx_Description.indexOf("~") + 1);
  drawString(x + 30, y + 5, TitleCase(Line1), LEFT);
  if (Line1 != Line2) drawString(x + 30, y + 30, Line2, LEFT);
}

void DrawSegment(int x, int y, int o1, int o2, int o3, int o4, int o11, int o12, int o13, int o14) {
  drawLine(x + o1,  y + o2,  x + o3,  y + o4,  Black);
  drawLine(x + o11, y + o12, x + o13, y + o14, Black);
}

void Nodata(int x, int y, bool IconSize, String IconName) {
  if (IconSize == LargeIcon) setFont(OpenSans24B); else setFont(OpenSans12B);
  drawString(x - 3, y - 10, "?", CENTER);
}

//#############################################################################
//-------- EPD DISPLAY FUNCTIONS ----------------------------------------------
//#############################################################################

void drawString(int x, int y, String text, alignment align) {
  char * data  = const_cast<char*>(text.c_str());
  int  x1, y1; //the bounds of x,y and w and h of the variable 'text' in pixels.
  int w, h;
  int xx = x, yy = y;
  get_text_bounds(&currentFont, data, &xx, &yy, &x1, &y1, &w, &h, NULL);
  if (align == RIGHT)  x = x - w;
  if (align == CENTER) x = x - w / 2;
  int cursor_y = y + h;
  write_string(&currentFont, data, &x, &cursor_y, framebuffer);
}

void fillCircle(int x, int y, int r, uint8_t color) {
  epd_fill_circle(x, y, r, color, framebuffer);
}

void drawFastHLine(int16_t x0, int16_t y0, int length, uint16_t color) {
  epd_draw_hline(x0, y0, length, color, framebuffer);
}

void drawFastVLine(int16_t x0, int16_t y0, int length, uint16_t color) {
  epd_draw_vline(x0, y0, length, color, framebuffer);
}

void drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color) {
  epd_write_line(x0, y0, x1, y1, color, framebuffer);
}

void drawCircle(int x0, int y0, int r, uint8_t color) {
  epd_draw_circle(x0, y0, r, color, framebuffer);
}

void drawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
  epd_draw_rect(x, y, w, h, color, framebuffer);
}

void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
  epd_fill_rect(x, y, w, h, color, framebuffer);
}

void fillTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1,
                  int16_t x2, int16_t y2, uint16_t color) {
  epd_fill_triangle(x0, y0, x1, y1, x2, y2, color, framebuffer);
}

void drawPixel(int x, int y, uint8_t color) {
  epd_draw_pixel(x, y, color, framebuffer);
}

void setFont(GFXfont const &font) {
  currentFont = font;
}

void edp_update() {
  epd_draw_grayscale_image(epd_full_screen(), framebuffer); // Update the screen
}