#pragma once

#include <string>
#include <thread>
#include <atomic>

#define LD06_UART_DEFAUT_SERIAL_UART_PORT "/dev/serial0"
#define LD06_UART_PACKET_SIZE 45
#define LD06_UART_BAUD_RATE B230400 
//#define LD06_UART_BAUD_RATE B115200 

/// @brief this is the class to control and read the LIDAR LD06
///
/// It's pin configuration should be
/// VCC (LD06) -> Pin 2 (5V) on Raspberry Pi Zero
/// GND (LD06) -> Pin 6 (GND) on Raspberry Pi Zero
/// TXD (LD06) -> Pin 10 (GPIO 15 - RXD) on Raspberry Pi Zero
/// RXD (LD06) -> Pin 8 (GPIO 14 - TXD) on Raspberry Pi Zero
class Lidar_LD06
{
public:
    Lidar_LD06();
    virtual ~Lidar_LD06();

    void initAndStart();
    
    bool isRunning();

private:
    int open_serial_port(const std::string port_name);
    static void read_lidar_data(Lidar_LD06 *myself);

    std::string _portName;
    std::atomic<int> _serialPort;

    std::thread _runningThread;
    std::atomic<bool> _lidarRunningState;
};
