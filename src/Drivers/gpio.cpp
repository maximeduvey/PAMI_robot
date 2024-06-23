/* PAMI GPIO Test
   After some research I've decided to use the pigpio library (Pi GPIO)
   It is the recommended lib.
   https://abyz.me.uk/rpi/pigpio/download.html

   Of note:
   - DIP switch numbering is inverted on the board (switch marked 1 correspond to 4 on the schematic)

*/

#include "gpio.hpp"
#include "LoggerAndDisplay.h"
#include "common_includes.h"
#include <pigpio.h>

// Methods ==============================================================

void GPIO::init (LoggerAndDisplay *logger)
{
    mlogger = logger;
    
    // GPIO init
    gpioInitialise();
    
    // GPIO setup
    gpioSetMode(DIP1, PI_INPUT);
    gpioSetMode(DIP2, PI_INPUT);
    gpioSetMode(DIP3, PI_INPUT);
    gpioSetMode(DIP4, PI_INPUT);
    gpioSetMode(SIDE, PI_INPUT);
    gpioSetMode(HALL, PI_INPUT);
    gpioSetMode(TOR1, PI_INPUT);
    gpioSetMode(TOR2, PI_INPUT);
    gpioSetMode(MINT, PI_INPUT);

    // Event handlers registration
/*  // this is demo code, may be thread-based, not recommended for PAMI applications
    gpioSetAlertFunc(DIP1, GPIO::onChange);
    gpioSetAlertFunc(DIP2, GPIO::onChange);
    gpioSetAlertFunc(DIP3, GPIO::onChange);
    gpioSetAlertFunc(DIP4, GPIO::onChange);
    gpioSetAlertFunc(SIDE, GPIO::onChange);
    gpioSetAlertFunc(HALL, GPIO::onChange);
    gpioSetAlertFunc(mProximitysensorLeft, GPIO::onChange);
    gpioSetAlertFunc(mProximitysensorRight, GPIO::onChange);
    gpioSetAlertFunc(MINT, GPIO::onChange);
*/

}

void GPIO::terminate ()
{
    gpioTerminate();
}

void GPIO::task ()   // Reads all GPIO to update the state of this object
{
    s1 = 1 - gpioRead(DIP1);    // 1- inverts the result so that ON=1/OFF=0
    s2 = 1 - gpioRead(DIP2);
    s3 = 1 - gpioRead(DIP3);
    s4 = 1 - gpioRead(DIP4);
    side = gpioRead(SIDE);
    pin = gpioRead(HALL);
    mProximitysensorLeft = gpioRead(TOR1);
    mProximitysensorRight = gpioRead(TOR2);
    mtint = gpioRead(MINT);

    if (mProximitysensorLeft != lastKnowState_PSL){
        printf("LEFT proximity sensor changed from:%d, to:%d\n", lastKnowState_PSL, mProximitysensorLeft);
        lastKnowState_PSL = mProximitysensorLeft;
    }
    if (mProximitysensorRight != lastKnowState_PSR){
        printf("RIGHT proximity sensor changed from:%d, to:%d\n", lastKnowState_PSR, mProximitysensorRight);
        lastKnowState_PSR = mProximitysensorRight;
    }
    //printf("Sensor %d - %d\n", mProximitysensorLeft, mProximitysensorRight);

}

// Event handlers =======================================================

// Generic handler used for demo purposes
void GPIO::onChange (int gpio, int level, uint32_t tick)
{
   // static uint32_t lastTick=0;
   // float elapsed = (float)(tick-lastTick)/1000000.0;

    //mlogger->log_and_display(6, 0, "pin %d = %d\n", gpio, level);

   // refresh();

   // lastTick = tick;
}


