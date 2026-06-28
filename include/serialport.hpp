/*
 * Author: Engr. Daniel Moune
 * Modified Library introduced in Arduino Playground which does not work
 * This works perfectly
 * LICENSE: MIT
 *
 *  DESCRIPTION
 * The <termios.h> header contains the definitions used by the terminal I/O interfaces
 * (see the XBD specification, General Terminal Interface  for the structures and names defined).
 *
 * The termios structure is defined, and includes at least the following members:
 * tcflag_t  c_iflag     input modes
 * tcflag_t  c_oflag     output modes
 * tcflag_t  c_cflag     control modes
 * tcflag_t  c_lflag     local modes
 * cc_t      c_cc[NCCS]  control chars
 */

#pragma once

#define ARDUINO_WAIT_TIME 2000
#define MAX_DATA_LENGTH 255

#include <fcntl.h>   // File control definitions
#include <termios.h> // POSIX terminal control definitions
#include <unistd.h>  // UNIX standard function definitions
#include <cstring>   // memset

typedef unsigned long DWORD; // DWORD is unsigned long

/**
 * @author Engr. Daniel Moune
 * @brief Serial Port Interface for Arduino
 *
 */
class SerialPort
{
private:
    const char *device; // serial port location on the operating system
    speed_t speed;      // baud rates supported by the underlying hardware
    int serial_fd;      // file descriptor for serial stream
    int serial_status;  // status of the serial port
    struct termios tty; // sets up terminal configuration for the serial port
    bool connected;     // true if the serial port is connected
    DWORD errors;       // errors from the serial port
public:
    /**
     * @brief Construct a new Serial Port object
     * from the port name provided as argument.
     * @param portName port location to connect to
     */
    SerialPort(const char *portName);
    /**
     * @brief Construct a new Serial Port object.
     * The default port location is "/dev/ttyACM0"
     */
    SerialPort();
    /**
     * @brief Destroy the Serial Port object.
     *
     */
    ~SerialPort();
    /**
     * @brief Reads from serial port into buffer.
     *
     * @param buffer    buffer that will be filled
     * @param buf_size  nbr of bytes to be read
     * @return the number of bytes written if successful, -1 otherwise
     */
    int readSerialPort(char *buffer, unsigned int buf_size);
    /**
     * @brief Writes buffer to serial port
     *
     * @param buffer    data to be written on the stream
     * @param buf_size  nbr of bytes to be written
     * @return the number of bytes written if successful, -1 otherwise
     */
    int writeSerialPort(const char *buffer, unsigned int buf_size);
    /**
     * @brief Checks if serial port is connected
     *
     * @return True if connected, false otherwise
     */
    bool isConnected() const;
    /**
     * @brief setup serial port
     *
     */
    void setup();
    /**
     * @brief closes serial port
     *
     * @return void
     */
    void closeSerial();
    /**
     * @brief Get the Port Configuration description
     *
     * @return description of the port configuration
     */
    // std::string getPortConfig();
};
