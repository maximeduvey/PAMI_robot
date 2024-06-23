/*  CAN Bus Servomotor Driver
    Nefastor

    This driver manages two "LK Tech" integrated servo motors which
    are the "propulsion" for the PAMI. They are controlled through the CAN bus.

*/

#define DEBUGLOG false

#include "drive.hpp"
#include <errno.h>

#include "LoggerAndDisplay.h"

DRIVE::DRIVE()
{
    printf("DRIVE::DRIVE()\n");
    mstate.store(DRIVE_STATE::DRIVE_STOPPED);
}

DRIVE::~DRIVE()
{
}

int DRIVE::init(LoggerAndDisplay *logger)
{
    if (DEBUGLOG)
        printf("DRIVE::init()\n");
    mlogger = logger;
    int retval = 0; // Nominal return value
                    // Open CAN socket
    if ((sock = socket(PF_CAN, SOCK_RAW, CAN_RAW)) < 0)
    {
        retval = -1;
    }
    else
    {
        // Find the interface index => in this case the interface name is "can0"
        strcpy(ifr.ifr_name, "can0");
        ioctl(sock, SIOCGIFINDEX, &ifr);

        memset(&addr, 0, sizeof(addr));
        addr.can_family = AF_CAN;
        addr.can_ifindex = ifr.ifr_ifindex;
        // Bind the socket
        if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0)
        {
            retval = -2;
        }
    }
    return retval;
}

void DRIVE::start() // Start the motor control thread
{
    if (DEBUGLOG)
        printf("DRIVE::start()\n");
    kill = 0;
    pthread_create(&motor_thread, NULL, motion_control_thread, this);
}

void DRIVE::stop() // Kill the propulsion thread, close the CAN sockets
{
    if (DEBUGLOG)
        printf("DRIVE::stop()\n");
    kill = 1;
    pthread_join(motor_thread, NULL); // Wait for the thread to end
}

// Simple "send command to motor" function
void DRIVE::can_send_motor(int id, unsigned char *data)
{
    if (DEBUGLOG)
        printf("DRIVE::can_send_motor(%d)\n", id);
    frame.can_id = id;
    frame.can_dlc = PAYLOAD_SIZE;
    int k;
    for (k = 0; k < PAYLOAD_SIZE; k++)
        frame.data[k] = data[k];

    // original code
    if (write(sock, &frame, sizeof(struct can_frame)) != sizeof(struct can_frame))
    {
        perror("Write");
        return;
    }

    // send(sock, &frame, sizeof(struct can_frame), MSG_DONTWAIT);
}

