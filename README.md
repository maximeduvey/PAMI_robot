This project is to manage the code and structure of the litle robot called palmi

### COMPILER

### 
### WORK in progress ###
### the below is notes furing process, real instruction should arrive soon
###

# piggpio
<!-- cmake -DCMAKE_SYSTEM_NAME=Linux \
      -DCMAKE_SYSTEM_PROCESSOR=armv6 \
      -DCMAKE_C_COMPILER=arm-linux-gnueabihf-gcc \
      -DCMAKE_FIND_ROOT_PATH=/usr/arm-linux-gnueabihf \ 
      ..
      -->
<!-- cmake -DCMAKE_SYSTEM_NAME=Linux \
      -DCMAKE_SYSTEM_PROCESSOR=armv6 \
      -DCMAKE_C_COMPILER=arm-linux-gnueabihf-gcc \
      -DCMAKE_CXX_COMPILER=arm-linux-gnueabihf-g++ \
      -DCMAKE_FIND_ROOT_PATH=/usr/arm-linux-gnueabihf \
      .. -->

#### bcm2835 (careful the ocnfigure) 
# autoreconf -fvi
<!-- tar zxvf bcm2835-1.71.tar.gz
./configure --host=arm-linux-gnueabihf
make
sudo make check
sudo make install -->

### Libraries ###

# /libs/bcm2835-1.71  compile but you can't make make check, you need a "gpio" user group and add your user to it
sudo groupadd gpio
sudo usermod -aG gpio $USER
# after that it's a good pratice to have dedicated gpio folder 
sudo mkdir /dev/gpiomem
sudo mkdir /dev/gpiochip
# set owner
sudo chown root:gpio /dev/gpiomem
sudo chmod g+rw /dev/gpiomem
sudo chown root:gpio /dev/gpiochip*
sudo chmod g+rw /dev/gpiochip*
# (not tested) if you want to have permission and group persisten accross reboots, you can create a udev rule
sudo nano /etc/udev/rules.d/99-gpio.rules
# add to the file
SUBSYSTEM=="gpio", KERNEL=="gpiochip*", ACTION=="add", GROUP="gpio", MODE="0660"
SUBSYSTEM=="gpio", KERNEL=="gpio*", ACTION=="add", GROUP="gpio", MODE="0660"
SUBSYSTEM=="gpio", KERNEL=="gpiomem", ACTION=="add", GROUP="gpio", MODE="0660"
# reboot

### useful dpkg apt command
<!-- 
sudo dpkg --print-architecture  default configurtion : amd64

sudo dpkg --print-foreign-architectures
sudo dpkg --add-architecture armhf

sudo apt-get update
# to compile for raspberry pi you need
sudo apt install gcc-arm-linux-gnueabihf g++-arm-linux-gnueabihf

wget http://ports.ubuntu.com/pool/main/n/ncurses/libncurses5-dev_6.2-0ubuntu2_armhf.deb
wget http://ports.ubuntu.com/pool/main/n/ncurses/libncursesw5-dev_6.2-0ubuntu2_armhf.deb
 -->

# my dpkg repository  sudo nano /etc/apt/sources.list
# Original repository
#deb [arch=arm64] http://archive.ubuntu.com/ubuntu/ jammy universe
#deb [arch=arm64] http://archive.ubuntu.com/ubuntu/ jammy-updates universe
#deb [arch=arm64] http://archive.ubuntu.com/ubuntu/ jammy multiverse
#deb [arch=arm64] http://archive.ubuntu.com/ubuntu/ jammy-updates multiverse
#deb [arch=arm64] http://security.ubuntu.com/ubuntu/ jammy-security universe
#deb [arch=arm64] http://security.ubuntu.com/ubuntu/ jammy-security multiverse
#deb [arch=arm64] http://archive.ubuntu.com/ubuntu/ jammy main restricted
#deb [arch=arm64] http://archive.ubuntu.com/ubuntu/ jammy-updates main restricted
#deb [arch=arm64] http://archive.ubuntu.com/ubuntu/ jammy-backports main restricted universe multiverse
#deb [arch=arm64] http://security.ubuntu.com/ubuntu/ jammy-security main restricted

# added from chatgpt
deb [arch=armhf] http://ports.ubuntu.com/ubuntu-ports jammy main restricted
deb [arch=armhf] http://ports.ubuntu.com/ubuntu-ports jammy-updates main restricted
deb [arch=armhf] http://ports.ubuntu.com/ubuntu-ports jammy universe
deb [arch=armhf] http://ports.ubuntu.com/ubuntu-ports jammy-updates universe
deb [arch=armhf] http://ports.ubuntu.com/ubuntu-ports jammy multiverse
deb [arch=armhf] http://ports.ubuntu.com/ubuntu-ports jammy-updates multiverse
deb [arch=armhf] http://ports.ubuntu.com/ubuntu-ports jammy-backports main restricted universe multiverse
deb [arch=armhf] http://ports.ubuntu.com/ubuntu-ports jammy-security main restricted
deb [arch=armhf] http://ports.ubuntu.com/ubuntu-ports jammy-security universe
deb [arch=armhf] http://ports.ubuntu.com/ubuntu-ports jammy-security multiverse


deb [arch=armhf] http://ports.ubuntu.com/ubuntu-ports focal main restricted universe multiverse
deb [arch=armhf] http://ports.ubuntu.com/ubuntu-ports focal-updates main restricted universe multiverse
deb [arch=armhf] http://ports.ubuntu.com/ubuntu-ports focal-security main restricted universe multiverse
deb [arch=armhf] http://ports.ubuntu.com/ubuntu-ports focal-backports main restricted universe multiverse

deb [arch=arm64] http://ports.ubuntu.com/ focal main multiverse universe
deb [arch=arm64] http://ports.ubuntu.com/ focal-security main multiverse universe
deb [arch=arm64] http://ports.ubuntu.com/ focal-backports main multiverse universe
deb [arch=arm64] http://ports.ubuntu.com/ focal-updates main multiverse universe