#pragma once

/// this header provides common include accross all the necessary class
/// a lot of driver write or read in register, 
/// this header is here to prevent header include loop and have a cleaner architecture

// Generic headers
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// Time management
#include <time.h>

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

// I2C-related
#include <unistd.h>				//Needed for I2C port, also microsecond timing (usleep)
#include <fcntl.h>				//Needed for I2C port
#include <sys/ioctl.h>			//Needed for I2C port
#include <linux/i2c-dev.h>		//Needed for I2C port


#include <ncurses.h>

#define PAMI_ID_ONE 1
#define PAMI_ID_TWO 2
#define PAMI_ID_THREE 3
