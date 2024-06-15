#!/bin/bash

### This script is for deploying and compiling the library for the pali
### 

#    |\__/,|   (`\
#  _.|o o  |_   ) )
# -(((---(((--------

source ../Include.sh

LIBS_FOLDER_NAME="libs"
CURRENT_PWD_FOLDER=$PWD

GLIBC_TAR_NAME="glibc-2.34.tar.gz"
GLIBC_FOLDER_NAME="glibc-2.34"

PIGPIO_TAR_NAME="pigpio_repo_master.zip"
PIGPIO_FOLDER_NAME="pigpio-master"

BCM_TAR_NAME="bcm2835-1.71.tar.gz"
BCM_FOLDER_NAME="bcm2835-1.71"

#################
### FUNCTIONS ###
#################

deploy_lib_glibc_2_34() {
    Logger $WHITE "Starting deploy_lib_glibc_2_34"
    run_command_and_exist_if_fail tar -xzf $GLIBC_TAR_NAME
    run_command_and_exist_if_fail cd $GLIBC_FOLDER_NAME
    run_command_and_exist_if_fail mkdir $BUILD_FOLDER_NAME
    run_command_and_exist_if_fail cd $BUILD_FOLDER_NAME
    run_command_and_exist_if_fail ../configure --prefix=/opt/glibc-2.34
    run_command_and_exist_if_fail make -j4
    #run_command_and_exist_if_fail sudo make install
    
    Logger $GREEN "$GLIBC_FOLDER_NAME deployed !"
    # if you don't want to make a "make isntall"
    # you may need to : LD_LIBRARY_PATH=/opt/glibc-2.34/lib ./pami
    run_command_and_exist_if_fail cd $CURRENT_PWD_FOLDER
}

#      Time to make the pig
#         <`--'\>______
#         /. .  `'     \
#        (`')  ,        @
#         `-._,        /
#            )-)_/--( >  jv
#           ''''  ''''
deploy_pigpio(){
    Logger $WHITE "Starting deploy_pigpio"
    
    run_command_and_exist_if_fail unzip $PIGPIO_TAR_NAME
    run_command_and_exist_if_fail cd $PIGPIO_FOLDER_NAME
    run_command_and_exist_if_fail mkdir $BUILD_FOLDER_NAME
    run_command_and_exist_if_fail cd $BUILD_FOLDER_NAME
    run_command_and_exist_if_fail cmake -DCMAKE_SYSTEM_NAME=Linux \
      -DCMAKE_SYSTEM_PROCESSOR=armv6 \
      -DCMAKE_C_COMPILER=arm-linux-gnueabihf-gcc \
      -DCMAKE_FIND_ROOT_PATH=/usr/arm-linux-gnueabihf \ 
      ..
    run_command_and_exist_if_fail make

    Logger $GREEN "$PIGPIO_FOLDER_NAME deployed !"
    run_command_and_exist_if_fail cd $CURRENT_PWD_FOLDER
}

deploy_bcm(){
    Logger $WHITE "Starting bcm"
    
    run_command_and_exist_if_fail tar -xzf $BCM_TAR_NAME
    run_command_and_exist_if_fail cd $BCM_FOLDER_NAME
    run_command_and_exist_if_fail mkdir $BUILD_FOLDER_NAME
    run_command_and_exist_if_fail cd $BUILD_FOLDER_NAME

    run_command_and_exist_if_fail ../configure --host=arm-linux-gnueabihf
    run_command_and_exist_if_fail make
    
    #run_command_and_exist_if_fail sudo make install
    
    Logger $GREEN "$BCM_FOLDER_NAME deployed !"
    run_command_and_exist_if_fail cd $CURRENT_PWD_FOLDER
}

main(){
    Logger $GREEN "Starting libs deployements"
    run_command_and_exist_if_fail cd $LIBS_FOLDER_NAME
    CURRENT_PWD_FOLDER=$PWD

    deploy_lib_glibc_2_34
    deploy_pigpio
    deploy_bcm
}

# just a tmp test func, to delete later
test() {
    run_command_and_exist_if_fail echo "hey"
    run_command_and_exist_if_fail Logger $GREEN "Ehhlo"
    run_command_and_exist_if_fail sudo ls -la
}

main
