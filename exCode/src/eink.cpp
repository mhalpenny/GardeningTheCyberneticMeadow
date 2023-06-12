/******************************************************/
//       THIS IS A GENERATED FILE - DO NOT EDIT       //
/******************************************************/

#line 1 "x:/Developer/Particle/eink-project/src/eink.ino"
// This #include statement was automatically added by the Particle IDE.
#include <GxEPD2_PP.h>
#include <Adafruit_GFX_RK.h>
#define ENABLE_GxEPD2_GFX 0
#include <Arduino.h>
void startupFunctions();
void setup();
void loop();
#line 6 "x:/Developer/Particle/eink-project/src/eink.ino"
//#include <Adafruit_GFX.h>
#include <FreeMonoBold9pt7b.h>
#include <GxEPD2_BW.h>
//#include <GxEPD2_3C.h>
//#include <SPI.h> //idk if i need this one

//#if defined(PARTICLE)
//GxEPD2_3C<GxEPD2_750c, GxEPD2_750c::HEIGHT / 4 > display(GxEPD2_750c(/*CS=D5*/ SS, /*DC=*/ A1, /*RST=*/ A0, /*BUSY=*/ D4));
//#endif

//#include "images.h"

//e-ink 
#include "Particle.h"

unsigned long firstAvailable = 0;
int counter;
//String novella = "";
unsigned short int wakeUpSwitch = D0; 
//int DUTY_CYCLE = 60; // specify duty cycle in seconds 
unsigned long DAY_CYCLE = 86400;
int DEBUG_CYCLE = 10;
int sleepMode = 0;
int swState = 0;
int val = 0;

STARTUP(startupFunctions());

void startupFunctions() {   
    // RGB.control(true);
    //RGB.brightness(0);
   System.enableFeature(FEATURE_RETAINED_MEMORY);
    //SYSTEM_MODE(MANUAL);
}

//SystemSleepConfiguration config;

void setup() {

    Particle.publish("Sleep", "State", 0, PUBLIC);
    //Particle.publish("Switch", "Value", 0, PUBLIC);
    pinMode(wakeUpSwitch, INPUT_PULLDOWN);
    
}

void loop() {
    
//--- START NOT IN USE -----------

     // bool wifiReady = WiFi.ready();
    //WiFi.useStaticIP(); //cuts down on data
    
    //tell the server to generate word     
   // udp.begin(11200);
   // udp.beginPacket(broadcast, PORT);
   // udp.write(5);
   // udp.endPacket();
    
    //receive new word
    //String word = '';
    
    //add word to string
    //sprintf(novella, "%d; %d", oldNovella, word);
    
    //refresh e-ink via SPI
    
//--- END NOT IN USE -----------
            
            //RESET SLEEP VARIABLE
    		sleepMode = 0;
			Particle.publish("Sleep", sleepMode ? "ON" : "OFF");
			delay(3000);
     
            //START SLEEP VARIABLE
            sleepMode = !sleepMode;
            Particle.publish("Sleep", sleepMode ? "ON" : "OFF");
			delay(3000);
			
			//CHECK VALUE OF MAGNETIC SWITCH
		    //val = digitalRead(wakeUpSwitch);
			//Particle.publish("Switch", String(val));
			//delay(1200);

			//--- TEST STOP MODES ---
			//STOP SLEEP System.sleep(pin, edge, timeout)
            //System.sleep(uint16_t wakeupPin, uint16_t CHANGE, DAY_CYCLE);
            //System.sleep(uint16_t wakeUpPin, uint16_t edgeTriggerMode);
           
           //SLEEP STOP
           System.sleep(wakeUpSwitch, FALLING);
           
           //POST-SLEEP

}