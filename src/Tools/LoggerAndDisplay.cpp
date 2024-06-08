
#include "LoggerAndDisplay.h"

#include <ncurses.h>
#include <fstream>
#include <ctime>
#include <cstdio>

#include <exception>

// Define the static member variable
std::ofstream LoggerAndDisplay::mLog_file;

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
        filename = Tools::current_timestamp() + ".logs";
    }
    mLog_file = std::ofstream(filename, std::ios_base::app);
    if (!mLog_file)
    {
        mvprintw(0, 0, (filename + " Error opening log file.").c_str());
        throw std::runtime_error("LoggerAndDisplay::Init_Logs() could not create log file:" + filename);
    }
    mvprintw(0, 0, ("Logs file created: " + filename).c_str());
}

void LoggerAndDisplay::closeLogs()
{
    if (mLog_file.is_open())
    {
        mLog_file.close();
    }
    endwin();
}

void LoggerAndDisplay::log_and_display(int y, int x, const char *format, ...)
{
    // Prepare the formatted message
    va_list args;
    va_start(args, format);
    char buffer[256];
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    mvprintw(y, x, buffer);
    if (!mLog_file){
    mLog_file << "[" << std::to_string(x) << std::to_string(y) <<"] " << buffer << std::endl;
    }
    refresh();
}

void LoggerAndDisplay::logAsPrintf(const char *format, ...)
{

    va_list args;
    va_start(args, format);
    char buffer[256];
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    if (!mLog_file){
    mLog_file << buffer << std::endl;
    }
    printf(buffer);
    refresh();
}