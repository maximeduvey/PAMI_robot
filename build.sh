#!/bin/bash
echo "building PAMI"
#rm pami
#g++ main.cpp oled.cpp sx1509.cpp gpio.cpp drive.cpp -o pami -lpthread -lncurses -lm -lpigpio

g++ \
src/*.cpp src/Drivers/*.cpp \
-o pami \
-Ilibs/pigpio-master \
-Llibs/pigpio-master/build \
-lpigpio \
-lpthread \
-lncurses \
-lm
