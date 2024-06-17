/* SX1509 Test

    Lessons learned:
    - Precise servo control will require software pulse generation
    - Pulses will need to be staggered, but with a maximum length of 2 ms and max 16 servos, that's 32 ms total to
      refresh all servos. Not a big deal.

    Expected software design for PAMI: I2C scheduler. There are three slaves on the bus (OLED, SX1509, motion tracker).
    Each slave will require its own set of operations. For the SX1509 we may also be looking at a variable number of I/O
    assignments (some for servos, some as inputs, some as outputs). This means the I2C will need to be handle by a
    dedicated thread. The scheduler should consist of a linked list of tasks that can be setup at run time if necessary.

    To take more decisions about this I'll need to look at the OLED and how to drive it (and where its data will come
    from. A global structure, I suppose)

    Tests implemented:
    - Write a pin
    - Read its state through another pin
    - Generate servo PWM's => Had to do it in software, the chip isn't capable

    Tests planned:


    Some code lifted from:
    - https://gist.github.com/JamesDunne/c2f59253bea14b97a4577f1050b9d917#file-sx1509_registers-h

    Notes on the SX1509:
    - By default, register address auto-increment
    - All pins default to inputs
    - Event registers allow for edge detection without using interrupts
    - No clock by default (not required for simple I/O)
    - 5V tolerance is an option that must be set in registers "RegHighInput" (default is 3.3V)

*/

#include "sx1509.hpp"
#include "LoggerAndDisplay.h"
#include "common_includes.h"
#include "sx1509_registers.h"

// Functions ============================================================
















// Setup the I2C bus controller for talking to the SX1509
void SX1509::init (int handle, LoggerAndDisplay *logger)
{
    mlogger = logger;
    // DEBUG - FORCING PIN MODES - for some reason, doing it before calling this method fails to do anything
    mode[0] = SX_SERVO;
    // mode[4] = SX_SERVO;
    mode[15] = SX_SERVO;

    // Save the file handle
	file_i2c = handle;
    getbus ();      // Sets the Pi's I2C controller to address the SX1509
    // Software reset the SX1509 as a precaution
    set(REG_RESET, 0x12);
    set(REG_RESET, 0x34);
    // Clear all the pins as a precaution
    set(REG_DATA_A, 0);
    set(REG_DATA_B, 0);
    // Prepare the direction configuration registers to match the mode[] property
    unsigned char RegDirA = 0xFF;   // Direction registers default to "all ones" meaning "all inputs"
    unsigned char RegDirB = 0xFF;
    for (int i = 0; i < 8; i++)
    {
        unsigned char mask = ~(1 << i);
        // REG_DIR_A composition based on first eight pins
        if (mode[i] == SX_SERVO)    // then the pin will be an output and its direction bit must be cleared
            RegDirA &= mask;
        // REG_DIR_B : same for pins 8..15
        if (mode[i + 8] == SX_SERVO)
            RegDirB &= mask;
    }
    // Write to the slave
    set(REG_DIR_A, RegDirA);
    set(REG_DIR_B, RegDirB);

    // Override for debug purposes
    // set(REG_DIR_A, 0xFE);
}

void SX1509::getbus ()
{
    // Acquire the slave (SX1509 default address is 0x3E)
    if (ioctl(file_i2c, I2C_SLAVE, 0x3E) < 0)
		LoggerAndDisplay::toreplace_log_and_display(5, 0, "Failed to acquire bus access and/or talk to slave.\n");	//ERROR HANDLING; you can check errno to see what went wrong    
}

// Write a register
void SX1509::set (unsigned char addr, unsigned char data)
{
    // Send the init frame to the stick
    char payload[2];
    payload[0] = addr;
    payload[1] = data;
    if (write(file_i2c, payload, 2) != 2)		//write() returns the number of bytes actually written, if it doesn't match then an error occurred (e.g. no response from the device)
		LoggerAndDisplay::toreplace_log_and_display(5, 0, "Failed to write to the i2c bus.\n");
}

// Read a register
unsigned char SX1509::get (unsigned char addr)
{
    unsigned char retval = 0;
    if (write(file_i2c, &addr, 1) != 1)		//write() returns the number of bytes actually written, if it doesn't match then an error occurred (e.g. no response from the device)
		LoggerAndDisplay::toreplace_log_and_display(5, 0, "Failed to write to the i2c bus.\n");
    if (read(file_i2c, &retval, 1) != 1)		//read() returns the number of bytes actually written, if it doesn't match then an error occurred (e.g. no response from the device)
		LoggerAndDisplay::toreplace_log_and_display(5, 0, "Failed to read from the i2c bus.\n");
    return retval;
}

// Sets the mode of a pin
void SX1509::setup (int pin, unsigned char pinmode)
{
    mode[pin % 16] = pinmode;
}

// Sets the position of a servo from 0 to 100 %
void SX1509::move (int pin, unsigned int pinratio)
{
    ratio[pin % 16] = pinratio;
}

































