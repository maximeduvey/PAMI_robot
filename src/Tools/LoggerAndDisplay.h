
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

    void initLogs(std::string filename = "");
    void closeLogs();

    static void toreplace_log_and_display(int y, int x, const char *format, ...);
    void log_and_display(int y, int x, const char *format, ...);
    void logAsPrintf(const char *format, ...);

    void display_pami_stats(const PAMI& pami);

private :
    std::ofstream mLog_file;
};
