/*  PAMI 2024
    Nefastor
    Header for the main.cpp file
    Contains the robot's main class declaration
*/

#ifndef __PAMI_MAIN_HPP__
#define __PAMI_MAIN_HPP__

// I2C-related
#include <unistd.h>				//Needed for I2C port, also microsecond timing (usleep)
#include <fcntl.h>				//Needed for I2C port
#include <sys/ioctl.h>			//Needed for I2C port
#include <linux/i2c-dev.h>		//Needed for I2C port

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

// s3 == 0 for long move
//#define TIMING_A1   (((io.s3 == 0) ? 92000000 : 91000000))  // 92.0 s
//#define TIMING_A2   (((io.s3 == 0) ? 92500000 : 91500000))  // sensors off limit 92.5 s
#define TIMING_A1   (92000000)
#define TIMING_A2   (92500000)

// ==== Classes =========================================
class LoggerAndDisplay; // predefine logger

class PAMI
{
    public:
    enum PAMI_STATE : int {
        PAMI_BIST = 0, // "Built-In Self Test" 
        PAMI_IDLE = 1,
        PAMI_ARMED = 2,
        PAMI_DELAY = 3,
        PAMI_RUN = 4
    };

    public: // PAMI Identifaction
        char hostname[10];    // Local hostname (no need to make it long, it's either "canpi3" or "pami1")
        int id;         // PAMI number (last digit of the hostname: 1, 2 or 3)
        char IP[NI_MAXHOST];     // IP address, as string

    public: // PAMI State
        PAMI_STATE state;      // PAMI state machine current state
        GPIO io;        // The discrete GPIO (inputs) of the PAMI mainboard
        struct timespec tzero;     // System time when the pin was pulled (causing a state transition from ARMED to DELAY)
        unsigned int time;      // Time in microseconds since the pin was pulled

    public: // PAMI I2C Bus and slaves
        int file_i2c;   // I2C bus handle
        OLED oled;      // OLED display driver (SSD1306 - 128x32 pixels)
        SX1509 sx;      // SX1509 I/O expander

    public: // PAMI propulsion
        DRIVE drive;

    public :
        LoggerAndDisplay *mlogger = nullptr;

    public:
        PAMI();         // Default constructor
        ~PAMI();
        void init(LoggerAndDisplay *logger);    // PAMI Initialization (earliest, before the state machine starts)
        void tasks();   // Sequence of tasks to be run in a loop
        void handle_time(); // Task function for this class. Essentially handles time management
        // Task methods specific to each state, when applicable
        void task_bist();
        void task_idle();
        void task_armed();
        void task_delay();
        void task_run();

    public:     // Extra methods
        void stateString (char* );  // Provides the current state of the PAMI state machine as a string 
};

#endif // __PAMI_MAIN_HPP__