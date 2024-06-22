/*  CAN Bus Servomotor Driver
    Nefastor

    This driver manages two "LK Tech" integrated servo motors which
    are the "propulsion" for the PAMI. They are controlled through the CAN bus.

*/

#ifndef __PAMI_DRIVE_HPP__
#define __PAMI_DRIVE_HPP__

#include "common_includes.h"
#include <atomic>

// ======= Constants =======================================================================

// Swapping these ID's changes which side each motor is on
#define MOTOR_LEFT (0x142)
#define MOTOR_RIGHT (0x141)

// Motor reversal selection: set the first macro to either 1 or -1:
#define LEFT_DIRECTION (1)
#define RIGHT_DIRECTION (-(LEFT_DIRECTION))

#define PAYLOAD_SIZE 8

// ======= Motor Structures ================================================================

typedef union MotorCmd_u
{
    unsigned char raw[PAYLOAD_SIZE];   // 8-byte payload of a CAN message to one or both motors
    // For 0x80 "motor off" / 0x88 "motor on" / 0x81 "motor stop" /
    //  0x30 "read PID parameters" / 0x33 "read acceleration" / 0x90 "read encoder" /
    //  0x19 "write current position to ROM as the motor zero point" /
    //  0x92 "read multi angle loop" / 0x94 "read single angle loop" / 0x95 "clear rotor angle" /
    //  0x9A "read motor state 1" / 0x9C "read motor state 2" / 0x9D "read motor state 3" /
    //  0x9B "clear motor state"
    struct no_param_s     // Usable for all commands with no payload : on / off / stop etc...
    {
        unsigned char cmd;          // command byte
        unsigned char pad1[7];      // empty bytes
    } no_param;
    // For 0xA0 "open loop control command"
    struct open_loop_s
    {
        unsigned char cmd;          // command byte - must be 0xA0
        unsigned char pad1[3];      // empty bytes
        signed short power;         // range : -850 to +850
        unsigned char pad2[2];      // empty bytes
    } open_loop;
    // Reply message for 0xA0, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7, 0xA8, 0x9C
    struct open_loop_reply_s        // Reply to the "open_loop" command
    {
        signed char cmd;            // command byte - will be 0xA0
        signed char temp;           // temperature (1°C/LSB)
        signed short torque;
        signed short speed;         // angular speed (1°/s/LSB)
        unsigned short encoder;     // encoder position
    } open_loop_reply;
    // For 0xA2 "speed closed loop control command" - Reply format is "open_loop_reply"
    struct speed_closed_loop_s
    {
        unsigned char cmd;          // command byte - must be 0xA2
        unsigned char pad1[3];      // empty bytes
        signed long speed;          // speed (0.01°/s/LSB)
    } speed_closed_loop;
    // For 0xA3 "multi loop angle control command 1" - Reply format is "open_loop_reply"
    struct multi_loop_angle_1_s
    {
        unsigned char cmd;          // command byte - must be 0xA3
        unsigned char pad1[3];      // empty bytes
        signed long angle;          // angle (0.01°/LSB)   
    } multi_loop_angle_1;
    // For 0xA4 "multi loop angle control command 2" - Reply format is "open_loop_reply"
    struct multi_loop_angle_2_s
    {
        unsigned char cmd;          // command byte - must be 0xA4
        unsigned char pad1;         // empty byte
        unsigned short speedlimit;  // speed limit (1°/LSB)
        signed long angle;          // angle (0.01°/LSB)   
    } multi_loop_angle_2;
    // For 0xA5 "single loop angle control 1" - Reply format is "open_loop_reply"
    struct single_loop_angle_1_s
    {
        unsigned char cmd;          // command byte - must be 0xA5
        unsigned char dir;          // spin direction (0 for CW, 1 for CCW)
        unsigned char pad1[2];      // empty bytes
        unsigned long angle;        // angle (0.01°/LSB) UNSIGNED !
    } single_loop_angle_1;
    // For 0xA6 "single loop angle control 2" - Reply format is "open_loop_reply"
    struct single_loop_angle_2_s
    {
        unsigned char cmd;          // command byte - must be 0xA6
        unsigned char dir;          // spin direction (0 for CW, 1 for CCW)
        unsigned short speedlimit;  // speed limit (1°/LSB)
        unsigned long angle;        // angle (0.01°/LSB) UNSIGNED !
    } single_loop_angle_2;
    // For 0xA7 "incremental angle control command 1" - Reply format is "open_loop_reply"
    struct incremental_angle_1_s
    {
        unsigned char cmd;          // command byte - must be 0xA7
        unsigned char pad1[3];      // empty bytes
        signed long angle;          // angle (0.01°/LSB)   
    } incremental_angle_1;
    // For 0xA8 "incremental angle control command 2" - Reply format is "open_loop_reply"
    struct incremental_angle_2_s
    {
        unsigned char cmd;          // command byte - must be 0xA8
        unsigned char pad1;         // empty byte
        unsigned short speedlimit;  // speed limit (1°/LSB)
        signed long angle;          // angle (0.01°/LSB)   
    } incremental_angle_2;
    // Reply message for 0x30 "read PID parameters", 0x31, 0x32
    // Also for commands 0x31 "write PID parameters to RAM" and 0x32 (same, to ROM)
    struct pid_coefficients_s
    {
        unsigned char cmd;          // command byte - 0x30 / 0x31 / 0x32
        unsigned char pad1;         // empty byte
        unsigned char position_P;   // position loop P coefficient
        unsigned char position_I;   // position loop I coefficient
        unsigned char speed_P;      // speed loop P coefficient
        unsigned char speed_I;      // speed loop I coefficient
        unsigned char torque_P;     // torque loop P coefficient
        unsigned char torque_I;     // torque loop I coefficient
    } pid_coefficients;
    // Reply message for 0x33 "read acceleration" and 0x34
    // Also for command 0x34 "write acceleration to RAM"
    struct acceleration_s
    {
        unsigned char cmd;          // command byte - 0x33 / 0x34
        unsigned char pad1[3];      // empty bytes
        signed long acceleration;   // acceleration (1°/s²/LSB)
    } acceleration;
    // Reply message for 0x90 "read encoder"
    struct encoder_s
    {
        unsigned char cmd;          // command byte - 0x90
        unsigned char pad1;         // empty byte
        unsigned short encoder;     // raw encoder value + offset, 14 bits
        unsigned short raw;         // raw encoder value, 14 bits
        unsigned short offset;      // encoder "zero" position, 14 bits
    } encoder;
    // For 0x91 "write encoder value to ROM as the motor zero point" (sets the encoder offset)
    // The reply message is identical. It's also the reply format for command 0x19
    struct encoder_offset_s
    {
        unsigned char cmd;          // command byte - 0x91 or 0x19
        unsigned char pad1[5];      // empty bytes
        unsigned short offset;      // encoder "zero" position, 14 bits
    } encoder_offset;
    // Reply message for 0x92 "read multi angle loop"
    //struct position_s
    //{
        //unsigned char cmd;          // command byte - 0x92
        // signed long long position;  // 56-bit accumulated rotor angle
    //} position;
    // 56-bit accumulated rotor angle. Shift right 8 bits to get rid of 0x92 command byte
    signed long long position;
    // Reply message for 0x94 "read single angle loop"
    struct rotor_angle_s
    {
        unsigned char cmd;          // command byte - 0x94
        unsigned char pad1[3];      // empty bytes
        unsigned long angle;        // rotor angle (0.01°/LSB), range 0..35999
    } rotor_angle;
    // Reply message for 0x9A "read motor state 1" (and also for 0x9B)
    struct motor_state_1_s
    {
        unsigned char cmd;          // command byte - 0x9A      (VERIFIED)
        signed char temp;           // temperature (1°C/LSB)    (VERIFIED)
        unsigned short voltage;     // motor voltage (0.1V/LSB) (VERIFIED)
        unsigned char pad1[4];      // empty bytes              (NOT VERIFIED !!!)
        unsigned char error;        // error flags (NOT VERIFIED !!!) (bit 0 "undervoltage", bit 3 "overtemperature")
    } motor_state_1;
    /*  This structure is incorrect (there is a mistake in the manual !)
    struct motor_state_1_s
    {
        unsigned char cmd;          // command byte - 0x9A
        signed char temp;           // temperature (1°C/LSB)
        unsigned char pad1;         // empty byte
        // unsigned short voltage;     // motor voltage (0.1V/LSB) => actually 0.01 V/LSB
        unsigned char voltage[2];       // PROBLEM : this field causes an alignment issue, apparently
        unsigned char pad2[2];      // empty bytes
        unsigned char error;        // error flags (bit 0 "undervoltage", bit 3 "overtemperature")
    } motor_state_1;
    */
    // Reply message for 0x9C "read motor state 2" format is "open_loop_reply"
    // Reply message for 0x9D "read motor state 3"
    struct motor_state_3_s
    {
        unsigned char cmd;          // command byte - 0x9D
        signed char temp;           // temperature (1°C/LSB)
        signed short A;             // current in motor phase A (1A/64LSB or 15.625 mA/LSB)
        signed short B;             // current in motor phase B (1A/64LSB or 15.625 mA/LSB)
        signed short C;             // current in motor phase C (1A/64LSB or 15.625 mA/LSB)
    } motor_state_3;
} MotorCmd_t;

