#include <cmath>
#include <ctime>
#include <cstdlib>
#include <iostream>
#include <string>
#include "../include/serialport.hpp"

#define MAX_DATA_LENGTH 255

SerialPort *arduino;                  // Declare a pointer to the arduino serial port 
char incomingData[MAX_DATA_LENGTH];   // Declare a buffer to collect incoming data from the port
char data;                            // Declare a variable to hold the incoming data

const std::string currentDateTime() {
    time_t     now = time(0);
    struct tm  tstruct;
    char       buf[80];
    tstruct = *localtime(&now);
    // Visit http://en.cppreference.com/w/cpp/chrono/c/strftime
    // for more information about date/time format
    strftime(buf, sizeof(buf), "%Y-%m-%d.%X", &tstruct);

    return buf;
}

int main(void)
{
  arduino = new SerialPort();
  arduino->setup();
  std::cout << currentDateTime() << "-->" << "[Connecting to arduino]: " <<((arduino->isConnected())?"Success":"Fail") << std::endl;
  while(arduino->isConnected()){
    if (arduino->readSerialPort(&data,1)>0)
    {
      std::cout << data;
      if (data == '\n')
      {
        std::cout << currentDateTime() << "-->";
      }      
    }
  }
  delete(arduino);
  return EXIT_SUCCESS;
}
