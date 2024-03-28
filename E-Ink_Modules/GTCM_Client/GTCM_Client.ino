//--- INFORMATION -----------------------------------------------------------------------------------------------------------------------
// AUTHOR: Matthew Halpenny (2023-2024)
// PROJECT: Gardening the Cybernetic Meadow
// MODULE: The Cybernetic Meadow (Installation Artwork)
// BOARD: LILYGO® TTGO T5-4.7 Inch E-paper ESP32 V3 Version 16MB FLASH 8MB PSRAM WIFI/Bluetooth Module Development Board
//
// DETAILS: This code enables the E-Ink ESP Board to:
//          - Use WiFi credentials to access the solar server on a local network
//          - Fetch new text generated with NLP on the project's solar server 
//          - Sleep between power triggers (supplied by the PMC of the MFCs)
//          - Encode the data for a binary pixel array and flash (update) the e-ink screen
//-----------------------------------------------------------------------------------------------------------------------------------------

//-- LIBRARIES & VARIABLES -----------------------------------------------------------------------------------------------------------------
// Board-specific requirements
#ifndef BOARD_HAS_PSRAM
#error "Please enable PSRAM !!!"
#endif

// Included libraries
#include <Arduino.h>            // Default arduino MCU library
#include "epd_driver.h"         // Drive for ePaper Display (E-Ink)
#include "pins.h"               // Default LilyGO pin access library
#include <pcf8563.h>            // Required library for NXP PCF8563 RTC (Real-Time Clock) chip
#include <HTTPClient.h>         // Arduino HTTP Client library
#include <WiFi.h>               // Arduino Wifi library
#include "esp_sleep.h"          // ESP32 sleep library
#include <SPI.h>                // Default SPI Communication library for communicating with the MCU and peripherals
//#include "Button2.h"          // LilyGO button library for TTGO T5-4.7 buttons

// Credentials
#include "credentials.h"        // WiFi & server credentials

// Fonts
#include "opensans8b.h"
#include "opensans10b.h"
#include "opensans12b.h"
#include "opensans18b.h"
#include "opensans24b.h"

// E-Ink variable declarations
#define SCREEN_WIDTH   EPD_WIDTH
#define SCREEN_HEIGHT  EPD_HEIGHT
enum alignment {LEFT, RIGHT, CENTER};
#define White         0xFF
#define LightGrey     0xBB
#define Grey          0x88
#define DarkGrey      0x44
#define Black         0x00
#define autoscale_on  true
#define autoscale_off false

// Font processing variables
GFXfont  currentFont;
uint8_t *framebuffer;
int state = 0;
int vref = 1100;
int cursor_x = 20;
int cursor_y = 60;
int MIDDLE_Y, MIDDLE_X;

// Display variables (strings)
int wordCount = 0;
int lineCount = 0;
DynamicJsonDocument doc(200);
String JsonIncoming = "";
String CURRENT_TEXT1 = "";
String CURRENT_TEXT2 = "";
String CURRENT_TEXT3 = "";
String OUTPUT1 = "";
String OUTPUT2 = "";
String OUTPUT3 = "";


// Board variables
int wifi_signal;
//Button2 btn1(BUTTON_1);

// Sleep variables
WiFi.enableWakeupUDP(); // Enable WoWLAN to wake up on any UDP packet (allows the solar server to wake ESP32)
esp_sleep_enable_uart_wakeup(64 * 1024); // Enable UART wakeup, with 64k RX buffer (avoids sleep lockouts when flashing new code)
//-----------------------------------------------------------------------------------------------------------------------------------------

