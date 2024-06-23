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

LoggerAndDisplay gLogger;

// === tmp test purpose
void test_logger(){
    gLogger.initLogs();
    gLogger.log_and_display(4, 0, "DIP 1..4 : %i-%i-%i-%i", 0, 20, 50, 90);
    gLogger.log_and_display(1, 0, "Ananaas : %s", "et caribou");

    getch();

    gLogger.closeLogs();
}

void print_oled(unsigned int loopcnt)
{
    char str[30];
    sprintf(str, "%05i", loopcnt);
    gPami.oled.print(str, 0, 0);
    gPami.stateString (str);
    gPami.oled.print(str, 10, 0); // And let's also display the current state
    gPami.oled.print(gPami.IP, 0, 1); // The PAMI's IP address on wlan0 (WiFi)
    
    gPami.oled.print(str, 0, 2);
    gPami.oled.refresh();
        
    sprintf (str, "gPami %i - batt %2.2f V", gPami.id, (float) gPami.drive.left.motor_state_1.motor_state_1.voltage / 100.0);
}

void mainInit(){
    gLogger.initLogs(); // init log file and ncurses
    gPami.init(&gLogger); // Initialize the PAMI
}

// ==== Main function ===============================
int main()
{
    // Initialization
    mainInit();

    gLogger.log_and_display(0, 0, "----== PAMI ==----");
    gLogger.log_and_display(1, 0, "host : %s", gPami.hostname);
    gLogger.log_and_display(2, 0, "PAMI : %i", gPami.id);
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
        // main task, will manage the others (run, wait, etc..)
        gPami.tasks();

        // ============= DEBUG CODE ==========================================================
        // Debug only : ensure execution ends cleanly after a few seconds
        gLogger.log_and_display(3, 0, "%05i", loopcnt);
        
        // Display on the OLED as well
        print_oled(loopcnt);

        // Display some of the inputs
        gLogger.display_pami_stats(gPami);

        // New servo test
        //if (0)
        //{
        //    gPami.sx.move(0, ratio);
        //    gPami.sx.move(15, ratio);
        //    ratio = (ratio + 1) % 100;
        //}
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