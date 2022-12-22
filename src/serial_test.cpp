#include "serial/serial.h"
#include <iostream>
using namespace std;
using namespace serial;


int main(void){
	cout << "Hello World!" << endl;
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
		mySerialPort = new Serial("/dev/ttyUSB0", 9600U, Timeout(50,200,3,200,3), eightbits, parity_none, stopbits_one, flowcontrol_none);
	} catch(IOException const& e){
		cout << e.what() << endl;
		return -1;
	};
	mySerialPort->flush();

	// close serial port
	mySerialPort->close();

	// done	
	return 0;
}
