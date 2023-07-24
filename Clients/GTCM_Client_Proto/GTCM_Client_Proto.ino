#ifndef BOARD_HAS_PSRAM
#error "Please enable PSRAM !!!"
#endif

#include <Arduino.h>
#include "epd_driver.h"
#include "pins.h"               // default LilyGO pin access header
#include <pcf8563.h>            // Required for NXP PCF8563 RTC chip
#include <HTTPClient.h>         // Arduino HTTP Client
#include <WiFi.h>               // Arduino Wifi
#include <SPI.h>                // SPI Communication
//#include "Button2.h"

#include "credentials.h"        // WiFi & server credentials

#define SCREEN_WIDTH   EPD_WIDTH
#define SCREEN_HEIGHT  EPD_HEIGHT

//fonts
#include "opensans8b.h"
#include "opensans10b.h"
#include "opensans12b.h"
#include "opensans18b.h"
#include "opensans24b.h"

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

GFXfont  currentFont;
uint8_t *framebuffer;
int wifi_signal;
int MIDDLE_Y, MIDDLE_X;

String test = "Hello World";
String test2 = "Hello Again, World";
String test3 = "Goodbye";

//Button2 btn1(BUTTON_1);
int state = 0;
int vref = 1100;
int cursor_x = 20;
int cursor_y = 60;

//test text by LilyGO
const char *overview[] = {
    "   ESP32 is a single 2.4 GHz Wi-Fi-and-Bluetooth\n"\
    "combo chip designed with the TSMC ultra-low-po\n"\
    "wer 40 nm technology. It is designed to achieve \n"\
    "the best power and RF performance, showing rob\n"\
    "ustness versatility and reliability in a wide variet\n"\
    "y of applications and power scenarios.\n",
    "➸ Xtensa® dual-core 32-bit LX6 microprocessor\n"\
    "➸ 448 KB ROM & External 16MBytes falsh\n"\
    "➸ 520 KB SRAM & External 16MBytes PSRAM\n"\
    "➸ 16 KB SRAM in RTC\n"\
    "➸ Multi-connections in Classic BT and BLE\n"\
    "➸ 802.11 n (2.4 GHz), up to 150 Mbps\n",
    "➸ 16 color grayscale\n"\
    "➸ Use with 4.7\" EPDs\n"\
    "➸ High-quality font rendering\n"\
    "➸ ~630ms for full frame draw\n"
};

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

    //testing buttons for manual refresh
    //btn1.setPressedHandler(buttonPressed);

    currentFont = OpenSans10B;
    MIDDLE_Y = EPD_HEIGHT / 2;
    MIDDLE_X = EPD_WIDTH / 2;
    //StartWiFi(); //WiFi works in this sketch, disabled for now

    epd_init();

    epd_poweron();
    epd_clear();

    //native string display function to test
    //write_string((GFXfont *)&FiraSans, (char *)overview[0], &cursor_x, &cursor_y, framebuffer);

    //looping with a delay makes it impossible to reflash 
    DisplayText();
    edp_update();

    epd_poweroff();
    //we need the wake function (button) for this - deep sleep
    //epd_poweroff_all();
}

void loop()
{
    // btn1.loop();
    // epd_poweron();
    // epd_clear();

    // DisplayText();
    // edp_update();
    // //memset(framebuffer, 0xFF, EPD_WIDTH * EPD_HEIGHT / 2);
    // //epd_poweroff();
    // delay(10000);
    // epd_poweroff_all();
}

// working version 
// void DisplayText() {
//   setFont(OpenSans10B);
//   drawString(5, 2, test, LEFT);
//   setFont(OpenSans8B);
//   drawString(500, 2, test2 + "  @   " + test3, LEFT);
// }

void DisplayText() {
  setFont(OpenSans18B);
  drawString(MIDDLE_X, MIDDLE_Y, test, CENTER);
}

//WiFi is stored as variable, so it doesn't need to be called
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

void setFont(GFXfont const &font) {
  currentFont = font;
}

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

void drawPixel(int x, int y, uint8_t color) {
  epd_draw_pixel(x, y, color, framebuffer);
}

void edp_update() {
  epd_draw_grayscale_image(epd_full_screen(), framebuffer); // Update the screen
}

//Look into sleep function, wake by button, and button options
// void displayInfo(void)
// {
//     cursor_x = 20;
//     cursor_y = 60;
//     state %= 4;
//     switch (state) {
//     case 0:
//         epd_clear();
//         write_string((GFXfont *)&FiraSans, (char *)overview[0], &cursor_x, &cursor_y, NULL);
//         break;
//     case 1:
//         epd_clear_area(area1);
//         write_string((GFXfont *)&FiraSans, (char *)overview[1], &cursor_x, &cursor_y, NULL);
//         break;
//     case 3:
//         delay(1000);
//         epd_clear_area(area1);
//         write_string((GFXfont *)&FiraSans, "DeepSleep", &cursor_x, &cursor_y, NULL);
//         epd_poweroff_all();
//     #if defined(T5_47)
//             // Set to wake up by GPIO39
//             esp_sleep_enable_ext1_wakeup(GPIO_SEL_39, ESP_EXT1_WAKEUP_ALL_LOW);
//     #endif
//             esp_deep_sleep_start();
//             break;
//                 break;
//     case 4:
//         break;
//     default:
//         break;
//     }
//     epd_poweroff();
// }

// void buttonPressed(Button2 &b)
// {
//     epd_clear());
//     setFont(OpenSans18B);
//     write_string(&currentFont, (char *)overview[0], &cursor_x, &cursor_y, NULL);
// }