// Container for motor status
typedef struct Motor_s
{
    // Motor command values (written by higher-level software, enforced by this driver)
    signed short power;         // open-loop power setting, -850 to +850
    signed long speed;          // closed-loop speed setting, 0.01°/s per LSB
    long long destination;      // odometry value the motor is asked to reach
    // Parsed replies from the motor
    MotorCmd_t open_loop_reply;
    MotorCmd_t pid_coefficients;    // Obtained from commands 0x30, 0x31 or 0x32
    MotorCmd_t acceleration;        // Obtained from commands 0x33 or 0x34
    MotorCmd_t encoder;
    signed long long position;      // Absolute position, accumulated, 56-bit
    signed long long previous;      // PREVIOUS absolute position, accumulated, 56-bit
    MotorCmd_t rotor_angle;
    MotorCmd_t motor_state_1;
    MotorCmd_t motor_state_3;
    // float voltage;                  // Decoded separately due to alignment issue
} Motor_t;

// ======= Driver Class ====================================================================
class LoggerAndDisplay;

class DRIVE
{   
    public:
        enum DRIVE_STATE : uint8_t{
            DRIVE_STOPPED = 0,
            DRIVE_RUNNING = 1
        };

    private :
        std::atomic<DRIVE_STATE> mstate;

    public:
        LoggerAndDisplay *mlogger = nullptr;

