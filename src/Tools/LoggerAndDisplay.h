
#pragma once

#include <fstream>
#include <string>
#include <cstdarg>
#include <cstdio>
#include <mutex>
#include "Tools.h"

class PAMI;

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

    static void display_pami_stats(const PAMI& pami);

private :
    static std::ofstream mLog_file;
};
