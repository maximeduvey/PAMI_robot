
#include "Lidar_LD06.h"

#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <cstdint>
#include <cstring>
#include <cstdio>
#include <vector>
#include <cmath>

#define DEBUG_LOG true

Lidar_LD06::Lidar_LD06()
{
    printf("Lidar_LD06::Lidar_LD06()\n");
}

Lidar_LD06::~Lidar_LD06()
{
}

/// @brief This function will initialize and start the LIDAR thread
/// this thread will fill/update a map with each point detected
void Lidar_LD06::initAndStart()
{
    if (DEBUG_LOG)
        printf("Lidar_LD06::initAndStart()\n");
    _lidarRunningState.store(false);
    open_serial_port(LD06_UART_DEFAUT_SERIAL_UART_PORT);
    std::thread lidar_thread(&Lidar_LD06::read_lidar_data, this);

    lidar_thread.detach();
        printf("Lidar_LD06::initAndStart(done)\n");
}

int Lidar_LD06::open_serial_port(const std::string port_name)
{
    if (DEBUG_LOG)
        std::cout << "Lidar_LD06::open_serial_port() port:"<< port_name << "\n";
    _portName = port_name;
    _serialPort.store(open(_portName.c_str(), O_RDWR | O_NOCTTY | O_NDELAY));
    if (_serialPort.load() == -1)
    {
        std::cerr << "Error opening serial port!" << std::endl;
        return -1;
    }

    struct termios tty;
    memset(&tty, 0, sizeof(tty));

    if (tcgetattr(_serialPort.load(), &tty) != 0)
    {
        std::cerr << "Error getting terminal attributes!" << std::endl;
        return -1;
    }

    // Set the baud rate
    cfsetospeed(&tty, LD06_UART_BAUD_RATE);
    cfsetispeed(&tty, LD06_UART_BAUD_RATE);

    // 8N1 mode (8 data bits, no parity, 1 stop bit)
    tty.c_cflag &= ~PARENB; // No parity bit
    tty.c_cflag &= ~CSTOPB; // Only 1 stop bit
    tty.c_cflag &= ~CSIZE;
    tty.c_cflag |= CS8; // 8 data bits

    tty.c_cflag &= ~CRTSCTS;       // Disable hardware flow control
    tty.c_cflag |= CREAD | CLOCAL; // Turn on the receiver

    // Set in raw mode
    tty.c_lflag &= ~ICANON;
    tty.c_lflag &= ~ECHO;
    tty.c_lflag &= ~ECHOE;
    tty.c_lflag &= ~ISIG;
    tty.c_iflag &= ~(IXON | IXOFF | IXANY); // Disable software flow control
    tty.c_oflag &= ~OPOST;                  // Raw output mode

    auto srPort =  _serialPort.load();
    tcsetattr(srPort, TCSANOW, &tty);

    std::cout << "Lidar_LD06::open_serial_port() successfully open with serial port:"<< srPort << std::endl;
    return srPort;
}

/// @brief static Function to read data from the LD06 lidar
/// @param myself
void Lidar_LD06::read_lidar_data(Lidar_LD06 *myself)
{
    int srPort =  myself->_serialPort.load();
    if (DEBUG_LOG)
        std::cout << "Lidar_LD06::read_lidar_data() using serial port:"<< srPort <<" \n" ;
    uint8_t header = 0;
    uint8_t data[LD06_UART_PACKET_SIZE];
    myself->_lidarRunningState.store(true);

    while (true)
    {
        // Read the header byte (0x54)
        int ret = read(srPort, &header, 1);
        if (ret == 0 || header == 0x54)
        {
            // Read the remaining packet (44 bytes)
            int bytes_read = read(srPort, &data, LD06_UART_PACKET_SIZE - 1);
            if (bytes_read != LD06_UART_PACKET_SIZE - 1)
            {
                std::cerr << "Error reading data packet! " << bytes_read << std::endl;
                continue;
            }

            // Parse the packet data
            uint16_t speed = (data[1] << 8) | data[0];
            uint16_t start_angle = (data[3] << 8) | data[2];
            uint16_t end_angle = (data[5] << 8) | data[4];
            uint16_t distances[41];

            for (int i = 0; i < 41; i++)
            {
                distances[i] = (data[2 * i + 7] << 8) | data[2 * i + 6];
            }

            float start_angle_deg = start_angle / 100.0f;
            float end_angle_deg = end_angle / 100.0f;

            // Display the results
            if (DEBUG_LOG)
                std::cout << "Speed: " << speed / 100.0f << " deg/sec" << std::endl;
            if (DEBUG_LOG)
                std::cout << "Start Angle: " << start_angle_deg << " degrees" << std::endl;
            if (DEBUG_LOG)
                std::cout << "End Angle: " << end_angle_deg << " degrees" << std::endl;

            if (DEBUG_LOG)
                std::cout << "Distances: ";
            for (int i = 0; i < 41; i++)
            {
                if (DEBUG_LOG)
                    std::cout << distances[i] << " ";
            }
            if (DEBUG_LOG)
                std::cout << std::endl
                          << std::endl;

            usleep(50000); // Delay to avoid overwhelming the serial port
        }
        else
        {
            if (DEBUG_LOG)
                std::cout << "Skipped header ret:" << ret << ", header value:" << (int)header  << std::endl;
        }
    }
}

bool Lidar_LD06::isRunning()
{
    return _lidarRunningState.load();
}
