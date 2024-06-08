
#pragma once

#include <fstream>
#include <string>
#include <cstdarg>
#include <cstdio>

#include "Tools.h"

class LoggerAndDisplay
{
private:
    /* data */
public:
    LoggerAndDisplay();
    ~LoggerAndDisplay();

    static void initLogs(std::string filename = "");
    static void closeLogs();

    static void log_and_display(int y, int x, const char *format, ...);
    static void logAsPrintf(const char *format, ...);

private :
    static std::ofstream mLog_file;
};
