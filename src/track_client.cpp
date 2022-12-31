#include "serial/serial.h" // https://www.github.com/wjwwood/serial
#include "mak_packet.h"
#include <iostream>
#include <unistd.h>
#include <sys/ioctl.h>
#include <string>

#define SERVER_PORT_DESC "Prolific Technology"
#define REQUIRE_SERVER false
#define USE_STATIC_PORTS true
#define STATIC_PORT_SERVER "/dev/ttyUSB2"

#define BAUDRATE 115200U

using namespace std;
using namespace serial;

int main(void){


    // ensure that we're on a linux system
    // TODO: handle Windows, darwin, etc.
    if(!__linux__){
        printf("Error: not running on a linux system\n");
        return -1;
    }

    // ensure width of float type is four bytes
    // TODO: handle other cases
    if(sizeof(float) != 4){
        printf("Error: float datatype is not four bytes wide\n");
        return -1;
    }
    //static_assert(sizeof(float)==4,"Float size is not four bytes!"); 

    // ensure little endian encoding
    // TODO: handle big endian case
    static float test_float = 1.254;
    uint8_t* p_fval = (uint8_t*) &test_float;
    if(*p_fval != 0x12){
        printf("Error: not a litle endian system\n");
        return -1;
    }


    // locate serial port 
    std::string server_port_string = "";

    if(USE_STATIC_PORTS){
        server_port_string = std::string(STATIC_PORT_SERVER);
    } else {   
        vector<PortInfo> all_ports = list_ports();
        if (all_ports.size() > 0) {

            // look through all COM ports
            cout << "Searching available COM ports..." << endl;
            for (unsigned int i = 0; i < all_ports.size(); i++) {

                cout << all_ports[i].port << ": " << all_ports[i].description << endl;
                /*
                // check for client port
                if( all_ports[i].description.find(STATIC_PORT_DESC) != std::string::npos){
                if(!server_port_string.empty()){
                printf("Error: multiple client ports found!\n");
                return -1;
                }
                client_port_string = all_ports[i].port;
                cout << "Found client on " << client_port_string << endl;
                }
                */
            }
            /*
               if( client_port_string.empty() ) {
               cout << "Error: no client port found" << endl;
               if(REQUIRE_CLIENT) 
               return -1;
               }
               */


        } else {
            cout << "No ports found!" << endl;
        }
    }

    // open serial connection to server (server in turn is connected to the tracker)
    cout << "Attempting to open " << server_port_string; 
    Serial* mySerialPort = NULL;
    try {
        mySerialPort = new Serial(server_port_string, BAUDRATE, Timeout(1,25,0,0,0), eightbits, parity_none, stopbits_one, flowcontrol_none);
    } catch(IOException const& e){
        cout << e.what() << endl;
        return -1;
    };
    mySerialPort->flush();
    cout << "done." << endl;



    cout << "Waiting to read bytes" << endl;
    uint8_t bytebuffer[5000];
    size_t bytes_read = 0;


    Timeout to = mySerialPort->getTimeout();
    cout << "Timeout(" << to.inter_byte_timeout << "," << to.read_timeout_constant << "," << to.read_timeout_multiplier << "," << to.write_timeout_constant << "," << to.write_timeout_multiplier << ")" <<  endl;

    while(1){
        while((int)bytes_read == 0){
            bytes_read = mySerialPort->read(bytebuffer,5000);
        }
        printf("Read %ld bytes: ", bytes_read);
        for(int i = 0; i<((int)bytes_read); i++){
            printf("0x%02X ",bytebuffer[i]);
        }
        printf("\n\n");
        bytes_read = 0;
    }



    // done
    return 0;
}
