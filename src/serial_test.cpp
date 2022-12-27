#include "serial/serial.h" // https://www.github.com/wjwwood/serial
#include "mak_packet.h"
#include <iostream>
using namespace std;
using namespace serial;


int main(void){

    // ensure linux system
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
    vector<PortInfo> all_ports = list_ports();
    if (all_ports.size() > 0) {
        cout << "Available COM ports:" << endl;
        for (unsigned int i = 0; i < all_ports.size(); i++) {
            cout << "* " << all_ports[i].port << " (" << all_ports[i].description << " / " << all_ports[i].hardware_id << ")" << endl;
        }
        cout << endl;
    }

    // open serial port
    cout << "Attempting to open /dev/ttyUSB0..." << endl;
    Serial* mySerialPort = NULL;
    try {
        mySerialPort = new Serial("/dev/ttyUSB0", 115200U, Timeout(50,200,3,200,3), eightbits, parity_none, stopbits_one, flowcontrol_none);
    } catch(IOException const& e){
        cout << e.what() << endl;
        return -1;
    };
    mySerialPort->flush();


    // write packet to serial port
    mySerialPort->write(mypacket,mypacket_length);


    // close serial port
    cout << "Closing serial port" << endl;
    mySerialPort->close();

    // done	
    return 0;
}