// Read a frame from the motors and update motor state properties of this class
// New : returns 0 if no issue, or "errno".
// New : argument should be 0 for blocking, 1 for non-blocking
int DRIVE::can_read_reply_frame(int noblock = 1)
{

    if (DEBUGLOG)
        printf("DRIVE::can_read_reply_frame()\n");
    // static int cnt = 0;
    // mlogger->log_and_display(11, 0, "%i", cnt++);
    // refresh ();

    struct can_frame f;
    /*
        if (read(sock, &f, sizeof(struct can_frame)) < 0) // Blocking read
        {
            mlogger->log_and_display(11, 0, "SOCKET ERROR");
            refresh ();
        }
    */
    // Experiment to avoid blocking read
    /*
        static int cnt = 0;
        while (1)
        {
            ssize_t sz = recv(sock, &f, sizeof(struct can_frame), MSG_DONTWAIT);
            // if (sz < 1) return;     // Nothing was received.
            if (sz > 0) break;  // something was received
            usleep (1000);  // millisecond sleep
            if (cnt++ == 100000)
            {
                mlogger->log_and_display(11, 0, "SOCKET ERROR - %i", cnt);
                refresh ();
            }
        }
    */
    // Let's do better:
    if (noblock == 1)
    {
        ssize_t sz = recv(sock, &f, sizeof(struct can_frame), MSG_DONTWAIT); // non-blocking flag
        if (sz < 0)
        {
            mlogger->log_and_display(11, 0, "ERR : %i", errno);
            // refresh ();
            return errno;
        }
    }
    else // do a blocking read
    {
        read(sock, &f, sizeof(struct can_frame));
    }

    // Process the received frame
    // Step 1 : determine which of the robot's motor structures to store the message into, based on ID
    Motor_t *m = 0;
    if (f.can_id == MOTOR_LEFT)
        m = &left;
    if (f.can_id == MOTOR_RIGHT)
        m = &right;
    // if (m == 0) return;      // Whatever message was received, wasn't something this method can parse
    // DEBUG ONLY
    /*if (m == 0)
    {
        mlogger->log_and_display(11, 0, "MESSAGE ERROR");
        refresh ();
    }*/
    // Step 2 : determine which field of that structure to store the message into, and do it
    switch (f.data[0])
    {
    case 0xA0:
    case 0xA2:
    case 0xA3:
    case 0xA4:
    case 0xA5:
    case 0xA6:
    case 0xA7:
    case 0xA8:
    case 0x9C: // open loop reply
        memcpy(m->open_loop_reply.raw, f.data, PAYLOAD_SIZE);
        break;
    case 0x30:
    case 0x31:
    case 0x32: // PID coefficients reply
        memcpy(m->pid_coefficients.raw, f.data, PAYLOAD_SIZE);
        break;
    case 0x33:
    case 0x34: // acceleration reply
        memcpy(m->acceleration.raw, f.data, PAYLOAD_SIZE);
        break;
    case 0x90: // encoder reply
        memcpy(m->encoder.raw, f.data, PAYLOAD_SIZE);
        break;
    case 0x92: // cumulative position
        memcpy(&m->position, f.data, PAYLOAD_SIZE);
        m->position = m->position >> PAYLOAD_SIZE; // The LSB is the command byte, not part of the 56-bit position
        break;
    case 0x94: // rotor angle reply
        memcpy(m->rotor_angle.raw, f.data, PAYLOAD_SIZE);
        break;
    case 0x9A:
    case 0x9B: // motor state 1
        memcpy(m->motor_state_1.raw, f.data, PAYLOAD_SIZE);
        // Voltage presents an alignment issue and must be decoded explicitly
        // unsigned short v = (m->motor_state_1.motor_state_1.voltage[1] << 8) | m->motor_state_1.motor_state_1.voltage[0];
        // m->voltage = (float) v / 10;
        break;
    case 0x9D: // motor state 3
        memcpy(m->motor_state_3.raw, f.data, PAYLOAD_SIZE);
        break;
    default: // replies that don't match anything are discarded
        break;
    }

    return 0;
}

// Combined send-receive method
// This is because the LK Tech servos REALLY don't like receiving commands before their replies are acknowledged
void DRIVE::send(int id, unsigned char *data)
{
    if (DEBUGLOG)
        printf(" DRIVE::send()\n");
    // Build the command frame, and send it
    frame.can_id = id;
    frame.can_dlc = PAYLOAD_SIZE;
    for (int k = 0; k < PAYLOAD_SIZE; k++)
        frame.data[k] = data[k];
    write(sock, &frame, sizeof(struct can_frame));
    // Get the motor's reply
    struct can_frame f;
    if (DEBUGLOG)
        printf(" DRIVE::send(A)\n");
    read(sock, &f, sizeof(struct can_frame));
    if (DEBUGLOG)
        printf(" DRIVE::send(B)\n");
    // Process the received frame
    // Step 1 - determine which of the robot's motor structures to store the message into, based on ID
    Motor_t *m = 0;
    if (f.can_id == MOTOR_LEFT)
        m = &left;
    if (f.can_id == MOTOR_RIGHT)
        m = &right;
    // Step 2 : determine which field of that structure to store the message into, and do it
    switch (f.data[0])
    {
    case 0xA0:
    case 0xA2:
    case 0xA3:
    case 0xA4:
    case 0xA5:
    case 0xA6:
    case 0xA7:
    case 0xA8:
    case 0x9C: // open loop reply
        memcpy(m->open_loop_reply.raw, f.data, PAYLOAD_SIZE);
        break;
    case 0x30:
    case 0x31:
    case 0x32: // PID coefficients reply
        memcpy(m->pid_coefficients.raw, f.data, PAYLOAD_SIZE);
        break;
    case 0x33:
    case 0x34: // acceleration reply
        memcpy(m->acceleration.raw, f.data, PAYLOAD_SIZE);
        break;
    case 0x90: // encoder reply
        memcpy(m->encoder.raw, f.data, PAYLOAD_SIZE);
        break;
    case 0x92: // cumulative position
        memcpy(&m->position, f.data, PAYLOAD_SIZE);
        m->position = m->position >> PAYLOAD_SIZE; // The LSB is the command byte, not part of the 56-bit position
        break;
    case 0x94: // rotor angle reply
        memcpy(m->rotor_angle.raw, f.data, PAYLOAD_SIZE);
        break;
    case 0x9A:
    case 0x9B: // motor state 1
        memcpy(m->motor_state_1.raw, f.data, PAYLOAD_SIZE);
        // Voltage presents an alignment issue and must be decoded explicitly
        // unsigned short v = (m->motor_state_1.motor_state_1.voltage[1] << 8) | m->motor_state_1.motor_state_1.voltage[0];
        // m->voltage = (float) v / 10;
        break;
    case 0x9D: // motor state 3
        memcpy(m->motor_state_3.raw, f.data, PAYLOAD_SIZE);
        break;
    default: // replies that don't match anything are discarded
        break;
    }
}

