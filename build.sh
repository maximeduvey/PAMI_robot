#!/bin/bash
echo "building PAMI"
#rm pami
#g++ main.cpp oled.cpp sx1509.cpp gpio.cpp drive.cpp -o pami -lpthread -lncurses -lm -lpigpio



g++ \
src/*.cpp \
src/Drivers/*.cpp \
src/Tools/*.cpp \
src/Actions/*.cpp \
-Isrc/ \
-Isrc/Drivers/ \
-Isrc/Tools/ \
-Isrc/Tools/ \
-Isrc/Actions/ \
-Isrc/Drivers/Components_Interface/ \
-o pami \
-lpigpio \
-lpthread \
-lncurses \
-lwiringPi \
-lm
