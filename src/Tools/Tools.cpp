#include "Tools.h"

#include <ncurses.h>
#include <ctime>
#include <cstdarg>
#include <cstdio>
#include <fstream>

Tools::Tools(/* args */)
{
}

Tools::~Tools()
{
}

std::string Tools::current_timestamp() {
    std::time_t now = std::time(nullptr);
    char buf[20];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d_%H-%M-%S", std::localtime(&now));
    return std::string(buf);
}