// Similar to the thread function, but meant to be called from the main thread, as part of the task sequencer system
void DRIVE::task()
{
    // Save odometry
    left.previous = left.position;
    right.previous = right.position;

    if (DEBUGLOG)
        printf("DRIVE::task()\n");
    MotorCmd_t motor_cmd;
    // Send commands to the motors:
    motor_cmd.multi_loop_angle_2.cmd = 0xA4;
    motor_cmd.multi_loop_angle_2.speedlimit = (unsigned short)left.speed;
    motor_cmd.multi_loop_angle_2.angle = (signed long)(LEFT_DIRECTION * left.destination);
    send(MOTOR_LEFT, motor_cmd.raw);
    for (int i = 1; i < PAYLOAD_SIZE; i++)
        motor_cmd.raw[i] = 0;
    motor_cmd.multi_loop_angle_2.cmd = 0xA4;
    motor_cmd.multi_loop_angle_2.speedlimit = (unsigned short)right.speed;
    motor_cmd.multi_loop_angle_2.angle = (signed long)(RIGHT_DIRECTION * right.destination);
    send(MOTOR_RIGHT, motor_cmd.raw);

    /*  // CLOSED LOOP - GRIP ISSUE AT MODERATE SPEEDS AND FASTER
    motor_cmd.speed_closed_loop.cmd = 0xA2;
    motor_cmd.speed_closed_loop.speed = LEFT_DIRECTION * left.speed;
    send (MOTOR_LEFT, motor_cmd.raw);
    for (int i = 1; i < 8; i++) motor_cmd.raw[i] = 0;
    motor_cmd.speed_closed_loop.cmd = 0xA2;
    motor_cmd.speed_closed_loop.speed = RIGHT_DIRECTION * right.speed;
    send (MOTOR_RIGHT, motor_cmd.raw);
    */

    /*  // OPEN LOOP - IMPRECISE
    motor_cmd.open_loop.cmd = 0xA0;
    motor_cmd.open_loop.power = LEFT_DIRECTION * left.power;   // Update the power setting and prepare to send
    send (MOTOR_LEFT, motor_cmd.raw);
    for (int i = 1; i < 8; i++) motor_cmd.raw[i] = 0;   // Clear the frame buffer, just in case the motors don't like non-zero data in unused fields. (starting at 1 because 0 is the command, there's always going to be something in there)
    motor_cmd.open_loop.cmd = 0xA0;
    motor_cmd.open_loop.power = RIGHT_DIRECTION * right.power;   // Update the power setting and prepare to send
    send (MOTOR_RIGHT, motor_cmd.raw);
    */

    // Read motor status and odometry
    for (int i = 1; i < PAYLOAD_SIZE; i++)
        motor_cmd.raw[i] = 0;
    motor_cmd.no_param.cmd = 0x9A; // Read status 1
    send(MOTOR_LEFT, motor_cmd.raw);
    send(MOTOR_RIGHT, motor_cmd.raw);

    for (int i = 1; i < PAYLOAD_SIZE; i++)
        motor_cmd.raw[i] = 0;
    motor_cmd.no_param.cmd = 0x92; // Read cumulative position
    send(MOTOR_LEFT, motor_cmd.raw);
    send(MOTOR_RIGHT, motor_cmd.raw);
    left.position = LEFT_DIRECTION * left.position;
    right.position = RIGHT_DIRECTION * right.position;
}

