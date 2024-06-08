
#pragma once

// this Tools class not only provide common function accros classes
// it also provide common include that may be needed accross multiple classes
// and prevent include reference loop 

#include <iostream>
#include <cstring>

class Tools
{
private:
    /* data */
public:
    Tools(/* args */);
    ~Tools();
    static std::string current_timestamp();
};