//-- SETUP --------------------------------------------------------------------------------------------------------------------------------
void setup()
{
    Serial.begin(115200);
    delay(1000);
    framebuffer = (uint8_t *)ps_calloc(sizeof(uint8_t), EPD_WIDTH * EPD_HEIGHT / 2);
    if (!framebuffer) {
        Serial.println("alloc memory failed !!!");
        while (1);
    }
    memset(framebuffer, 0xFF, EPD_WIDTH * EPD_HEIGHT / 2);

    currentFont = OpenSans12B;
    MIDDLE_Y = EPD_HEIGHT / 2;
    MIDDLE_X = EPD_WIDTH / 2;

    StartWiFi(); 
    epd_init();
    epd_poweron();
    epd_clear();
    epd_poweroff();
    esp_deep_sleep_start();

    // Check the wake-up reason
    esp_sleep_wakeup_cause_t wakeupReason = esp_sleep_get_wakeup_cause();

        if (wakeupReason == ESP_SLEEP_WAKEUP_UART) {
        // The board woke up due to UART activity
        Serial.println("Woke up due to UART activity");
        // You can upload new code or perform any necessary actions here
    } else if (wakeupReason == ESP_SLEEP_WAKEUP_WIFI) {
        // The board woke up due to a WiFi packet
        Serial.println("Woke up due to WiFi packet");
        // The server told us theres an update, fetch the word
        fetchJsonData();
        epd_init();
        epd_poweron();
        epd_clear();
        DisplayText();
        edp_update();
        epd_poweroff();
        esp_deep_sleep_start();
    } else {
        // The board woke up for some other reason
        Serial.println("Woke up for a different reason");
    }
}
//-----------------------------------------------------------------------------------------------------------------------------------------

//-- LOOP ---------------------------------------------------------------------------------------------------------------------------------
void loop()
{ 
  //...
}
//-----------------------------------------------------------------------------------------------------------------------------------------

//-- FUNCTIONS [WiFi] --------------------------------------------------------------------------------------------------------------
// WiFi is stored as variable, so it doesn't need to be called
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
//-----------------------------------------------------------------------------------------------------------------------------------------

//-- FUNCTIONS [JSON - SERVER] ------------------------------------------------------------------------------------------------------
// Fetch new text updates stored on a JSON file generated by the solar server
void fetchJsonData() {
  if (WiFi.status() == WL_CONNECTED) {
    String jsonUrl = "http://" + String(serverIP) + String(jsonPath);
    HTTPClient http;
    http.begin(jsonUrl);
    int httpResponseCode = http.GET();

    if (httpResponseCode == 200) {
      DynamicJsonDocument doc(200);
      deserializeJson(doc, http.getString());
      // Access the JSON data
      JsonArray words = doc[0]["word"];
      JsonIncoming = JSON.stringify(words[0]);
      wordCount = wordCount++; 
    } else {
      Serial.println("Error fetching JSON file");
    }
    http.end();
  }
}
//-----------------------------------------------------------------------------------------------------------------------------------------

//-- FUNCTIONS [DISPLAY] -----------------------------------------------------------------------------------------------------------
// Set the font to be used
void setFont(GFXfont const &font) {
  currentFont = font;
}

// Format a string to be flashed to the E-Ink
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

// Format text to be compatible with E-Ink pixels
void DisplayText() {
  setFont(OpenSans12B);
  // Check the length of the current sentence, if its too long, break to a new line
  if (wordCount >= 7){
    lineCount = lineCount++;
    wordCount = 0;
  }
  // Format string output based on the number of lines
    //Since it's always triggered by the server, revise Strings
  if (lineCount == 0){
    OUTPUT1 = CURRENT_TEXT1 + JsonIncoming;
    CURRENT_TEXT1 = OUTPUT1;
    drawString(MIDDLE_X, MIDDLE_Y, OUTPUT1, CENTER);
  } else if (lineCount == 1){
    OUTPUT2 = CURRENT_TEXT2 + JsonIncoming;
    CURRENT_TEXT2 = OUTPUT2;
    drawString(MIDDLE_X, (MIDDLE_Y-10), OUTPUT1, CENTER); 
    drawString(MIDDLE_X, (MIDDLE_Y+10), OUTPUT2, CENTER);
  } else if (lineCount == 2){
    OUTPUT3 = CURRENT_TEXT3 + JsonIncoming;
    CURRENT_TEXT3 = OUTPUT3;
    drawString(MIDDLE_X, (MIDDLE_Y-20), OUTPUT1, CENTER); 
    drawString(MIDDLE_X, MIDDLE_Y, OUTPUT2, CENTER);
    drawString(MIDDLE_X, (MIDDLE_Y+20), OUTPUT3, CENTER);
  }
}

// Update a pixel on the E-Ink
void drawPixel(int x, int y, uint8_t color) {
  epd_draw_pixel(x, y, color, framebuffer);
}

// Flash the E-Ink screen to update the display
void edp_update() {
  epd_draw_grayscale_image(epd_full_screen(), framebuffer); // Update the screen
}
//-----------------------------------------------------------------------------------------------------------------------------------------