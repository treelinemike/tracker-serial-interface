#include "serial/serial.h" // https://www.github.com/wjwwood/serial
#include "mak_packet.h"
#include <iostream>
#include <fstream>
#include <unistd.h>
#include <sys/ioctl.h>
#include <string>
#include <thread>
#include <mutex>
#include <cassert>

#define SERVER_PORT_DESC "Prolific Technology"
#define REQUIRE_SERVER false
#define USE_STATIC_PORTS true
#define STATIC_PORT_SERVER "/dev/ttyUSB1"
//#define STATIC_PORT_SERVER "/dev/cu.usbserial-FTD31G87"

#define BAUDRATE 115200U

#define BYTE_BUFFER_LENGTH 2048
#define PACKET_BUFFER_LENGTH 255

#define assertm(exp, msg) assert(((void)msg, exp))

using namespace std;
using namespace serial;

// simple class for representing a serial message
class SimpleMsg {
    private:
        uint8_t msg_type;
        size_t msg_size;
        uint8_t *msg;
    public:
        void set_msg(uint8_t mtype, size_t msize, uint8_t* mdata){
            this->msg_type = mtype;
            this->msg_size = msize;
            this->msg = (uint8_t*) malloc(msize);
            memcpy(this->msg, mdata, msize);
        }
        uint8_t get_msg_type(){
            return this->msg_type;
        }
        uint8_t* get_msg_buffer(){
            return this->msg;
        }
        size_t get_msg_size(){
            return this->msg_size;
        }
};

// vector to store messages and mutex to lock it
std::vector<SimpleMsg> msg_buffer;
std::mutex msg_lock;

