
# pragma once

// 4 RPS / 240 RPM max speed
// Set the initial speed of the motors
// Initial values are unequal to compensate for issues with the motors.
/* #define DEFAULT_MOTOR_SPEED_PAMI_ONE_WHEEL_LEFT 2000
#define DEFAULT_MOTOR_SPEED_PAMI_ONE_WHEEL_RIGHT 2880 */

//left:3085, right:1795
#define DEFAULT_MOTOR_SPEED_PAMI_ONE_WHEEL_LEFT 1829
#define DEFAULT_MOTOR_SPEED_PAMI_ONE_WHEEL_RIGHT 3051

#define DEFAULT_MOTOR_SPEED_PAMI_TWO_WHEEL_LEFT 2880
#define DEFAULT_MOTOR_SPEED_PAMI_TWO_WHEEL_RIGHT 2050

#define DEFAULT_MOTOR_SPEED_PAMI_THREE_WHEEL_LEFT 1700
#define DEFAULT_MOTOR_SPEED_PAMI_THREE_WHEEL_RIGHT 2880

/// This is the default value to do a full turn turn for the wheel with the default above value
/// (suposition) I suppose 36000 is the number of point around the wheel, it's also the motor position on the Drive
#define DEFAULT_ONE_FULL_TURN_RPM 36000
//#define MINIMETER_PER_TURN 12.9
#define MINIMETER_PER_TURN 8.6

// This is a temporary timer measured by hand taken to do a "1 turn action"
// for the Motors to do a fulle turn (one direction or opposed direction)
// for a config rpm 36 000 for a speed of 2000 / 2880
#define TIME_ONE_TURN_ACTION_MILISEC 800

// this represent the tolerance we accepte on the motor position
// for example: we ask 36000 and get 35995 it's goo
#define MOTOR_POSITION_TOLERANCE 6

// this is the value we will apply to try to manage the difference in motor speed 
// to try to maintain a straight line
#define SPEED_STRENGHT_CORRECTION 5