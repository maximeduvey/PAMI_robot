
#include "PAMI.h"
#include "Drivers/common_includes.h"

#include "LoggerAndDisplay.h"
#include "MovementAction.h"
#include <thread>

#include "sharedInfos.h"

// Default constructor
PAMI::PAMI()
{
    mDControl.setDrive(&drive);
}
PAMI::~PAMI() {}

// Task method of the PAMI object (essentially, time management)
// Call after the robot's inputs have been sampled
void PAMI::handle_time()
{
    struct timespec ts;
    timespec_get(&ts, TIME_UTC);                                  // Get the current time
    unsigned long seconds = (ts.tv_sec - tzero.tv_sec) * 1000000; // expressed in microseconds
    unsigned long nanos = (ts.tv_nsec - tzero.tv_nsec) / 1000;    // expressed in microseconds
    time = seconds + nanos;
}

// Task methods specific to each state, when applicable
void PAMI::task_bist()
{
    // TO DO : all necessary self-tests
    // - In case of failure, signal it and remain stuck in BIST state
    // - In case of success, transition to IDLE state
    pState = PAMI_IDLE;
}

void PAMI::task_idle()
{
    // TO DO : wait for all conditions necessary to transition to ARMED state
    // - Motor power must be enabled (meaning emergency stop is armed)
    // - Pin must be present
    if (io.pin == PIN_PRESENT)
    {
        pState = PAMI_ARMED;
        iniStrat_brainDeadForward();
    }
}

void PAMI::task_armed()
{
    // Default servo positions
    /*     sx.move(RIGHT_SERVO, RIGHT_AHEAD);
        sx.move(LEFT_SERVO, LEFT_AHEAD); */
    // TO DO : wait for the pin to get pulled, then transition to DELAY state
    // printf("PAMI::task_armed()\n");
    if (io.pin == PIN_PULLED)
    {
        printf("PAMI::task_armed(arm to delay) \n");
        timespec_get(&tzero, TIME_UTC); // Get T-Zero for this run
        drive.motors_on();              // turn on the motors and reset odometry
        pState = PAMI_DELAY;
        // mDControl.goForward(10000);
    }
}

void PAMI::task_delay()
{
    // Default servo positions
    /*     sx.move(RIGHT_SERVO, RIGHT_AHEAD);
        sx.move(LEFT_SERVO, LEFT_AHEAD); */
    // TO DO :
    // - Wait 90 seconds and transition to RUN state
    // - If the pin is placed back on, transition immediately to ARMED state
    // - Transition immediately to "run" state if DIP switch 1 is "on" (for tests only)
    if (io.pin == PIN_PRESENT)
    {
        printf("PAMI::task_delay(B)\n");
        drive.motors_off();
        pState = PAMI_ARMED;
        return;
    }
    if (io.s1 == 1)
    {
        time = PAMI_WAIT_DELAY + 1; // Simulate time elapsed to avoid breaking time-dependent code
    }
    if (time > PAMI_WAIT_DELAY)
    {
        pState = PAMI_RUN;
    }
}

void PAMI::task_run()
{
    bool debug = true;
    if (io.pin == PIN_PRESENT)
    {
        if (debug)
            printf("PAMI::task_run(io.pin)\n");
        drive.motors_off();
        pState = PAMI_ARMED;
        return;
    }
    drive.printfDriveInfos();

    // mDControl.doCalibrationOnMotorSpeed();
    mDControl.startRunning();
    simpleDetectCollission();

    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    return;
}

// Provide a printable version of the PAMI state variable (strings are padded to the same length)
void PAMI::stateString(char *str)
{
    switch (pState)
    {
    case PAMI_BIST:
        sprintf(str, "BIST   ");
        break;
    case PAMI_IDLE:
        sprintf(str, "IDLE   ");
        break;
    case PAMI_ARMED:
        sprintf(str, "ARMED  ");
        break;
    case PAMI_DELAY:
        sprintf(str, "DELAY  ");
        break;
    case PAMI_RUN:
        sprintf(str, "RUNNING");
        break;
    default:
        // Perhaps add an error message, as this should never happen
        break;
    }
}