// function to read from serial port, to be run in dedicated thread
void monitor_serial_port(Serial* mySerialPort){

    uint8_t byte_buffer[BYTE_BUFFER_LENGTH] = {0};
    size_t bytes_read;
    uint8_t *p_byte_end = byte_buffer + BYTE_BUFFER_LENGTH;
    uint8_t *p_byte, *p_start_search, *p_stop_search, *p_byte_copy;
    uint8_t start_found_flag, end_found_flag, crc_verified_flag;
    uint8_t DLE_count = 0;
    uint32_t packet_length;
    uint8_t packet_buffer[PACKET_BUFFER_LENGTH] = {0};
    uint8_t *p_packet = packet_buffer;
    uint8_t *p_packet_end = packet_buffer + PACKET_BUFFER_LENGTH;
    uint8_t CRC_received, CRC_computed;
    uint8_t packet_type;

    // start adding to beginning of byte buffer
    p_byte = byte_buffer;

    // loop forever, capturing bytes from serial stream and
    // processing packets when they are identified
    printf("Parsing data from serial port...\n");
    while(1){

        // reset all parsing variables
        p_start_search = byte_buffer;
        DLE_count = 0;
        start_found_flag = 0;
        end_found_flag = 0;
        packet_length = 0;
        crc_verified_flag = 0;

        // read all available bytes from serial port
        // TODO: this will need to be updated for implementation on a microcontroller
        bytes_read = mySerialPort->read(p_byte,p_byte_end-p_byte);
        p_byte += bytes_read;

        // try to position p_start_search at DLE byte in DLE STX 
        while( !start_found_flag && p_start_search < p_byte) {
            while(*p_start_search == DLE){
                ++DLE_count;
                ++p_start_search;
            }
            if( (DLE_count % 2 == 1) && *p_start_search == STX ){
                start_found_flag = 1;
            } else {
                ++p_start_search;
                DLE_count = 0;
            }
        }

        // adjust p_byte or p_start_search
        if( !start_found_flag ){
            //printf("Start not found!\n");
            // we didn't find a start, so clear byte buffer
            // unless last byte is DLE which could be part of a start byte
            if(p_byte > byte_buffer){
                if( *(p_byte-1) == DLE ){
                    *(byte_buffer) = DLE;
                    p_byte = byte_buffer + 1;
                } else {
                    p_byte = byte_buffer;
                }
            }
            //printf("p_byte is %ld bytes ahead of buffer start\n",p_byte-byte_buffer);
        } else {
            //printf("Found start\n");
            //back up pointer to the DLE
            --p_start_search;

            /* DEBUG  
               printf("Byte buffer from start of message: ");
               p_display = p_start_search;
               while(p_display < p_byte){
               printf("0x%02X ",*p_display);
               ++p_display;
               }
               printf("\n");
             */
        }

        // try to find DLE ETX
        // in a new block to reduce nesting
        if(start_found_flag){
            p_stop_search = p_start_search;
            DLE_count = 0;

            while( !end_found_flag && (p_stop_search < p_byte)) {
                while(*p_stop_search == DLE){
                    ++DLE_count;
                    ++p_stop_search;
                }
                if( (DLE_count % 2 == 1) && (*p_stop_search == ETX) ){
                    end_found_flag = 1;
                } else {
                    ++p_stop_search;
                    DLE_count = 0;
                }
            }

            /* DEBUG
               if(end_found_flag){
               printf("Found end\n");
               } else {
               printf("No end found\n");
               }
             */
        }

        // now we have a full message
        // so copy it out to message buffer and reset the byte buffer
        // while copying to byte buffer un-stuff DLEs
        if(end_found_flag){

            // increment p_stop_search so it points to one byte past ETX
            ++p_stop_search;

            // copy each byte out
            p_packet = packet_buffer;
            p_byte_copy = p_start_search;
            while(p_byte_copy < p_stop_search){

                // make sure we have enough room to copy another byte into the packet buffer
                if(p_packet >= p_packet_end){
                    printf("Error - insufficient message buffer length\n");
                    return;
                }

                // copy this byte in
                *p_packet = *p_byte_copy;
                ++packet_length;

                // increment packet buffer pointer
                ++p_packet;

                // increment byte buffer pointer, un-stuffing DLEs
                if((*p_byte_copy == DLE) && ((p_byte_copy+1) < p_byte_end) && (*(p_byte_copy+1) == DLE) ){
                    p_byte_copy = p_byte_copy +2;
                } else {
                    ++p_byte_copy;
                }
            }

            /* DEBUG
            // now we have a complete message
            // display it
            p_display = packet_buffer;
            printf("Unstuffed packet: ");
            while(p_display < (packet_buffer + packet_length)){
            printf("0x%02X ",*p_display);
            ++p_display;
            }
            printf("\n");
             */

            // reset byte buffer
            // i.e. copy everything past message we extracted back to the beginning of the buffer
            p_byte_copy = byte_buffer;
            while(p_stop_search < p_byte){
                *p_byte_copy = *p_stop_search;
                ++p_byte_copy;
                ++p_stop_search;
            }
            p_byte = p_byte_copy;

            /* DEBUG
            // now show remaining buffer past the packet we just extracted
            p_display = byte_buffer;
            printf("Remaining bytes in byte_buffer: ");
            while(p_display < p_byte){
            printf("0x%02X ",*p_display);
            ++p_display;
            }
            printf("\n");
             */

        }

        // now we have an unstuffed packet
        // check CRC
        if(packet_length > 0){

            // ensure packet is at least the minimum length required
            if(packet_length < 6){
                printf("Error: packet not long enough to have a CRC and length byte"); // length = 4 would be {DLE,STX,DLE,ETX}
            return;
            }

            // extract CRC from packet
            CRC_received = *(packet_buffer + packet_length -3);

            // compute CRC
            CRC_computed = 0x00;
            crcAddBytes(&CRC_computed,packet_buffer+2,packet_length-5); // adjust to omit DLE, STX, ETX, and CRC8 bytes

            // check CRC
            //printf("CRC8 received: 0x%02X, computed: 0x%02X\n",CRC_received,CRC_computed);
            if(CRC_received != CRC_computed){
                printf("Error: CRC8 mismatch, discarding packet\n");
            } else {
                crc_verified_flag = 1;
            }
        }

        // process the packet we've received
        if(crc_verified_flag){

            // get packet type
            packet_type = *(packet_buffer + 2);

            //printf("Received packet of type 0x%02X and length %d\n",packet_type,packet_length);

            // create a new message object and put it into vector           
            SimpleMsg new_msg;
            new_msg.set_msg(packet_type,packet_length,packet_buffer);
            msg_lock.lock();
            msg_buffer.push_back(new_msg);
            msg_lock.unlock();


        }
    }
    return;
}



