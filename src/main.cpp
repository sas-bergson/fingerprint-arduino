#include <cmath>
#include <ctime>
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <sstream>
#include <bitset>
#include <string>
#include "../include/serialport.hpp"

#define MAX_DATA_LENGTH 255

SerialPort *arduino;                  // Declare a pointer to the arduino serial port 
char incomingData;                    // Declare a buffer to collect incoming data from the port
std::stringstream ss;                 // Declare a buffer to stack incoming data 
std::stringstream ss_data;
char userInput[MAX_DATA_LENGTH];      // Declare a buffer to collect user input
int pid;                              // Declare a process ID
uint8_t k = 0;

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
        ss << incomingData; 
        if (incomingData == '\n')
        {
          std::string stmt = ss.str();
          bool data_available = (stmt.find("packet->data")!=std::string::npos?true:false);
          if (data_available)
          {
             std::cout << "Received packet from arduino" << std::endl;
             std::string data = stmt.substr(17,stmt.size()-21);
             if (k>0) ss_data << " ";
             ss_data << data;
             std::cout << currentDateTime() << "-->" << ss_data.str() << " | " << ss_data.str().size()  << std::endl;
             k++;
             if (k==4){
              std::string fname (currentDateTime());
              std::ofstream outfile (fname + ".txt",std::ios::app);
              std::ofstream outfile2 (fname + ".bin", std::ios::binary|std::ios::app);
              std::string byte_str;
              uint8_t byte;
              for (size_t i = 0; i < ss_data.str().size()/6+1; i++)
              {
                ss_data >> byte_str;
                std::cout << byte_str.substr(0,4) << std::endl;
                outfile << byte_str;
                byte = std::stoi(byte_str.substr(0,4),0,16);
                std::bitset<8> x(byte);
                std::cout << x << ", " ;
                outfile2 << byte;
              }
              k = 0;
              outfile.close();
              outfile2.close();
              std::cout << std::endl;
              std::stringstream().swap(ss_data);
             }
          }
          else std::cout << currentDateTime() << "-->" << stmt;
          std::stringstream().swap(ss);
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
