#include "serial/serial.h" // https://www.github.com/wjwwood/serial
#include "mak_packet.h"
#include <CombinedApi.h>
#include <iostream>
#include <unistd.h>
#include <sys/ioctl.h>
#include <string>

#define AURORA_PORT_DESC "NDI Aurora"
#define STATIC_PORT_DESC "Prolific Technology"

#define REQUIRE_AURORA false
#define REQUIRE_CLIENT false

#define USE_STATIC_PORTS true
#define STATIC_PORT_SERVER "/dev/ttyUSB2"

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
    //printf("0x%02X%02X%02X%02X\n", *p_fval, *(p_fval+1), *(p_fval+2), *(p_fval+3));


    // build a sample packet
    uint8_t mypacket[MAX_PACKET_LENGTH] = {0x00};
    size_t mypacket_length = MAX_PACKET_LENGTH;

    uint32_t frame_num = 4011;
    uint8_t tool_num = 2;
    float q[SIZE_Q] = {1.2,2.3,3.4,4.5};
    //float q[SIZE_Q] = {1.2,2.3,3.4,4.501953}; // force a DLE stuff in q[3]
    float t[SIZE_T] = {5.6,6.7,7.8};
    float trk_fit_err = 0.2537;

    int result = 0;
    result = compose_tracker_packet(mypacket, &mypacket_length, frame_num, tool_num, q, SIZE_Q, t, SIZE_T, trk_fit_err);
    if( result != 0 ){
        printf("Error: unexpected result from packet composition (%d)\n",result);
        return -1;
    }


    // display packet
    uint8_t* p_byte = mypacket;
    printf("Packet (len = %ld): ",mypacket_length); 
    while( p_byte < (mypacket + mypacket_length) ){
        printf("0x%02X ",*p_byte);
        ++p_byte;
    }
    printf("\n");


    // list serial ports 
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

    // open client serial port (not the tracker)
    cout << "Attempting to open " << server_port_string; 
    Serial* mySerialPort = NULL;
    try {
        mySerialPort = new Serial(server_port_string, 115200U, Timeout(50,200,3,200,3), eightbits, parity_none, stopbits_one, flowcontrol_none);
    } catch(IOException const& e){
        cout << e.what() << endl;
        return -1;
    };
    mySerialPort->flush();
    cout << "done." << endl;



    // close serial port
    cout << "Closing serial port" << endl;
    mySerialPort->close();

    // done	
    return 0;
}
