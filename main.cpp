/*  PAMI 2024 - Main Source File
    Nefastor for Team Goldorak
    Notes:
    - This program is designed to never exit. It is meant to run from boot
      and until power is cut or the Pi is somehow shutdown or rebooted.
*/


// Project headers
#include "main.hpp"

// ==== Globals =====================================

PAMI pami;

// ==== PAMI class methods ==========================

// Task method of the PAMI object (essentially, time management)
// Call after the robot's inputs have been sampled
void PAMI::task ()
{
    struct timespec ts;
    timespec_get(&ts, TIME_UTC);        // Get the current time
    unsigned long seconds = (ts.tv_sec - pami.tzero.tv_sec) * 1000000;  // expressed in microseconds
    unsigned long nanos = (ts.tv_nsec - pami.tzero.tv_nsec) / 1000;     // expressed in microseconds
    time = seconds + nanos;
}

// Task methods specific to each state, when applicable
void PAMI::task_bist ()
{
    // TO DO : all necessary self-tests
    // - In case of failure, signal it and remain stuck in BIST state
    // - In case of success, transition to IDLE state
    state = PAMI_IDLE;
}

void PAMI::task_idle ()
{
    // Default servo positions
    if (0)
    {
        pami.sx.move(RIGHT_SERVO, RIGHT_AHEAD);
        pami.sx.move(LEFT_SERVO, LEFT_AHEAD);
    }
    // TO DO : wait for all conditions necessary to transition to ARMED state
    // - Motor power must be enabled (meaning emergency stop is armed)
    // - Pin must be present
    if (io.pin == PIN_PRESENT)
    {
        state = PAMI_ARMED;
    }
}

void PAMI::task_armed ()
{
        // Default servo positions
    pami.sx.move(RIGHT_SERVO, RIGHT_AHEAD);
    pami.sx.move(LEFT_SERVO, LEFT_AHEAD);
    // TO DO : wait for the pin to get pulled, then transition to DELAY state
    if (io.pin == PIN_PULLED)
    {
        timespec_get(&pami.tzero, TIME_UTC);        // Get T-Zero for this run
        drive.motors_on ();         // turn on the motors and reset odometry
        state = PAMI_DELAY;
    }
}

void PAMI::task_delay ()
{
    // Default servo positions
    pami.sx.move(RIGHT_SERVO, RIGHT_AHEAD);
    pami.sx.move(LEFT_SERVO, LEFT_AHEAD);
    // TO DO :
    // - Wait 90 seconds and transition to RUN state
    // - If the pin is placed back on, transition immediately to ARMED state
    // - Transition immediately to "run" state if DIP switch 1 is "on" (for tests only)
    if (io.pin == PIN_PRESENT)
    {
        drive.motors_off ();
        state = PAMI_ARMED;
        return;
    }
    if (io.s1 == 1)
    {
        time = PAMI_WAIT_DELAY + 1; // Simulate time elapsed to avoid breaking time-dependent code
    }
    if (time > PAMI_WAIT_DELAY)
    {
        state = PAMI_RUN;
        // Set the initial speed of the motors
        // Initial values are unequal to compensate for issues with the motors.
        // pami.drive.speed (1440, 1440);  // 4 RPS / 240 RPM max speed
        // pami.drive.speed (1000, 1440);  // 4 RPS / 240 RPM max speed
        // pami.drive.speed (1500, 2880);  // 4 RPS / 240 RPM max speed // Used on first matches
        if (pami.id == 1)
            pami.drive.speed (2000, 2880);  // PAMI 1 values calibrated, do not change.
        if (pami.id == 2)
            pami.drive.speed (2880, 2050); // PAMI 2 values calibrated, do not change.
        if (pami.id == 3)
            pami.drive.speed (1700, 2880); // PAMI 3 values calibrated, do not change.
    }
}

// s3 == 0 for long move
//#define TIMING_A1   (((io.s3 == 0) ? 92000000 : 91000000))  // 92.0 s
//#define TIMING_A2   (((io.s3 == 0) ? 92500000 : 91500000))  // sensors off limit 92.5 s
#define TIMING_A1   (92000000)
#define TIMING_A2   (92500000)

