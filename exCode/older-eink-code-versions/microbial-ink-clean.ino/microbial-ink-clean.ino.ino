// Slow Serif
// By Matthew Halpenny



#define ENABLE_GxEPD2_GFX 0
#include <GxEPD2_BW.h>

#include <Fonts/FreeMonoBold9pt7b.h>
#include <Fonts/FreeMono9pt7b.h>  
#include <Fonts/FreeSans9pt7b.h>

#define GxEPD2_DISPLAY_CLASS GxEPD2_BW
#define GxEPD2_DRIVER_CLASS GxEPD2_750_T7

#define GxEPD2_BW_IS_GxEPD2_BW true
#define GxEPD2_3C_IS_GxEPD2_3C true
#define GxEPD2_7C_IS_GxEPD2_7C true
#define GxEPD2_1248_IS_GxEPD2_1248 true
#define IS_GxEPD(c, x) (c##x)
#define IS_GxEPD2_BW(x) IS_GxEPD(GxEPD2_BW_IS_, x)
#define IS_GxEPD2_3C(x) IS_GxEPD(GxEPD2_3C_IS_, x)
#define IS_GxEPD2_7C(x) IS_GxEPD(GxEPD2_7C_IS_, x)
#define IS_GxEPD2_1248(x) IS_GxEPD(GxEPD2_1248_IS_, x)
#define MAX_DISPLAY_BUFFER_SIZE 65536ul 
#define MAX_HEIGHT(EPD) (EPD::HEIGHT <= MAX_DISPLAY_BUFFER_SIZE / (EPD::WIDTH / 8) ? EPD::HEIGHT : MAX_DISPLAY_BUFFER_SIZE / (EPD::WIDTH / 8))
GxEPD2_DISPLAY_CLASS<GxEPD2_DRIVER_CLASS, MAX_HEIGHT(GxEPD2_DRIVER_CLASS)> display(GxEPD2_DRIVER_CLASS(/*CS=*/ 15, /*DC=*/ 27, /*RST=*/ 26, /*BUSY=*/ 25));

#include "bitmaps/Bitmaps640x384.h"

#define MAG_PIN 4
#define MAG_PIN_BITMASK 0x400000000 
#define MOSI_PIN 14
#define MISO_PIN 19 
#define SCK_PIN 13
#define BUSY_PIN 25
#define DC_PIN 27
#define RST_PIN 26
#define CS_PIN 15
#define Threshold 05 

// counter
int counter = 0;
bool initScreen = false;
RTC_DATA_ATTR int bootCount = 0;

void print_wakeup_reason(){
  esp_sleep_wakeup_cause_t wakeup_reason;

  wakeup_reason = esp_sleep_get_wakeup_cause();

  switch(wakeup_reason)
  {
    case ESP_SLEEP_WAKEUP_EXT0 : Serial.println("Wakeup caused by external signal using RTC_IO"); break;
    case ESP_SLEEP_WAKEUP_EXT1 : Serial.println("Wakeup caused by external signal using RTC_CNTL"); break;
    case ESP_SLEEP_WAKEUP_TIMER : Serial.println("Wakeup caused by timer"); break;
    case ESP_SLEEP_WAKEUP_TOUCHPAD : Serial.println("Wakeup caused by touchpad"); break;
    case ESP_SLEEP_WAKEUP_ULP : Serial.println("Wakeup caused by ULP program"); break;
    default : Serial.printf("Wakeup was not caused by deep sleep: %d\n",wakeup_reason); break;
  }
}

bool standbyClear = false;

void setup() {

  Serial.begin(115200);
  Serial.println();
  Serial.println("setup");
  display.init(115200); // uses standard SPI pins, e.g. SCK(18), MISO(19), MOSI(23), SS(5)
  // *** special handling for Waveshare ESP32 Driver board *** //
  // ********************************************************* //
  SPI.end(); // release standard SPI pins, e.g. SCK(18), MISO(19), MOSI(23), SS(5)
  //SPI: void begin(int8_t sck=-1, int8_t miso=-1, int8_t mosi=-1, int8_t ss=-1);
  SPI.begin(13, 12, 14, 15); // map and init SPI pins SCK(13), MISO(12), MOSI(14), SS(15)
  // *** end of special handling for Waveshare ESP32 Driver board *** //
  // **************************************************************** //

  if (standbyClear == true){
  //refresh screen
  clearScreen();
  }else{
    ++bootCount;
    Serial.println("Boot number: " + String(bootCount));
    print_wakeup_reason();
    printNovella();
    delay(1000);
    goToSleep();
  }
}

void loop() 
{
}


const char preText[] = "Hello World, this is my new book!";
String slowEdit[10] = {" In ", "the ", "long ", "haul ", "writings ", "have ", "decisively reshaped many debates that animate environmental fallout… This demands an understanding of the manifesto’s message.",
"\n \n  It performs this cartography while present in the view. The body of the delta and its substrates; of megamergers; of disappearing problematic brand names through that act of national reengineering.",
"\n \n  The invention of emptiness, emptiness being the vessel itself for the order of the Western powers typically supported by oligarchs, dictators, and assays. Indeed, aesthetics becomes twisted into a soap bubble, exporting a part of reinvention as pure wilderness.",
"\n \n  We can read these scenes as intimating the twilight of the megadam and, disproportionately, children. However, in consuming oxygen, thereby rendering the deceased immaterial: he was only staged to  none of these combined effects; munitions represent a novel."};
String novella = "";

void printNovella(){
  
  for (int i=1; i <= bootCount; i++){
    novella.concat(slowEdit[i-1]);
    Serial.println(novella);
  }
  
  display.setRotation(0);
  display.setFont(&FreeMono9pt7b);
  display.setTextColor(GxEPD_BLACK);
  int16_t tbx, tby;
  uint16_t tbw, tbh;
  display.getTextBounds(novella, 0, 0, &tbx, &tby, &tbw, &tbh);
  uint16_t x = ((display.width() - tbw) / 2) - tbx;
  uint16_t y = ((display.height() - tbh) / 2) - tby;
  display.setFullWindow();
  display.firstPage();
  do
  {
    display.fillScreen(GxEPD_WHITE);
    display.setCursor(10, 35);
    display.print(novella);
    display.print(" ");
  }
  while (display.nextPage()); 
  display.hibernate();
}

void goToSleep(){
  touchAttachInterrupt(T0, callback, Threshold);
  esp_sleep_enable_touchpad_wakeup();
  
  Serial.println("Going to sleep now");
  delay(1000);
  Serial.flush(); 
  esp_deep_sleep_start();
  Serial.println("This will never be printed");
}

void clearScreen(){
  display.setFullWindow();
  display.firstPage();
  do
  {
    display.fillScreen(GxEPD_WHITE);
  }
  while (display.nextPage());
  display.hibernate();
  delay(5000);
  Serial.println("cleared... see you soon :)");
}

void callback(){
  //placeholder callback function
}