// PAMI Initialization (earliest, before the state machine starts)
void PAMI::init(LoggerAndDisplay *logger)
{
    mlogger = logger;
    pState = PAMI_BIST;                   // PAMI always starts by testing itself
    gethostname(hostname, HOST_NAME_LEN); // Determine the local hostname
    char last = hostname[strlen(hostname) - 1];
    switch (last)
    {
    case '1':
        PAMI_ID = 1;
        id = 1;
        break;
    case '2':
        PAMI_ID = 2;
        id = 2;
        break;
    case '3':
        PAMI_ID = 3;
        id = 3;
        break;
        // Add a default case for when the hostname is invalid
    }
    mDControl.setId(id);
    printf("PAMI::init(%d)\n", mDControl.mId);
    // Get the local IP address (based on example at https://man7.org/linux/man-pages/man3/getifaddrs.3.html)
    struct ifaddrs *ifaddr;
    int family, s;
    if (getifaddrs(&ifaddr) != -1)
    {
        // success, parse
        int line = 10;
        for (struct ifaddrs *ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next)
        {
            if (strcmp(ifa->ifa_name, "wlan0") != 0)
                continue; // Looking only for the WiFi address
            if (ifa->ifa_addr == NULL)
                continue;                                                                                            // Skip unconnected interfaces
            family = ifa->ifa_addr->sa_family;                                                                       // Check the interface is indeed IPv4
            if (family == AF_INET)                                                                                   // Parse to PAMI::IP
                s = getnameinfo(ifa->ifa_addr, sizeof(struct sockaddr_in), IP, NI_MAXHOST, NULL, 0, NI_NUMERICHOST); // returns 0 if failure
                                                                                                                     // mlogger->log_and_display(line++, 0, "IF: %s\t\taddress: <%s>\n", ifa->ifa_name, host); // ifa_name will be "wlan0" for onboard WiFi
            // refresh ();
        }
        freeifaddrs(ifaddr);
    }
    // Open the I2C bus
    char *filename = (char *)"/dev/i2c-1";
    if ((file_i2c = open(filename, O_RDWR)) < 0)
    {
        // ERROR HANDLING: you can check errno to see what went wrong
    }
    // SX1509 pin setup (must be done BEFORE calling its init function)
    // THERE IS A PROBLEM WITH THE FOLLOWING CALLS
    // sx.setup (0, SX_SERVO);     // This method takes a pin number and the desired functionality (SX_SERVO or SX_INPUT)
    // sx.mode[0] = 1;      // DEBUG CHECK
    // sx.move (0, 50);             // Servo control command : tells servo on pin 0 to go to 50% of its stroke
    // Initialize the I2C slaves
    oled.init(file_i2c, mlogger);
    sx.init(file_i2c, mlogger);
    // GPIO init
    io.init(mlogger);
    // Propulsion init
    drive.init(mlogger); // CAN bus initialization, socket binding
    // drive.start();      // Propulsion thread start - THREAD REPLACED BY TASK FUNCTION FOR NOW
    // Get an initial time
    timespec_get(&tzero, TIME_UTC);
}

// The sequences of tasks executed by the PAMI for each state
// This is essentially the state machine. Each sequence also performs state transitions
void PAMI::tasks()
{
    // Before the state switch, call the tasks that are executed at the beginning of all state sequences
    io.task();     // Read all Raspi inputs
    handle_time(); // PAMI task (time management)
    sx.task();     // SX1509 (servo control / input sampling)

    // FOR TESTS ONLY - MOTOR CONTROL TASK
    // drive.task();

    switch (pState)
    {
    case PAMI_BIST:
        task_bist();
        break;
    case PAMI_IDLE:
        task_idle();
        break;
    case PAMI_ARMED:
        task_armed();
        break;
    case PAMI_DELAY:
        task_delay();
        break;
    case PAMI_RUN:
        task_run();
        break;
    default:
        // Perhaps add an error message, as this should never happen
        break;
    }
}

void PAMI::iniStrat_brainDeadForward()
{
    printf("PAMI::iniStrat_brainDeadForward()\n");
    mDControl.clearAction();
    // mDControl.addAction(MovementAction::createActionTurn90Right());

    mDControl.addAction(MovementAction::createActionGoForward(10));
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    mDControl.addAction(MovementAction::createActionTurn90Right());
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    mDControl.addAction(MovementAction::createActionTurn90Left());
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    mDControl.addAction(MovementAction::createActionTurn90Left());

    // if (PAMI_ID == 2)
    // {
    //     printf("PAMI::iniStrat_brainDeadForward(PAMI 1)\n");
    //     mDControl.addAction(MovementAction::createActionGoForward(120));
    // }
    // else if (PAMI_ID == 3)
    // {
    //     printf("PAMI::iniStrat_brainDeadForward(PAMI 2)\n");
    //     mDControl.addAction(MovementAction::createActionGoForward(60));
    //     mDControl.addAction(MovementAction::createActionTurn90Right());
    //     mDControl.addAction(MovementAction::createActionGoForward(140));
    // }
    // else if (PAMI_ID == 1)
    // {
    //     printf("PAMI::iniStrat_brainDeadForward(PAMI 3)\n");
    //     mDControl.addAction(MovementAction::createActionGoForward(130));
    //     mDControl.addAction(MovementAction::createActionTurn90Left());
    //     mDControl.addAction(MovementAction::createActionGoForward(160));
    // }
}

void PAMI::simpleDetectCollission()
{
    // printf("PAMI::simpleDetectCollission() L:%d, R:%d\n",io.mProximitysensorLeft, io.mProximitysensorRight);
    if (io.mProximitysensorLeft != 1 || io.mProximitysensorRight != 1)
    {
        interuptMovement();
    }
    else if (mDControl.isStopped())
    {
        mDControl.resume();
    }
}

void PAMI::interuptMovement()
{
    mDControl.stop();
}