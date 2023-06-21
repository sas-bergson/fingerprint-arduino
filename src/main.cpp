#include <cmath>
#include <ctime>
#include <cstdlib>
#include <iostream>
#include <string>
#include "../include/serialport.hpp"

#define MAX_DATA_LENGTH 255

SerialPort *arduino;                  // Declare a pointer to the arduino serial port 
char incomingData;                    // Declare a buffer to collect incoming data from the port
char userInput[MAX_DATA_LENGTH];      // Declare a buffer to collect user input
int pid;                              // Declare a process ID

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
  sleep(1);
  std::cout << currentDateTime() << "-->" << "[Connecting to arduino]: " <<((arduino->isConnected())?"Success":"Fail") << std::endl;
  
  if(arduino->isConnected()){
    pid = fork();   // Fork a child process
  }

  while(arduino->isConnected())
  {
     if(pid == 0)
    {
      while (arduino->readSerialPort(&incomingData,1)>0)
      {
        std::cout << incomingData;
        if (incomingData == '\n')
        {
          std::cout << currentDateTime() << "-->";
        }      
      }
    }
    else
    {
      std::cin >> userInput;
      arduino->writeSerialPort(userInput,sizeof userInput);
    }
  }

  delete(arduino);
  
  return EXIT_SUCCESS;
}