        // CAN bus sockets
        int sock; // formerly can_sock_motor_control;

        // CAN-related
        struct sockaddr_can addr;
        struct ifreq ifr;
        struct can_frame frame;

        pthread_t motor_thread;         // Motor control thread
        int kill;                  // Thread kill signal

        // Status of the motors
        Motor_t left;
        Motor_t right;

    public:
        DRIVE();
        ~DRIVE();

    public:     // Thread methods
        static void* motion_control_thread (void *arg);   // Experimental version
        static void* motion_control_thread_old (void *arg); // First version, not reliable
    
    public:     // Task function as a (temporary?) replacement for a thread
        void motors_on ();  // Motor init function, to be called only once, on transition from "armed" to "delay". Resets odometry.
        void task ();       // Must only be called in the "delay" and "run" states
        void motors_off (); // Motor off function, to be called when transitioning towards the idle state

    public:
        void can_send_motor (int id, unsigned char *data);
        int can_read_reply_frame (int noblock);
        void send (int id, unsigned char *data);    // equivalent to calling can_send_motor and can_read_reply_frame back to back (blocking)

    public:
        int init (LoggerAndDisplay*logger);       // Open and bind the CAN socket this object uses to talk to the motors
        void start ();            // Start the motor control thread
        void stop ();       // Kill the propulsion thread, close the CAN sockets

    public:     // Higher-level motor commands
        void power (signed short l, signed short r);      // Sends open-loop power commands to both motors (left, right)
        void speed (signed long l, signed long r);        // Sends closed-loop speed commands to both motors (left, right)
        void move (long l, long r);             // Sends the motors to specific positions expressed in odometry units
    
    DRIVE_STATE getDriveState() const;


};



#endif // __PAMI_DRIVE_HPP__