void PAMI::task_run ()
{
    if (io.pin == PIN_PRESENT)
    {
        drive.motors_off ();
        state = PAMI_ARMED;
        return;
    }
    // TO DO : Wait until the end of match and transition the robot to IDLE state
    if (io.s1 == 1) // test mode, 90 second delay was skipped, they need to be added here
    {
        time += 90000000;   // In microseconds
    }
    if (io.s3 == 1) // Shorter move
    {
        // Delay start by one second, and ignore obstacles during that time
        if (time < 91000000)
            return;
    }
    if (time > PAMI_STOP_TIME)
    {
        drive.motors_off ();
        state = PAMI_IDLE;
    }
    if (io.pin == PIN_PRESENT)    // Run abort by replacing the pin
    {
        drive.motors_off ();
        state = PAMI_ARMED;
    }
    // New : two different behaviors based on second DIP switch:
    if (io.s2 == 0)
    {
        if ((io.tor1 == 0) || (io.tor2 == 0))   // Stop on obstacle detection
            drive.motors_off ();
        else
            drive.motors_on ();
    }
    else    // Ignore wall-side sensors, and other sensor after a certain time travelled
    {
        if (io.side == 1)   // "YELLOW" side -> ignore left side sensor (ToR 1)
        {   
            pami.sx.move(LEFT_SERVO, LEFT_DEPLOYED);
            if ((io.tor2 == 0) && (time < TIMING_A1))    // only ack the obstacle until the 92th second
            {
                drive.motors_off ();
                state = PAMI_IDLE;
            }
        }
        else                // "BLUE" side -> ignore right side sensor (ToR 2)
        {
            pami.sx.move(RIGHT_SERVO, RIGHT_DEPLOYED);
            if ((io.tor1 == 0) && (time < TIMING_A1))    // only ack the obstacle until the 92th second
            {
                drive.motors_off ();
                state = PAMI_IDLE;
            }
        }
        if (time > TIMING_A2)    // Full stop after 2.5 seconds
        {
            drive.motors_off ();
            state = PAMI_IDLE;
        }        
    }

    // Set a speed (test only)
    // pami.drive.power (200, 200); // open-loop, not good
    // pami.drive.speed (36000, 36000);    // closed-loop, 1 turn per second.
    // pami.drive.speed (200000, 200000);

    // Odometry deltas
    signed long long delta_left = pami.drive.left.position - pami.drive.left.previous;
    signed long long delta_right = pami.drive.right.position - pami.drive.right.previous;
    int delta = abs (delta_left - delta_right);


    // Attempt at differential speed regulation to keep a straight line
    unsigned short speedl = 1440;
    unsigned short speedr = 1440;
    float correction_strength = delta * 25.0;
    int correction = 5;     // 5 is a known-good value
    //if (delta > 10)
    //{
        // if (pami.drive.left.position > pami.drive.right.position)
        if (delta_left > delta_right)
        {
            speedl *= (1 + correction_strength);
            speedr *= (1 - correction_strength);
            pami.drive.left.speed -= correction;
            pami.drive.right.speed += correction;
        }
        // if (pami.drive.left.position < pami.drive.right.position)
        if (delta_left < delta_right)
        {
            speedl *= (1 - correction_strength);
            speedr *= (1 + correction_strength);
            pami.drive.left.speed += correction;
            pami.drive.right.speed -= correction;
        }
    //}
    // pami.drive.speed (speedl - 100, speedr + 200);
    // pami.drive.speed (speedr, speedl);
/*
    // Experimental avoidance code
    // if ((io.tor1 == 0) || (io.tor2 == 0))   // Stop on obstacle detection
    if (io.tor1 == 0)   // Obstacle to the left
        pami.drive.right.speed = 0;
        // pami.drive.right.speed -= 1000;
        // pami.drive.left.speed += 100;   // Accelerate left-side to avoid towards the right
    if (io.tor2 == 0)   // Obstacle to the right
        pami.drive.left.speed = 0;
        // pami.drive.left.speed -= 1000;
        // pami.drive.right.speed += 100;   // Accelerate right-side to avoid towards the left
*/
    // Combined position+speed regulation
    // pami.drive.speed (360, 360);    // 1 RPS max
    // pami.drive.speed (540, 540);    // 1.5 RPS max
    // pami.drive.speed (1440, 1440);    // 4 RPS max
    // pami.drive.speed (2880, 2880);    // 4 RPS max
    // pami.drive.move (540000, 540000);  // 15 full turns
    pami.drive.move (720000, 720000);  // 20 full turns
    // pami.drive.move (360000, 360000);  // 10 full turns
    // pami.drive.move (180000, 180000);  // 5 full turns

    // Save odometry
    pami.drive.left.previous = pami.drive.left.position;
    pami.drive.right.previous = pami.drive.right.position;

    // Run the motors
    drive.task ();
}