void DRIVE::motors_on() // Motor init function, to be called only once, on transition from "armed" to "delay". Resets odometry.
{
    if (DEBUGLOG)
        printf("DRIVE::motors_on(%d)\n", mstate.load());
    if (mstate.load() != DRIVE_RUNNING)
    {
        mstate.store(DRIVE_RUNNING);
        MotorCmd_t motor_cmd;
        // Turn on the motors ("motor on", 0x88) then reset odometry (0x95)
        motor_cmd.no_param.cmd = 0x88; // The "no_param" union member is for motor commands that take no parameters, such as turning a motor on
        send(MOTOR_LEFT, motor_cmd.raw);
        send(MOTOR_RIGHT, motor_cmd.raw);
        motor_cmd.no_param.cmd = 0x95; // The "no_param" union member is for motor commands that take no parameters, such as turning a motor on
        send(MOTOR_LEFT, motor_cmd.raw);
        send(MOTOR_RIGHT, motor_cmd.raw);
    }
    if (DEBUGLOG)
        printf("DRIVE::motors_on end (%d)\n", mstate.load());
}

void DRIVE::motors_off() // Motor off function, to be called when transitioning towards the idle state
{
    if (DEBUGLOG)
        printf("DRIVE::motors_off(%d)\n", mstate.load());
    if (mstate.load() == DRIVE_RUNNING)
    {
        mstate.store(DRIVE_STOPPED);
        // Send "motor stop" (0x81), then "motor off" (0x81)
        MotorCmd_t motor_cmd;
        // Turn on the motors
        motor_cmd.no_param.cmd = 0x81; // The "no_param" union member is for motor commands that take no parameters, such as turning a motor on
        send(MOTOR_LEFT, motor_cmd.raw);
        send(MOTOR_RIGHT, motor_cmd.raw);
        motor_cmd.no_param.cmd = 0x80; // The "no_param" union member is for motor commands that take no parameters, such as turning a motor on
        send(MOTOR_LEFT, motor_cmd.raw);
        send(MOTOR_RIGHT, motor_cmd.raw);
    }
    if (DEBUGLOG)
        printf("DRIVE::motors_off end (%d)\n", mstate.load());
}

// Sends open-loop power commands to both motors
void DRIVE::power(signed short l, signed short r) // note : motor #4 (right) and #3 (left)
{
    if (DEBUGLOG)
        printf("DRIVE::power(%d***%d)\n", l, r);
    left.power = l;
    right.power = r;
}

// Sends closed-loop speed commands to both motors (left, right)
void DRIVE::speed(signed long l, signed long r)
{
    if (DEBUGLOG)
        printf("DRIVE::speed(%lu***%lu)\n", l, r);
    left.speed = l;
    right.speed = r;
}

// Sends the motors to specific positions expressed in odometry units
void DRIVE::move(long l, long r)
{
    if (DEBUGLOG)
        printf("DRIVE::move(%ld***%ld)\n", l, r);
    left.destination = l;
    right.destination = r;
}

// ************ THE GRAVEYARD OF USELESS OLD CODE *******************************************************

/* New version of the CAN bus thread, meant to be more robust.
 */
