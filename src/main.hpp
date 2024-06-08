/*  PAMI 2024
    Nefastor
    Header for the main.cpp file
    Contains the robot's main class declaration
*/

#ifndef __PAMI_MAIN_HPP__
#define __PAMI_MAIN_HPP__

// Generic headers
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// Time management
#include <time.h>
// I2C-related
#include <unistd.h>				//Needed for I2C port, also microsecond timing (usleep)
#include <fcntl.h>				//Needed for I2C port
#include <sys/ioctl.h>			//Needed for I2C port
#include <linux/i2c-dev.h>		//Needed for I2C port
// CAN bus
#include <net/if.h>
#include <linux/can.h>
#include <linux/can/raw.h>
// Threads
#include <pthread.h>
// UDP networking
#include <arpa/inet.h> // Not so sure
#include <sys/socket.h>  // data structures for socket
#include <netinet/in.h>	 // constants and structures for IP domain address
#include <sys/types.h>	// definitions of data types used for system calls
#include <netdb.h>      // for gethostbyname()
#include <ifaddrs.h>    // for getifaddrs()
// ncurses
#include <ncurses.h>
// Odometry
#include <math.h>   // IMPORTANT - requires "-lm" in gcc command line
// Drivers
#include "Drivers/oled.hpp"
#include "Drivers/sx1509.hpp"
#include "Drivers/gpio.hpp"
#include "Drivers/drive.hpp"

// ==== Macros ==========================================

// #define DEBUG_TIMINGS   // Enables shorter delays for debugging
#ifdef DEBUG_TIMINGS
#define PAMI_WAIT_DELAY     (5000000)       // From pulling the pin, time (in microseconds) the PAMI must wait before "going to work"
#define PAMI_STOP_TIME      (10000000)      // Match duration (in microseconds) after which the PAMI must become idle
#else
#define PAMI_WAIT_DELAY     (90000000)       // From pulling the pin, time (90 seconds in microseconds) the PAMI must wait before "going to work"
#define PAMI_STOP_TIME      (100000000)      // Match duration (100 seconds in microseconds) after which the PAMI must become idle
// #define PAMI_STOP_TIME      (92000000)
#endif

// State machine states (values of PAMI::state)
#define PAMI_BIST   (0)     // Initial state
#define PAMI_IDLE   (1)     // When not playing a match
#define PAMI_ARMED  (2)     // Safety pin in place, ready to play a match
#define PAMI_DELAY  (3)     // 90 second phase of the match where the PAMI waits
#define PAMI_RUN    (4)     // The active phase, where the PAMI does its job

// ==== Classes =========================================

class PAMI
{
    public: // PAMI Identifaction
        char hostname[10];    // Local hostname (no need to make it long, it's either "canpi3" or "pami1")
        int id;         // PAMI number (last digit of the hostname: 1, 2 or 3)
        char IP[NI_MAXHOST];     // IP address, as string

    public: // PAMI State
        int state;      // PAMI state machine current state
        GPIO io;        // The discrete GPIO (inputs) of the PAMI mainboard
        struct timespec tzero;     // System time when the pin was pulled (causing a state transition from ARMED to DELAY)
        unsigned int time;      // Time in microseconds since the pin was pulled

    public: // PAMI I2C Bus and slaves
        int file_i2c;   // I2C bus handle
        OLED oled;      // OLED display driver (SSD1306 - 128x32 pixels)
        SX1509 sx;      // SX1509 I/O expander

    public: // PAMI propulsion
        DRIVE drive;

    public:
        PAMI();         // Default constructor
        void init();    // PAMI Initialization (earliest, before the state machine starts)
        void tasks();   // Sequence of tasks to be run in a loop
        void task();    // Task function for this class. Essentially handles time management
        // Task methods specific to each state, when applicable
        void task_bist();
        void task_idle();
        void task_armed();
        void task_delay();
        void task_run();

    public:     // Extra methods
        void stateString (char* );  // Provides the current state of the PAMI state machine as a string 

};


// ==== Externs =========================================

extern PAMI pami;

#endif // __PAMI_MAIN_HPP__