// Provide a printable version of the PAMI state variable (strings are padded to the same length)
void PAMI::stateString (char *str)
{
    switch (state)
    {
        case PAMI_BIST:
            sprintf (str, "BIST   ");
            break;
        case PAMI_IDLE:
            sprintf (str, "IDLE   ");
            break;
        case PAMI_ARMED:
            sprintf (str, "ARMED  ");
            break;
        case PAMI_DELAY:
            sprintf (str, "DELAY  ");
            break;
        case PAMI_RUN:
            sprintf (str, "RUNNING");
            break;
        default:
            // Perhaps add an error message, as this should never happen
            break;
    }
}


// Default constructor
PAMI::PAMI ()
{

}

// PAMI Initialization (earliest, before the state machine starts)
void PAMI::init ()
{
    state = PAMI_BIST;      // PAMI always starts by testing itself
    gethostname (hostname, 10);    // Determine the local hostname
    char last = hostname[strlen(hostname) - 1];
    switch (last)
    {
        case '1': id = 1; break;
        case '2': id = 2; break;
        case '3': id = 3; break;
        // Add a default case for when the hostname is invalid
    }
    // Get the local IP address (based on example at https://man7.org/linux/man-pages/man3/getifaddrs.3.html)
    struct ifaddrs *ifaddr;
    int family, s;
    if (getifaddrs(&ifaddr) != -1)
    {
        // success, parse
        int line = 10;
        for (struct ifaddrs *ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next)
        {
            if (strcmp(ifa->ifa_name, "wlan0") != 0) continue;   // Looking only for the WiFi address
            if (ifa->ifa_addr == NULL) continue;        // Skip unconnected interfaces
            family = ifa->ifa_addr->sa_family;          // Check the interface is indeed IPv4
            if (family == AF_INET)          // Parse to PAMI::IP
                s = getnameinfo(ifa->ifa_addr, sizeof(struct sockaddr_in), IP, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);  // returns 0 if failure
                //mvprintw (line++, 0, "IF: %s\t\taddress: <%s>\n", ifa->ifa_name, host); // ifa_name will be "wlan0" for onboard WiFi
                //refresh ();
        }
        freeifaddrs(ifaddr);
    }
    // Open the I2C bus
    char *filename = (char*)"/dev/i2c-1";
	if ((file_i2c = open(filename, O_RDWR)) < 0)
	{
		//ERROR HANDLING: you can check errno to see what went wrong
	}
    // SX1509 pin setup (must be done BEFORE calling its init function)
    // THERE IS A PROBLEM WITH THE FOLLOWING CALLS
    // sx.setup (0, SX_SERVO);     // This method takes a pin number and the desired functionality (SX_SERVO or SX_INPUT)
    // sx.mode[0] = 1;      // DEBUG CHECK
    // sx.move (0, 50);             // Servo control command : tells servo on pin 0 to go to 50% of its stroke
    // Initialize the I2C slaves
    oled.init (file_i2c);
    sx.init (file_i2c);
    // GPIO init
    io.init ();
    // Propulsion init
    drive.init();       // CAN bus initialization, socket binding
    // drive.start();      // Propulsion thread start - THREAD REPLACED BY TASK FUNCTION FOR NOW
    // Get an initial time
    timespec_get(&pami.tzero, TIME_UTC);
}

// The sequences of tasks executed by the PAMI for each state
// This is essentially the state machine. Each sequence also performs state transitions
void PAMI::tasks ()
{
    // Before the state switch, call the tasks that are executed at the beginning of all state sequences
    io.task();      // Read all Raspi inputs
    task();         // PAMI task (time management)
    sx.task();      // SX1509 (servo control / input sampling)

    // FOR TESTS ONLY - MOTOR CONTROL TASK
    // drive.task();

    switch (state)
    {
        case PAMI_BIST:
            task_bist ();
            break;
        case PAMI_IDLE:
            task_idle ();
            break;
        case PAMI_ARMED:
            task_armed ();
            break;
        case PAMI_DELAY:
            task_delay ();
            break;
        case PAMI_RUN:
            task_run ();
            break;
        default:
            // Perhaps add an error message, as this should never happen
            break;
    }
}


