/*  PAMI 2024 - Main Source File
    Nefastor for Team Goldorak
    Notes:
    - This program is designed to never exit. It is meant to run from boot
      and until power is cut or the Pi is somehow shutdown or rebooted.
*/

// Project headers
#include "common_includes.h"
#include "LoggerAndDisplay.h"
#include "PAMI.h"

// ==== Globals =====================================

PAMI gPami; // mais salami
//LoggerAndDisplay gLogger;

// === tmp test purpose
void test_logger(){
    LoggerAndDisplay::initLogs();
    LoggerAndDisplay::log_and_display(4, 0, "DIP 1..4 : %i-%i-%i-%i", 0, 20, 50, 90);
    LoggerAndDisplay::log_and_display(1, 0, "Ananaas : %s", "et caribou");

    getch();

    LoggerAndDisplay::closeLogs();
}

// ==== Main function ===============================
// for test purpose
int main()
{
    test_logger();
    //main_tmp()
    return 0;
}


int main_tmp ()
{
    // Initialization
    initscr ();    // Initialize ncurses
    gPami.init();    // Initialize the PAMI

    mvprintw (0, 0, "----== PAMI ==----");
    mvprintw (1, 0, "host : %s", gPami.hostname);
    mvprintw (2, 0, "PAMI : %i", gPami.id);
    refresh ();

// ============ DEMOs ================================================================

    // gPami.sx.demo1 ();       // Servo demo 

// ===================================================================================

    // Debug only : loop down-counter
    unsigned int loopcnt = 10000;    // the main loop has a 10 ms execution time (forced for now)
    // demo only : servo angle
    unsigned int ratio = 0;     // open-loop control

    // Test only : start the motors with two different speeds
    // gPami.drive.power(-100, -200);     // left, right

    // Main loop
    // while (1)
    while (gPami.io.s4 == 0)     // Use DIP switch 4 to end the program
    {
        gPami.tasks();

// ============= DEBUG CODE ==========================================================
        // Debug only : ensure execution ends cleanly after a few seconds
        mvprintw (3, 0, "%05i", loopcnt);
        // Display on the OLED as well
        char str[30];
        sprintf(str, "%05i", loopcnt);
        gPami.oled.print(str, 0, 0);
        gPami.stateString (str);
        gPami.oled.print(str, 10, 0); // And let's also display the current state
        gPami.oled.print(gPami.IP, 0, 1); // The PAMI's IP address on wlan0 (WiFi)
        // gPami.oled.refresh();
        // Display some of the inputs
        mvprintw (4, 0, "DIP 1..4 : %i-%i-%i-%i", gPami.io.s1, gPami.io.s2, gPami.io.s3, gPami.io.s4);
        mvprintw (5, 0, "Side %i", gPami.io.side);
        mvprintw (6, 0, "Pin %i", gPami.io.pin);
        mvprintw (7, 0, "ToR 1, 2 : %i - %i", gPami.io.tor1, gPami.io.tor2);
        mvprintw (8, 0, "MT INT %i", gPami.io.mtint);
        // Let's show what time it is
        //timespec_get(&gPami.tzero, TIME_UTC);
        //mvprintw (9, 0, "Time: %d - %d", ts.tv_sec, ts.tv_nsec);
        mvprintw (9, 0, "Time: %i", gPami.time);
        // Sleep a bit
        // usleep (10000);  // 10 ms delay // No longer needed, OLED takes long enough to refresh.
        // Display some variables for debugging
        // mvprintw (10, 0, "mode[0] = %i - SX_SERVO = %i", gPami.sx.mode[0], SX_SERVO);
        // Print motor positions
        // mvprintw (10, 0, " Left: %d", gPami.drive.left.position);
        // mvprintw (11, 0, "Right: %d", gPami.drive.right.position);
        // mvprintw (10, 0, "long: %i bytes", sizeof(long)); // Verified that a long is 4 bytes on Raspi.
        mvprintw (10, 0, "batt: %f V", (float) gPami.drive.left.motor_state_1.motor_state_1.voltage / 100.0);
        mvprintw (11, 0, "speeds %i | %i", gPami.drive.left.speed, gPami.drive.right.speed);
        
        refresh();
        // odometry on OLED
        /*sprintf(str, "%08i", gPami.drive.left.position);
        gPami.oled.print(str, 0, 2);
        sprintf(str, "%08i", gPami.drive.right.position);
        gPami.oled.print(str, 11, 2);*/
        sprintf (str, "gPami %i - batt %2.2f V", gPami.id, (float) gPami.drive.left.motor_state_1.motor_state_1.voltage / 100.0);
        gPami.oled.print(str, 0, 2);
        gPami.oled.refresh();
        // New servo test
        if (0)
        {
            gPami.sx.move(0, ratio);
            gPami.sx.move(15, ratio);
            ratio = (ratio + 1) % 100;
        }
        // if (loopcnt-- == 0) break;   // Auto-termination on time-out (safety feature during debug)
        // ===================================================================================
    }



    // Note : everything past this point should never be executed,
    // as the infinite loop is infinite.

    
    gPami.io.terminate ();
    close (gPami.file_i2c);  // Close the I2C bus
    gPami.drive.stop (); // Kill the propulsion thread, close the CAN sockets
    endwin ();    // End ncurses
    

    return 0;
}