int main(void){

    uint8_t *packet_buffer = nullptr;

    ofstream outfile;
    outfile.open("output.csv");



    // ensure that we're on a linux or darwin (apple) system
    // TODO: handle Windows, etc.
#ifdef __linux__
    printf("Running on linux\n");
#elif __APPLE__
    printf("Running on darwin\n");
#else
    printf("Not running on linux or darwin, exiting.\n");
    return -1;
#endif

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
    cout << "Attempting to open " << server_port_string << "... "; 
    Serial* mySerialPort = NULL;
    try {
        mySerialPort = new Serial(server_port_string, BAUDRATE, Timeout(1,25,0,0,0), eightbits, parity_none, stopbits_one, flowcontrol_none);
    } catch(IOException const& e){
        cout << e.what() << endl;
        return -1;
    };
    mySerialPort->flush();
    cout << "done." << endl;

    // start reading from the serial port in a separate thread
    std::thread serial_thread (monitor_serial_port,mySerialPort);


    // process packets
    while(1){
        if(msg_buffer.size() > 0){


            // get the first element in the vector
            // do this quickly so the read thread can keep accessing the vector
            msg_lock.lock();
            assertm( msg_buffer.size() > 0, "Vector length changed while acquirng lock.." );
            SimpleMsg this_msg = msg_buffer[0];
            msg_buffer.erase(msg_buffer.begin());
            msg_lock.unlock();

            packet_buffer = this_msg.get_msg_buffer();

            switch(this_msg.get_msg_type()){

                case PKT_TYPE_GET_PROBE_TFORM:
                    uint16_t data_size;
                    uint32_t probe_sn;

                    // check for correct data length
                    if(this_msg.get_msg_size() < 5){
                        printf("Error: incorrect packet length, discarding packet\n");
                    }
                    memcpy(&data_size,packet_buffer+3,2);
                    if( data_size != 4 ){
                        printf("Error: incorrect payload data size, discarding packet\n");
                        break;
                    }

                    // get probe serial number
                    if(this_msg.get_msg_size() < (long unsigned int)(8+data_size)){
                        printf("Error: incorrect packet length, discarding packet\n");
                    }
                    memcpy(&probe_sn,packet_buffer+5,4);
                    printf("Requested transform for probe s/n: 0x%08X\n",probe_sn);

                    break;


                case PKT_TYPE_TRANSFORM_DATA:
                    if(this_msg.get_msg_size() != PKT_LENGTH_TRANSFORM_DATA){
                        printf("Error: incorrect packet length, discarding packet\n");
                        break;
                    }


                    uint32_t frame_num;
                    float q0,q1,q2,q3,tx,ty,tz;

                    memcpy(&frame_num,packet_buffer+3,4);
                    memcpy(&q0,packet_buffer+8,4);
                    memcpy(&q1,packet_buffer+12,4);
                    memcpy(&q2,packet_buffer+16,4);
                    memcpy(&q3,packet_buffer+20,4);
                    memcpy(&tx,packet_buffer+24,4);
                    memcpy(&ty,packet_buffer+28,4);
                    memcpy(&tz,packet_buffer+32,4);

                    printf("Frame %5d; q = (%+07.4f, %+07.4f, %+07.4f, %+07.4f); t = (%+8.2f, %+8.2f, %+8.2f)\n",frame_num, q0, q1, q2, q3, tx, ty, tz);

                    outfile << tx << "," << ty << "," << tz << endl;

                    break;

                default:
                    printf("Unsupported packet type, discarding packet\n");
            }



        }
    }

    serial_thread.join(); 

    // close serial port
    // TODO: we'll never get here, would be nice to catch CTRL+C, but this isn't trivial
    mySerialPort->close();

    // done
    return 0;

}
