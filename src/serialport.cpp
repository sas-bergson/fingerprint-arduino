/*
* Author: Daniel Moune
* Modified Library introduced in Arduino Playground which does not work
* LICENSE: MIT
*/

#include "../include/serialport.hpp"

const char *UNIX_DEFAULT = "/dev/ttyACM0";  //sets where the serial port is plugged in

SerialPort::SerialPort(const char *portName)
{
    device = portName;
    speed = B9600;
    memset (&tty, 0, sizeof tty);
    connected = false;
}

SerialPort::SerialPort()
{
    device = UNIX_DEFAULT;
    speed = B9600;
    memset (&tty, 0, sizeof tty);
    connected = false;
}

SerialPort::~SerialPort()
{
    if (connected)
    {
        closeSerial();
    }
}

// Reading bytes from serial port to buffer;
// returns read bytes count on success, -1 if error occurs or 0 if EOF
int SerialPort::readSerialPort(char *buffer, unsigned int buf_size)
{
    return read(serial_fd, (void*) buffer, buf_size);
}

// Sending provided buffer to serial port;
// returns writes bytes count on success, -1 if error occurs
int SerialPort::writeSerialPort(const char *buffer, unsigned int buf_size)
{
    return write(serial_fd, (void*) buffer, buf_size);
}

bool SerialPort::isConnected()
{
    if(serial_fd != -1 && serial_status != -1)
    {
        connected = true;
    }
    return connected;
}

void SerialPort::setup()
{
    serial_fd = open(device, O_RDWR | O_NOCTTY | O_NDELAY); //opens serial port in device slot
    serial_status = tcgetattr(serial_fd, &tty); //get device serial monitor confiuration
    cfsetospeed(&tty, speed);
    cfsetispeed(&tty, speed);

    tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;     // 8-bit chars
        // disable IGNBRK for mismatched speed tests; otherwise receive break
        // as \000 chars
    tty.c_iflag &= ~IGNBRK;         // disable break processing
    tty.c_lflag = 0;                // no signaling chars, no echo,
                                    // no canonical processing
    tty.c_oflag = 0;                // no remapping, no delays
    tty.c_cc[VMIN]  = 0;            // read doesn't block
    tty.c_cc[VTIME] = 5;            // 0.5 seconds read timeout

    tty.c_iflag &= ~(IXON | IXOFF | IXANY); // shut off xon/xoff ctrl

    tty.c_cflag |= (CLOCAL | CREAD);// ignore modem controls,
                                    // enable reading
    tty.c_cflag &= ~(PARENB | PARODD);      // shut off parity
    tty.c_cflag |= 0;
    tty.c_cflag &= ~CSTOPB;
    tty.c_cflag &= ~CRTSCTS;

    serial_status = tcflush(serial_fd, TCIFLUSH);    //flush input buffer
    serial_status = tcsetattr(serial_fd, TCSANOW, &tty); //set serial port
    if(serial_fd != -1 && serial_status != -1)
    {
        connected = true;
    }
}

void SerialPort::closeSerial()
{
    delete device;
    speed = errors = 0;
    connected = false;
    close(serial_fd);
}
