#include <stdio.h>
#include <stdint.h>

#define DLE 0x10
#define STX 0x02
#define ETX 0x03
#define BYTE_BUFFER_LENGTH 1024
#define PACKET_BUFFER_LENGTH 255

int main(){

    uint8_t byte_buffer[BYTE_BUFFER_LENGTH] = {0x10, 0x10, 0x00, 0x10, 0x10, 0x10,0x02,0x12,0x22,0xCD,0x12,0x13,0x10,0x03,0xFF,0xCD};
    uint8_t* p_byte = byte_buffer + 14; // keep this pointing to one byte past end of data we've written into the buffer
    uint8_t* p_byte_end = byte_buffer + BYTE_BUFFER_LENGTH;
    uint8_t* p_start_search, *p_stop_search, *p_byte_copy;
    uint8_t start_found_flag, end_found_flag, crc_verified_flag;
    uint8_t DLE_count = 0;
    uint8_t* p_display = byte_buffer;
    uint32_t packet_length, bytes_to_copy;
    uint8_t packet_buffer[PACKET_BUFFER_LENGTH] = {0};
    uint8_t *p_packet = packet_buffer;
    uint8_t *p_packet_end = packet_buffer + PACKET_BUFFER_LENGTH;
    uint8_t CRC_received, CRC_computed; 

    // num_bytes_written = mySerialPart->(p_byte,p_byte_end-p_byte)
    // p_byte += num_bytes_written;

    
    // reset all of our variables
    p_start_search = byte_buffer;
    DLE_count = 0;
    start_found_flag = 0;
    end_found_flag = 0;
    packet_length = 0;
    crc_verified_flag = 0;
    
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
        printf("Start not found!\n");
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
        printf("p_byte is %ld bytes ahead of buffer start\n",p_byte-byte_buffer);
    } else {
        printf("Found start\n");
        //back up pointer to the DLE
        --p_start_search;

        /* DEBUG */   
        printf("Byte buffer from start of message: ");
        p_display = p_start_search;
        while(p_display < p_byte){
            printf("0x%02X ",*p_display);
            ++p_display;
        }
        printf("\n");
        /**/
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

        /* DEBUG */
        if(end_found_flag){
            printf("Found end\n");
        } else {
            printf("No end found\n");
        }
        /**/
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
                return -1;
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

        /* DEBUG */
        // now we have a complete message
        // display it
        p_display = packet_buffer;
        printf("Unstuffed packet: ");
        while(p_display < (packet_buffer + packet_length)){
            printf("0x%02X ",*p_display);
            ++p_display;
        }
        printf("\n");
        /**/

        // reset byte buffer
        // i.e. copy everything past message we extracted back to the beginning of the buffer
        p_byte_copy = byte_buffer;
        while(p_stop_search < p_byte){
            *p_byte_copy = *p_stop_search;
            ++p_byte_copy;
            ++p_stop_search;
        }
        p_byte = p_byte_copy;
    
        /* DEBUG */
        // now show remaining buffer past the packet we just extracted
        p_display = byte_buffer;
        printf("Remaining bytes in byte_buffer: ");
        while(p_display < p_byte){
            printf("0x%02X ",*p_display);
            ++p_display;
        }
        printf("\n");
        /**/

    }

    // now we have an unstuffed packet
    // check CRC
    if(packet_length > 0){
        
        if(packet_length < 6){
            printf("Error: packet not long enough to have a CRC and length byte"); // length = 4 would be {DLE,STX,DLE,ETX}
            return -1;
        }

        uint8_t CRC8_received = *(packet_buffer + packet_length -3);
        printf("CRC8 received: 0x%02X\n",CRC8_received);

    }

    // 

    // done
    return 0;
}

