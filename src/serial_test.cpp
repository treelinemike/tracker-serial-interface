#include "serial/serial.h"
#include "mak_packet.h"
#include <iostream>
using namespace std;
using namespace serial;

// 
int compose_tracker_packet(char *q, char* t, char* packet, int packet_length){

    return 0;
}

int main(void){
	cout << "Hello World!" << __linux__ << endl;
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

    // test conversion
    //double
    static_assert(sizeof(float)==4,"Float size is not four bytes!"); 
    cout << "Size of float: " << sizeof(float)<< endl;
    static float test_float = 1.254;
    constexpr uint8_t* p_fval = (uint8_t*) &test_float;
    printf("0x%02X%02X%02X%02X\n", *p_fval, *(p_fval+1), *(p_fval+2), *(p_fval+3));
    static_assert(*p_fval == 12,"Not a litle endian system!");

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
