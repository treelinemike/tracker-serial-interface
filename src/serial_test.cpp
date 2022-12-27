#include "serial/serial.h" // https://www.github.com/wjwwood/serial
#include "mak_packet.h"
#include <iostream>
using namespace std;
using namespace serial;

#define SIZE_Q 4
#define SIZE_T 3
#define MAX_PACKET_LENGTH 255

// 
int compose_tracker_packet(float *q, size_t q_size, float* t, size_t t_size, uint8_t* packet, size_t *packet_length){

    // grab max packet length from input before resetting
    size_t max_packet_length = *packet_length;
    *packet_length = 0;


    for(unsigned int qidx = 0; qidx < q_size; qidx++){
       printf("q[%d] = %8.4f\n",qidx,*(q+qidx));
    } 
    
    for(unsigned int tidx = 0; tidx < t_size; tidx++){
       printf("t[%d] = %8.4f\n",tidx,*(t+tidx));
    } 
    
    
    *packet_length = 123;
    return 0;
}

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
    float q[SIZE_Q] = {1.2,2.3,3.4,4.5};
    float t[SIZE_T] = {5.6,6.7,7.8};
    uint8_t mypacket[MAX_PACKET_LENGTH] = {0x00};
    size_t mypacket_length = MAX_PACKET_LENGTH;
    compose_tracker_packet(q,SIZE_Q,t,SIZE_T,mypacket,&mypacket_length);
    printf("New packet length: %ld\n",mypacket_length);


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


    // try to write something to serial port
    mySerialPort->write("Hello, world!\r\n");


    // test some functions
    usartWriteDLEStuff(mySerialPort,'A');
    usartWriteDLEStuff(mySerialPort,DLE);
    usartWriteDLEStuff(mySerialPort,'\r');
    usartWriteDLEStuff(mySerialPort,'\n');

	// close serial port
    cout << "Closing serial port" << endl;
	mySerialPort->close();

	// done	
	return 0;
}
