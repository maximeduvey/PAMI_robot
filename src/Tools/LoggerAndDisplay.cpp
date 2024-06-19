
#include "LoggerAndDisplay.h"

#include "PAMI.h"

#include <ncurses.h>
#include <fstream>
#include <ctime>
#include <cstdio>

#include <exception>


//std::ofstream LoggerAndDisplay::mLog_file;

LoggerAndDisplay::LoggerAndDisplay() {}
LoggerAndDisplay::~LoggerAndDisplay() {}

/// @brief This function init the logger, it will create a file that will store messages (displayed or not)
/// @param filename if not filename is passed in parameter the filename storing the logs will use the current timestamp
void LoggerAndDisplay::initLogs(std::string filename /*  = "" */)
{
    initscr();
    noecho();
    cbreak();
    if (filename == "")
    {
        filename = "logs/"+Tools::current_timestamp() + ".logs";
    }
    mLog_file = std::ofstream(filename.c_str(), std::ios_base::app);
    if (!mLog_file)
    {
        mvprintw(0, 0, (filename + " Error opening log file.").c_str());
        throw std::runtime_error("LoggerAndDisplay::Init_Logs() could not create log file:" + filename);
    } 
    //mvprintw(0, 0, ("Logs file created: " + filename).c_str());
}

void LoggerAndDisplay::closeLogs()
{
    if (mLog_file.is_open())
    {
        mLog_file.close();
    }
    endwin();
}

// todo: add a mutex for access
void LoggerAndDisplay::log_and_display(int y, int x, const char *format, ...)
{
    // Prepare the formatted message
    va_list args;
    va_start(args, format);
    char buffer[256];
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    if (mLog_file.is_open()) {
        mLog_file << "[" << std::to_string(x) << std::to_string(y) <<"] " << buffer << std::endl;
    }
    refresh();
}

// need to be repared
void LoggerAndDisplay::logAsPrintf(const char *format)
{
    //printf("logAsPrintf in: %s\n", format);
/*     va_list args;
    va_start(args, format);
    char buffer[256];
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);*/
/*     if (mLog_file.is_open()){
        mLog_file << format ;
     } */
   /* printf(buffer);
    refresh(); */
}

/// @brief this function will display the common infos of the current PAMI
/// It's meant to be called regulary and provide the real time stats
/// @param the pami which you want to display infos of
void LoggerAndDisplay::display_pami_stats(const PAMI& pami)
{
    log_and_display(4, 0, "DIP 1..4 : %i-%i-%i-%i", pami.io.s1, pami.io.s2, pami.io.s3, pami.io.s4);
    log_and_display(5, 0, "Side %i", pami.io.side);
    log_and_display(6, 0, "Pin %i", pami.io.pin);
    log_and_display(7, 0, "ToR 1, 2 : %i - %i", pami.io.tor1, pami.io.tor2);
    log_and_display(8, 0, "MT INT %i", pami.io.mtint);
    // Let's show what time it is
    //timespec_get(&pami.tzero, TIME_UTC);
    //log_and_display(9, 0, "Time: %d - %d", ts.tv_sec, ts.tv_nsec);
    log_and_display(9, 0, "Time: %i", pami.time);
    // Sleep a bit
    // usleep (10000);  // 10 ms delay // No longer needed, OLED takes long enough to refresh.
    // Display some variables for debugging
    // log_and_display(10, 0, "mode[0] = %i - SX_SERVO = %i", pami.sx.mode[0], SX_SERVO);
    // Print motor positions
    // log_and_display(10, 0, " Left: %d", pami.drive.left.position);
    // log_and_display(11, 0, "Right: %d", pami.drive.right.position);
    // log_and_display(10, 0, "long: %i bytes", sizeof(long)); // Verified that a long is 4 bytes on Raspi.
    log_and_display(10, 0, "batt: %f V", (float) pami.drive.left.motor_state_1.motor_state_1.voltage / 100.0);
    log_and_display(11, 0, "speeds %i | %i", pami.drive.left.speed, pami.drive.right.speed);
    
    refresh();
    // odometry on OLED
    /*sprintf(str, "%08i", pami.drive.left.position);
    pami.oled.print(str, 0, 2);
    sprintf(str, "%08i", pami.drive.right.position);
    pami.oled.print(str, 11, 2);*/
}