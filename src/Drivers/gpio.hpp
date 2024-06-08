/*  PAMI 2024 GPIO Driver
    Nefastor
    This is a VERY SPECIFIC wrapper for the pigpio library,
    designed to handle the Pi GPIO on the PAMI mainboard.
    Those GPIO are all inputs used for sensors and switches
*/

#ifndef __PAMI_GPIO_HPP__
#define __PAMI_GPIO_HPP__

// Macros ===============================================================

// GPIO numbers (GPIOxx, not the header pin numbers)
#define DIP1 4      // GPIO4  - pin 7
#define DIP2 27     // GPIO27 - pin 13
#define DIP3 23     // GPIO23 - pin 16
#define DIP4 22     // GPIO22 - pin 15
#define SIDE 24     // GPIO24 - pin 18
#define HALL 21     // GPIO21 - pin 40
#define TOR1 7      // GPIO7 - pin 26
#define TOR2 5      // GPIO5 - pin 29
#define MINT 20     // GPIO20 - pin 38

// State constants
#define PIN_PRESENT (0)
#define PIN_PULLED  (1)

// Class ================================================================

class GPIO
{
    public:     // State of all PAMI inputs:
        // DIP switch:
        int s1;     // GPIO22 - pin 15
        int s2;     // GPIO23 - pin 16
        int s3;     // GPIO27 - pin 13
        int s4;     // GPIO4 - pin 7
        // "Side" switch
        int side;   // GPIO24 - pin 18
        // Hall sensor (arming pin a.k.a. tirette)
        int pin;    // GPIO21 - pin 40
        // ToR sensors from the battery board
        int tor1;     // GPIO7 - pin 26
        int tor2;     // GPIO5 - pin 29
        // Motion tracker interrupt pin
        int mtint;     // GPIO20 - pin 38

    public:
        void init ();        
        void terminate ();
        void task ();   // Reads all GPIO to update the state of this object

    public:     // event handlers
        static void onChange (int gpio, int level, uint32_t tick);
};


#endif // __PAMI_GPIO_HPP__