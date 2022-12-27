#include "serial/serial.h" // https://www.github.com/wjwwood/serial
#include "mak_packet.h"
#include <iostream>
using namespace std;
using namespace serial;

#define SIZE_Q 4
#define SIZE_T 3
#define MAX_PACKET_LENGTH 255
#define PKT_TYPE_TRANSFORM_DATA 0x01

// add bytes to a tracker packet
// always updates packet length
// updates CRC if requested
// stuffs DLEs if requested
int add_to_packet(uint8_t* chars_to_add, size_t num_chars_to_add, uint8_t* packet, size_t* packet_length, uint8_t max_packet_length, uint8_t* CRC, bool do_update_crc, bool do_stuff_dle){

    size_t   num_bytes_added = 0;
    uint8_t* p_in            = chars_to_add;                     // pointer to first byte in the input array that we will copy
    uint8_t* p_in_end        = chars_to_add + num_chars_to_add;  // pointer to first byte beyond end of input array
    uint8_t* p_out_start     = packet + *packet_length;          // pointer to first byte we will add to the packet
    uint8_t* p_out_end       = packet+max_packet_length;         // pointer to first byte beyond end of output array allocation
    uint8_t* p_out           = p_out_start;                      // pointer to first byte in the output array that we will write to

    // iterate through all input bytes
    while(p_in < p_in_end){

        // add byte to output as long as there is room in the output buffer allocation
        if( p_out < p_out_end ){

            // copy input byte into output (no reason to use memcpy here?)
            *p_out = *p_in;

            // update CRC
            if(do_update_crc){
                crcAddBytes(CRC,p_out,1);
            }

            // stuff DLE
            if(do_stuff_dle && (*p_out == DLE)){
                if((p_out + 1) < p_out_end){
                    ++p_out;
                    *p_out = DLE;
                    ++num_bytes_added;
                } else {
                    printf("Error stuffing DLE: insufficient packet buffer size\n");
                    return -2;
                }
            }

            // increment input, output, and num_bytes_added
            ++p_in;
            ++p_out;
            ++num_bytes_added;
        } else {
            printf("Error adding byte: insufficient packet buffer size\n");
            return -1;
        }
    }

    // update packet length
    *packet_length += num_bytes_added;
    // TODO: assert(*packet_length <= max_packet_length)
    
    // done
    return 0;
}

// build a tracker packet 
int compose_tracker_packet(float *q, size_t q_size, float* t, size_t t_size, uint8_t* packet, size_t *packet_length){

    int result;

    // grab max packet length from input before resetting
    size_t max_packet_length = *packet_length;
    *packet_length = 0;

    uint8_t temp_packet[max_packet_length] = {0x00};

    // initialize CRC8 (polynomiax = 0x07, initial value = 0x00, final xor = 0x00, unreflected)
    uint8_t CRC = 0x00;

    // add DLE, STX
    printf("Adding DLE, STX\n");
    *temp_packet = DLE;
    *(temp_packet + 1) = STX;
    if((result = add_to_packet(temp_packet, 2, packet, packet_length, max_packet_length, &CRC, false, false)) != 0)
        return result;
    printf("Done. CRC = 0x%02X\n",CRC);

    // add packet type
    printf("Adding packet type\n");
    *temp_packet = PKT_TYPE_TRANSFORM_DATA;
    if((result = add_to_packet(temp_packet, 1, packet, packet_length, max_packet_length, &CRC, true, true)) != 0)
        return result;
    printf("Done. CRC = 0x%02X\n",CRC);


    for(unsigned int qidx = 0; qidx < q_size; qidx++){
        printf("q[%d] = %8.4f\n",qidx,*(q+qidx));
    } 

    for(unsigned int tidx = 0; tidx < t_size; tidx++){
        printf("t[%d] = %8.4f\n",tidx,*(t+tidx));
    } 

    // add CRC
    if((result = add_to_packet(&CRC, 1, packet, packet_length, max_packet_length, &CRC, false, true)) != 0)
        return result;
    
    // add DLE, ETX
    *temp_packet = DLE;
    *(temp_packet + 1) = ETX;
    if(add_to_packet(temp_packet, 2, packet, packet_length, max_packet_length, &CRC, false, false) != 0)
        return -1;

    // done
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
    int result = 0;
    result = compose_tracker_packet(q,SIZE_Q,t,SIZE_T,mypacket,&mypacket_length);
    if( result != 0 ){
        printf("Error: unexpected result from packet composition (%d)\n",result);
        return -1;
    }

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