void *DRIVE::motion_control_thread(void *arg)
{
    // DEBUG ONLY
    int loopcnt = 0;
    // Cast argument to get a pointer to the thread's parent object
    DRIVE *drive = (DRIVE *)arg;
    // State change detection keepers
    signed short power_left = 0;
    signed short power_right = 0;
    // Motor initialization should be moved outside of the thread
    // Turn on the motors
    MotorCmd_t motor_cmd;
    motor_cmd.no_param.cmd = 0x88; // The "no_param" union member is for motor commands that take no parameters, such as turning a motor on
    drive->send(MOTOR_LEFT, motor_cmd.raw);
    drive->send(MOTOR_RIGHT, motor_cmd.raw);
    // Go straight to infinite loop:
    while (drive->kill == 0)
    {
        // mlogger->log_and_display(13, 0, "%i", loopcnt++);
        //  Limit execution rate to something CAN-friendly, say 100 Hz, by using a 10 ms pause
        usleep(10000); // changed to blocking socket API, this shouldn't be necessary

        // Phase one: take-in any CAN messages received on the socket
        // while (drive->can_read_reply_frame() == 0);  // Read buffered ingress frames until there's none pending

        // Phase two: send commands to the motors:
        // Did the power commands change ? If so, send new commands to the motors
        if (drive->left.power != power_left)
        {
            for (int i = 1; i < PAYLOAD_SIZE; i++)
                motor_cmd.raw[i] = 0; // Clear the frame buffer, just in case the motors don't like non-zero data in unused fields. (starting at 1 because 0 is the command, there's always going to be something in there)
            motor_cmd.open_loop.cmd = 0xA0;
            motor_cmd.open_loop.power = power_left = drive->left.power; // Update the power setting and prepare to send
            drive->send(MOTOR_LEFT, motor_cmd.raw);
        }
        if (drive->right.power != power_right)
        {
            for (int i = 1; i < PAYLOAD_SIZE; i++)
                motor_cmd.raw[i] = 0;
            motor_cmd.open_loop.cmd = 0xA0;
            motor_cmd.open_loop.power = power_right = drive->right.power; // Update the power setting and prepare to send
            drive->send(MOTOR_RIGHT, motor_cmd.raw);
        }
        // Read motor status
        for (int i = 1; i < PAYLOAD_SIZE; i++)
            motor_cmd.raw[i] = 0;
        motor_cmd.no_param.cmd = 0x92; // Read cumulative position
        drive->send(MOTOR_LEFT, motor_cmd.raw);
        drive->send(MOTOR_RIGHT, motor_cmd.raw);
    }
    // Motor deinitialization should be moved outside of the thread:
    // Clean-up before termination : turn off the motors, etc...
    // Note that I don't clear previous CAN message payloads. I don't know if the motors care
    motor_cmd.no_param.cmd = 0x81;         // The "no_param" union member is for motor commands that take no parameters, such as turning a motor on
    for (int i = 1; i < PAYLOAD_SIZE; i++) // (starting at 1 because 0 is the command, there's always going to be something in there)
        motor_cmd.raw[i] = 0;              //  Clean the rest of the payload, just in case.
    drive->send(MOTOR_LEFT, motor_cmd.raw);
    drive->send(MOTOR_RIGHT, motor_cmd.raw);
    return NULL;
}