// ==== Main function ===============================

int main ()
{
    // Initialization
    initscr ();    // Initialize ncurses
    pami.init();    // Initialize the PAMI

    mvprintw (0, 0, "----== PAMI ==----");
    mvprintw (1, 0, "host : %s", pami.hostname);
    mvprintw (2, 0, "PAMI : %i", pami.id);
    refresh ();

// ============ DEMOs ================================================================

    // pami.sx.demo1 ();       // Servo demo 

// ===================================================================================

    // Debug only : loop down-counter
    unsigned int loopcnt = 10000;    // the main loop has a 10 ms execution time (forced for now)
    // demo only : servo angle
    unsigned int ratio = 0;     // open-loop control

    // Test only : start the motors with two different speeds
    // pami.drive.power(-100, -200);     // left, right

    // Main loop
    // while (1)
    while (pami.io.s4 == 0)     // Use DIP switch 4 to end the program
    {
        pami.tasks();

// ============= DEBUG CODE ==========================================================
        // Debug only : ensure execution ends cleanly after a few seconds
        mvprintw (3, 0, "%05i", loopcnt);
        // Display on the OLED as well
        char str[30];
        sprintf(str, "%05i", loopcnt);
        pami.oled.print(str, 0, 0);
        pami.stateString (str);
        pami.oled.print(str, 10, 0); // And let's also display the current state
        pami.oled.print(pami.IP, 0, 1); // The PAMI's IP address on wlan0 (WiFi)
        // pami.oled.refresh();
        // Display some of the inputs
        mvprintw (4, 0, "DIP 1..4 : %i-%i-%i-%i", pami.io.s1, pami.io.s2, pami.io.s3, pami.io.s4);
        mvprintw (5, 0, "Side %i", pami.io.side);
        mvprintw (6, 0, "Pin %i", pami.io.pin);
        mvprintw (7, 0, "ToR 1, 2 : %i - %i", pami.io.tor1, pami.io.tor2);
        mvprintw (8, 0, "MT INT %i", pami.io.mtint);
        // Let's show what time it is
        //timespec_get(&pami.tzero, TIME_UTC);
        //mvprintw (9, 0, "Time: %d - %d", ts.tv_sec, ts.tv_nsec);
        mvprintw (9, 0, "Time: %i", pami.time);
        // Sleep a bit
        // usleep (10000);  // 10 ms delay // No longer needed, OLED takes long enough to refresh.
        // Display some variables for debugging
        // mvprintw (10, 0, "mode[0] = %i - SX_SERVO = %i", pami.sx.mode[0], SX_SERVO);
        // Print motor positions
        // mvprintw (10, 0, " Left: %d", pami.drive.left.position);
        // mvprintw (11, 0, "Right: %d", pami.drive.right.position);
        // mvprintw (10, 0, "long: %i bytes", sizeof(long)); // Verified that a long is 4 bytes on Raspi.
        mvprintw (10, 0, "batt: %f V", (float) pami.drive.left.motor_state_1.motor_state_1.voltage / 100.0);
        mvprintw (11, 0, "speeds %i | %i", pami.drive.left.speed, pami.drive.right.speed);
        
        refresh();
        // odometry on OLED
        /*sprintf(str, "%08i", pami.drive.left.position);
        pami.oled.print(str, 0, 2);
        sprintf(str, "%08i", pami.drive.right.position);
        pami.oled.print(str, 11, 2);*/
        sprintf (str, "PAMI %i - batt %2.2f V", pami.id, (float) pami.drive.left.motor_state_1.motor_state_1.voltage / 100.0);
        pami.oled.print(str, 0, 2);
        pami.oled.refresh();
        // New servo test
        if (0)
        {
            pami.sx.move(0, ratio);
            pami.sx.move(15, ratio);
            ratio = (ratio + 1) % 100;
        }
        // if (loopcnt-- == 0) break;   // Auto-termination on time-out (safety feature during debug)
// ===================================================================================
    }



    // Note : everything past this point should never be executed,
    // as the infinite loop is infinite.

    
    pami.io.terminate ();
    close (pami.file_i2c);  // Close the I2C bus
    pami.drive.stop (); // Kill the propulsion thread, close the CAN sockets
    endwin ();    // End ncurses
    

    return 0;
}
