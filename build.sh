#!/bin/bash
echo "building PAMI"
#rm pami
g++ main.cpp oled.cpp sx1509.cpp gpio.cpp drive.cpp -o pami -lpthread -lncurses -lm -lpigpio
