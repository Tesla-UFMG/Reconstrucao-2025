#include "serialib/serialib.h"
#include <vector>
#include <string>
#include <iostream>
#include <cstdio>


void listPorts(std::vector<std::string>& ports) {
    serialib device;
    std::string device_name;

    for (int i = 1; i < 99; i++) {
        #if defined(_WIN32) || defined(_WIN64)
            device_name = std::string("\\\\.\\COM") + std::to_string(i); // Ex.: \\.\COM1
        #elif defined(__linux__)
            device_name = std::string("/dev/ttyACM") + std::to_string(i - 1); // Ex.: /dev/ttyACM0 ... ttyACM97
        #else
            #error "Sistema operacional não suportado"
        #endif

        if (device.openDevice(device_name.c_str(), 115200) == 1) {
            ports.push_back(device_name);
            device.closeDevice();
        }
    }
}


int main() {    
    std::vector<std::string> ports;    
    listPorts(ports);
    std::string SERIAL_PORT = ports[0];

    serialib serial;
    char errorOpening = serial.openDevice(SERIAL_PORT.c_str(), 115200);

    if (errorOpening!=1) return errorOpening;
    printf ("Successful connection to %s\n",SERIAL_PORT.c_str());

    while (true){
        char buffer[15];
        serial.readString(buffer, '\n', 14, 2000);
        printf("String read: %s", buffer);
    }
    // Close the serial device
    serial.closeDevice();

    return 0;
}
