
#include "PAMI.h"
#include "Drivers/common_includes.h"

// Default constructor
PAMI::PAMI ()
{

}
PAMI::~PAMI()
{
    
}

// Task method of the PAMI object (essentially, time management)
// Call after the robot's inputs have been sampled
void PAMI::task ()
{
    struct timespec ts;
    timespec_get(&ts, TIME_UTC);        // Get the current time
    unsigned long seconds = (ts.tv_sec - tzero.tv_sec) * 1000000;  // expressed in microseconds
    unsigned long nanos = (ts.tv_nsec - tzero.tv_nsec) / 1000;     // expressed in microseconds
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
        sx.move(RIGHT_SERVO, RIGHT_AHEAD);
        sx.move(LEFT_SERVO, LEFT_AHEAD);
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
    sx.move(RIGHT_SERVO, RIGHT_AHEAD);
    sx.move(LEFT_SERVO, LEFT_AHEAD);
    // TO DO : wait for the pin to get pulled, then transition to DELAY state
    if (io.pin == PIN_PULLED)
    {
        timespec_get(&tzero, TIME_UTC);        // Get T-Zero for this run
        drive.motors_on ();         // turn on the motors and reset odometry
        state = PAMI_DELAY;
    }
}

void PAMI::task_delay ()
{
    // Default servo positions
    sx.move(RIGHT_SERVO, RIGHT_AHEAD);
    sx.move(LEFT_SERVO, LEFT_AHEAD);
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
        // drive.speed (1440, 1440);  // 4 RPS / 240 RPM max speed
        // drive.speed (1000, 1440);  // 4 RPS / 240 RPM max speed
        // drive.speed (1500, 2880);  // 4 RPS / 240 RPM max speed // Used on first matches
        if (id == 1)
            drive.speed (2000, 2880);  // PAMI 1 values calibrated, do not change.
        if (id == 2)
            drive.speed (2880, 2050); // PAMI 2 values calibrated, do not change.
        if (id == 3)
            drive.speed (1700, 2880); // PAMI 3 values calibrated, do not change.
    }
}

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
            sx.move(LEFT_SERVO, LEFT_DEPLOYED);
            if ((io.tor2 == 0) && (time < TIMING_A1))    // only ack the obstacle until the 92th second
            {
                drive.motors_off ();
                state = PAMI_IDLE;
            }
        }
        else                // "BLUE" side -> ignore right side sensor (ToR 2)
        {
            sx.move(RIGHT_SERVO, RIGHT_DEPLOYED);
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
    // drive.power (200, 200); // open-loop, not good
    // drive.speed (36000, 36000);    // closed-loop, 1 turn per second.
    // drive.speed (200000, 200000);

    // Odometry deltas
    signed long long delta_left = drive.left.position - drive.left.previous;
    signed long long delta_right = drive.right.position - drive.right.previous;
    int delta = abs (delta_left - delta_right);


    // Attempt at differential speed regulation to keep a straight line
    unsigned short speedl = 1440;
    unsigned short speedr = 1440;
    float correction_strength = delta * 25.0;
    int correction = 5;     // 5 is a known-good value
    //if (delta > 10)
    //{
        // if (drive.left.position > drive.right.position)
        if (delta_left > delta_right)
        {
            speedl *= (1 + correction_strength);
            speedr *= (1 - correction_strength);
            drive.left.speed -= correction;
            drive.right.speed += correction;
        }
        // if (drive.left.position < drive.right.position)
        if (delta_left < delta_right)
        {
            speedl *= (1 - correction_strength);
            speedr *= (1 + correction_strength);
            drive.left.speed += correction;
            drive.right.speed -= correction;
        }
    //}
    // drive.speed (speedl - 100, speedr + 200);
    // drive.speed (speedr, speedl);
/*
    // Experimental avoidance code
    // if ((io.tor1 == 0) || (io.tor2 == 0))   // Stop on obstacle detection
    if (io.tor1 == 0)   // Obstacle to the left
        drive.right.speed = 0;
        // drive.right.speed -= 1000;
        // drive.left.speed += 100;   // Accelerate left-side to avoid towards the right
    if (io.tor2 == 0)   // Obstacle to the right
        drive.left.speed = 0;
        // drive.left.speed -= 1000;
        // drive.right.speed += 100;   // Accelerate right-side to avoid towards the left
*/
    // Combined position+speed regulation
    // drive.speed (360, 360);    // 1 RPS max
    // drive.speed (540, 540);    // 1.5 RPS max
    // drive.speed (1440, 1440);    // 4 RPS max
    // drive.speed (2880, 2880);    // 4 RPS max
    // drive.move (540000, 540000);  // 15 full turns
    drive.move (720000, 720000);  // 20 full turns
    // drive.move (360000, 360000);  // 10 full turns
    // drive.move (180000, 180000);  // 5 full turns

    // Save odometry
    drive.left.previous = drive.left.position;
    drive.right.previous = drive.right.position;

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
    timespec_get(&tzero, TIME_UTC);
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