// STATUS - PARTIAL AND VALIDATED. MISSING: sending commands to the motors over CAN bus (requires speed values from the remote)
// Runs on the robot, waits for and processes UDP packets from the remote control
// IMPORTANT - YOU MUST READ THE REPLY FRAME SENT BACK BY THE MOTORS AFTER EACH COMMAND, EVEN IF YOU DON'T USE THE REPLY
void *DRIVE::motion_control_thread_old(void *arg)
{
    if (DEBUGLOG)
        printf("DRIVE::motion_control_thread_old()");
    DRIVE *drive = (DRIVE *)arg; // Cast argument to get a pointer to the thread's parent object

    // State change detection keepers
    signed short power_left = 0;
    signed short power_right = 0;

    // Turn on the motors
    MotorCmd_t motor_cmd;
    motor_cmd.no_param.cmd = 0x88; // The "no_param" union member is for motor commands that take no parameters, such as turning a motor on
    drive->can_send_motor(MOTOR_LEFT, motor_cmd.raw);
    drive->can_send_motor(MOTOR_RIGHT, motor_cmd.raw);
    drive->can_read_reply_frame();
    drive->can_read_reply_frame();

    static int cnt = 0;
    while (drive->kill == 0)
    {

        // mlogger->log_and_display(11, 0, "%i", cnt++);

        // TO DO : add support for a command to reset cumulative position counters in the motors

        // Did the power commands change ? If so, send new commands to the motors
        if (drive->left.power != power_left)
        {
            motor_cmd.open_loop.cmd = 0xA0;
            motor_cmd.open_loop.power = power_left = drive->left.power; // Update the power setting and prepare to send
            drive->can_send_motor(MOTOR_LEFT, motor_cmd.raw);
            drive->can_read_reply_frame();
        }
        if (drive->right.power != power_right)
        {
            motor_cmd.open_loop.cmd = 0xA0;
            motor_cmd.open_loop.power = power_right = drive->right.power; // Update the power setting and prepare to send
            drive->can_send_motor(MOTOR_RIGHT, motor_cmd.raw);
            drive->can_read_reply_frame();
        }

        // Read motor status
        motor_cmd.no_param.cmd = 0x92; // Read cumulative position
        drive->can_send_motor(MOTOR_LEFT, motor_cmd.raw);
        drive->can_send_motor(MOTOR_RIGHT, motor_cmd.raw);
        drive->can_read_reply_frame();
        drive->can_read_reply_frame();
        motor_cmd.no_param.cmd = 0x9A; // Read status 1
        drive->can_send_motor(MOTOR_LEFT, motor_cmd.raw);
        drive->can_send_motor(MOTOR_RIGHT, motor_cmd.raw);
        drive->can_read_reply_frame();
        drive->can_read_reply_frame();
    }

    // Clean-up before termination : turn off the motors, etc...
    // Note that I don't clear previous CAN message payloads. I don't know if the motors care
    motor_cmd.no_param.cmd = 0x81; // The "no_param" union member is for motor commands that take no parameters, such as turning a motor on
    for (int i = 1; i < PAYLOAD_SIZE; i++)
        motor_cmd.raw[i] = 0; //  Clean the rest of the payload, just in case.
    drive->can_send_motor(MOTOR_LEFT, motor_cmd.raw);
    drive->can_send_motor(MOTOR_RIGHT, motor_cmd.raw);
    drive->can_read_reply_frame();
    drive->can_read_reply_frame();

    /*


        while (1)
        {
            // Receive packets into global variable "s"
            recvfrom(sock_in, &s, sizeof(s), 0, (struct sockaddr *)&sock_addr, &len);
            // Print the received stick data
            // printf ("\n%3i - %3i | C %i - Z %i | %i - %i", s.x, s.y, s.c, s.z, s.m1.open_loop.power, s.m2.open_loop.power);
            // Command the motors
            can_send_motor (0x141, s.m1.raw);
            can_send_motor (0x142, s.m2.raw);
            // Read the response messages into the state structure
            can_read_reply_frame();
            can_read_reply_frame();
            // Additional CAN commands for telemetry

    /*        motor_cmd.no_param.cmd = 0x90;  // Read the encoders (the values are redundant with other messages)
            can_send_motor (0x141, motor_cmd.raw);
            can_send_motor (0x142, motor_cmd.raw);
            can_read_reply_frame();
            can_read_reply_frame();     */
    /*        motor_cmd.no_param.cmd = 0x94;  // Read the rotor angles
            can_send_motor (0x141, motor_cmd.raw);
            can_send_motor (0x142, motor_cmd.raw);
            can_read_reply_frame();
            can_read_reply_frame();
            motor_cmd.no_param.cmd = 0x9A;  // Read status 1
            can_send_motor (0x141, motor_cmd.raw);
            can_send_motor (0x142, motor_cmd.raw);
            can_read_reply_frame();
            can_read_reply_frame();
            //motor_cmd.no_param.cmd = 0x9D;  // Read status 3 - DOES NOT YIELD REPLIES !
            //can_send_motor (0x141, motor_cmd.raw);
            //can_send_motor (0x142, motor_cmd.raw);
            //can_read_reply_frame();
            //can_read_reply_frame();

            // Display the parsed replies
    /*        printf ("\nTemp %i|%i - Speed %i|%i",
                r.m1.open_loop_reply.open_loop_reply.temp,
                r.m2.open_loop_reply.open_loop_reply.temp,
                r.m1.open_loop_reply.open_loop_reply.speed,
                r.m2.open_loop_reply.open_loop_reply.speed);*/
    // Send the state structure to the remote control over UDP
    /*        sendto (sock_out, &r, sizeof(r), 0, (struct sockaddr *) &sock_addr2, sizeof(sock_addr2));
            // Exit test
            if (s.c > 150)
                break;
            // Angle measurements reset on long press on trigger Z
            if (s.z > 150)
            {
                odometry_reset ();
                motor_cmd.no_param.cmd = 0x95;  // Reset angle measurements
                can_send_motor (0x141, motor_cmd.raw);
                can_send_motor (0x142, motor_cmd.raw);
                can_read_reply_frame();
                can_read_reply_frame();
            }
        }

        // TO DO : stop the motors

    */
    return NULL;
}

DRIVE::DRIVE_STATE DRIVE::getDriveState() const
{
    return mstate.load();
}

void DRIVE::printfDriveInfos()
{
    printf("DRIVE::printfDriveInfos() left-position:%lld, right-position:%lld \n",
     left.position, right.position);
}
