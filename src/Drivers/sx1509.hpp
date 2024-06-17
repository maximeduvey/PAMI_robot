/*  SX1509 Driver
    Nefastor
    This is a VERY SPECIALIZED driver for the 2024 PAMI
    Initial design in C, this is a C++ port to save my sanity and brain cells
*/

#ifndef __SX1509_PAMI_DRIVER__
#define __SX1509_PAMI_DRIVER__

// Pin mode constants
#define SX_INPUT    (0)
#define SX_SERVO    (1)

// PAMI 2024 Specific constants:
#define LEFT_SERVO  (15)
#define RIGHT_SERVO (0)
#define LEFT_AHEAD (0)
#define LEFT_DEPLOYED (100)
#define RIGHT_AHEAD (100)
#define RIGHT_DEPLOYED (0)

class LoggerAndDisplay;

class SX1509
{
    public:
        int file_i2c;                   // I2C bus handle
        LoggerAndDisplay *mlogger = nullptr;

    public:     // pin configuration and state
        unsigned char mode[16];      // Mode for each individual pin
        unsigned int ratio[16];      // When a pin is setup as servo output, this property determines the commanded angle, between 0 and 100%
        unsigned char input[16];     // When a pin is setup as input, its state will be stored in this property for the application to access

    public:     // Low-level methods
        void init (int handle, LoggerAndDisplay *logger);     // Takes an I2C bus file handle and initializes the SX1509 on that bus
        void getbus ();             // When the bus has multiple slaves, call this prior to talking to the SX1509
        void set (unsigned char addr, unsigned char data);        // Write a register
        unsigned char get (unsigned char addr);                  // Read a register

    public:     // High-level methods
        void setup (int pin, unsigned char pinmode); // { mode[pin % 16] = pinmode; }       // Sets the mode of a pin
        void move (int pin, unsigned int pinratio); // { ratio[pin % 16] = pinratio; }      // Sets the position of a servo from 0 to 100 %
        void task ();       // Task method for this driver

    public:     // Demo methods
        void demo1 ();
};

#endif // __SX1509_PAMI_DRIVER__