// Task method (must be called from the task sequencer)
void SX1509::task ()
{
    getbus ();
    // Read all pins and decode those that are setup as inputs.
    // In fact, just decode them all, it's simpler. Highler-level code should know not to read its own servo outputs.
    if (1)
    {
        unsigned char RegDataA = get(REG_DATA_A);
        unsigned char RegDataB = get(REG_DATA_B);
        for (int i = 0; i < 8; i++)
        {
            input[i] = (RegDataA & (1 << i)) ? 1 : 0;
            input[i + 8] = (RegDataB & (1 << i)) ? 1 : 0;
        }
    }
    // Perform pulse generation for each servo output.
    if (1)
    {
        for (int i = 0; i < 8; i++)
        {
            unsigned char mask = 0x01 << i;
            // Pins 0..7
            if (mode[i] == SX_SERVO)
            {
                set (REG_DATA_A, mask);   // Turn the servo's pin on
                usleep (500 + (ratio[i] * 10));     // Length of the pulse
                set (REG_DATA_A, 0);   // Turn the servo's pin back off
            }
            // Pins 8..15
            if (mode[i + 8] == SX_SERVO)
            {
                set (REG_DATA_B, mask);   // Turn the servo's pin on
                usleep (500 + (ratio[i + 8] * 10));     // Length of the pulse
                set (REG_DATA_B, 0);   // Turn the servo's pin back off
            }
            // ensure a certain dead time on all servo outputs, and update the mask
            // usleep (18000); // 18 ms
        }
    }
    /*
    // More direct test code
    int pulse = 420 + (ratio[0] * 10);
    set(REG_DATA_A, 1);
    usleep(pulse);
    set(REG_DATA_A, 0);
    usleep(18000);*/
}

// Demos ================================================================

// Servomotor test on GPIO0
// Remember to call init() first
void SX1509::demo1 ()
{
    getbus ();      // Sets the Pi's I2C controller to address the SX1509
    // Software reset the SX1509 as a precaution
    set(REG_RESET, 0x12);
    set(REG_RESET, 0x34);
    // Set pin 0 as output and "blink it" at 0.5 Hz for a little while
    // Change the direction of pin 0 to an output by changing RegDirA from 0xFF to 0xFE
    set(REG_DATA_A, 0);     // Clear all pins first
    set(REG_DIR_A, 0xFE);
    // This loop generates 100 pulses of increasing width to drive a servo through its full stroke, ten times
    for (int reps = 0; reps < 10; reps++)
    {
        for (int p = 0; p < 100; p++)  // With a digital servo, a single pulse per second is enough !
        {
            int pulse = 420 + (p * 10);
            set(REG_DATA_A, 1);
            usleep(pulse);
            set(REG_DATA_A, 0);
            usleep(18000);
        }
    }
}

// Main function ========================================================
/*
void main_sx ()
{
    
    // Initialize the bus for use with the SX1509
    sxinit ();

    // Software reset the SX1509 as a precaution
    sxwrite(REG_RESET, 0x12);
    sxwrite(REG_RESET, 0x34);

    // Set pin 0 as output and "blink it" at 0.5 Hz for a little while
    // Change the direction of pin 0 to an input by changing RegDirA from 0xFF to 0xFE
    sxwrite(REG_DATA_A, 0);     // Clear all pins first
    sxwrite(REG_DIR_A, 0xFE);
    unsigned char state = 0;

    // Setting up hardware PWM:
    // First, I'll need to enable the internal clock source (2 MHz)
    sxwrite(REG_CLOCK, 0x50); // Binary 0101_0000
    // There's also clock settings in a "misc" register:
    sxwrite(REG_MISC, 0x12); // Binary 0001_0010 also disabling address auto increment
    // Enable LED driver on pin 0
//    sxwrite(REG_LED_DRIVER_ENABLE_A, 0x01);
    // The data register shall control pulse length (not the definitive mode, I expect)
//    sxwrite(REG_T_ON_0, 0x01);
//    sxwrite(REG_OFF_0, 0); // 0x08);  // Binary : AAAA_ABBB where B should be zero and A is the off time.

//    sxwrite(REG_DATA_A, 0);     // Test for T-off control.

    int angle = 0;      // simple positioning, each LSB adds 250 µs, value goes from 0 to 4
 
    for (int p = 0; p < 100; p++)  // With a digital servo, a single pulse per second is enough !
    {
        int pulse = 420 + (p * 10);
        sxwrite(REG_DATA_A, 1);
        usleep(pulse);
        sxwrite(REG_DATA_A, 0);
        usleep(18000);
    }

    int timer = 11;
    while (timer--)
    {
        // For the following test, short SX1509 I/O 0 and 1 together.

        // Let's see how fast I can toggle:
        int i;
#if 0
        for (i = 0; i < 10; i++)
        {
            sxwrite(REG_DATA_A, 1);
            usleep(100 * i);  // 1 µs
            sxwrite(REG_DATA_A, 0);
            state = 1 - state;
            // Measured delay vs pulse length:
            // 0 - 580 µs
            // 1 - 680 µs
            // 2 - 780 µs and so on.
            // Conclusion : to produce servo pulses I'll need delays between 420 and 1420 µs
        }
#endif
        // Servo test : generate ten pulses at 20 ms period (roughly)
        int pulse = 420 + (angle * 100);
        for (int p = 0; p < 1; p++)  // With a digital servo, a single pulse per second is enough !
        {
            sxwrite(REG_DATA_A, 1);
            usleep(pulse);
            sxwrite(REG_DATA_A, 0);
            usleep(18000);
        }
        angle++;
        if (angle == 11) angle = 0;

        


        // It's pretty bad : 0.5 ms between toggles if done in a loop.
        // Consecutive calls : same
        // Actually, I need to generate pulses between 1 and 2 ms, so I could get away with using a delay function...
        // ... between the on and off writes.

        // The data register defaults to 0xFF so I should read a 1 on bit 0 and 1 of that register:
        // unsigned char d1 = sxread(REG_DATA_A);
        // LoggerAndDisplay::toreplace_log_and_display(2, 0, "first read = %i", d1);
        // I read 0x03 which means that indeed both pins are at 1.

        // sxwrite(REG_DATA_A, 0);     // Test for T-off control.

        LoggerAndDisplay::toreplace_log_and_display(1, 0, "%i", timer);
        refresh ();
        sleep (1);
    }

    // Set all pins as inputs again, for safety
    sxwrite(REG_DIR_A, 0xFF);

    // Close the bus
    close(file_i2c);
    
}*/