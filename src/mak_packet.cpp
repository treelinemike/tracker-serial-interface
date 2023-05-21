/* mak_packet.cpp
 *
 * Custom serial packet definitions and associated functions.
 *
 * Updated:  19-May-2023
 * Author: 	 M. Kokko
 *
 */

// includes
#include "mak_packet.h"

// add bytes to a tracker packet
// always updates packet length
// updates CRC if requested
// stuffs DLEs if requested
int add_bytes_to_packet(uint8_t* chars_to_add, size_t num_chars_to_add, uint8_t* packet, size_t* packet_length, uint8_t max_packet_length, uint8_t* CRC, bool update_crc_flag, bool stuff_dle_flag){

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
            if(update_crc_flag){
                crcAddBytes(CRC,p_out,1);
            }

            // stuff DLE
            if(stuff_dle_flag && (*p_out == DLE)){
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
int compose_tracker_packet(uint8_t* packet, size_t *packet_length, uint32_t frame_num, std::vector<tform>& tforms){

    int result;

    // grab max packet length from input before resetting
    size_t max_packet_length = MAX_PACKET_LENGTH;
    *packet_length = 0;

    uint8_t temp_packet[MAX_PACKET_LENGTH] = {0x00};

    // initialize CRC8 (polynomial = 0x07, initial value = 0x00, final xor = 0x00, unreflected)
    uint8_t CRC = 0x00;

    // add DLE, STX
    //printf("Adding DLE, STX\n");
    *temp_packet = DLE;
    *(temp_packet + 1) = STX;
    if((result = add_bytes_to_packet(temp_packet, 2, packet, packet_length, max_packet_length, &CRC, false, false)) != 0)
        return result;
    //printf("Done. CRC = 0x%02X\n",CRC);

    // add packet type
    //printf("Adding packet type\n");
    *temp_packet = PKT_TYPE_TRANSFORM_DATA;
    if((result = add_bytes_to_packet(temp_packet, 1, packet, packet_length, max_packet_length, &CRC, true, true)) != 0)
        return result;
    //printf("Done. CRC = 0x%02X\n",CRC);

    // add number of transforms
    *temp_packet = tforms.size();
    if((result = add_bytes_to_packet(temp_packet, 1, packet, packet_length, max_packet_length, &CRC, true, true)) != 0)
        return result;

    // add frame number
    //printf("Adding frame number\n");
    if((result = add_bytes_to_packet((uint8_t*)(&frame_num), 4, packet, packet_length, max_packet_length, &CRC, true, true)) != 0)
        return result;
    //printf("Done. CRC = 0x%02X\n",CRC);


    // add each transform
    for( auto & thistf : tforms ){

        // array for transform data
        uint8_t newdata[36] = {0};

        // memcpy transform data into the array
        memcpy(newdata,&(thistf.id),4);
        memcpy(newdata+4,&(thistf.q0),4);
        memcpy(newdata+8,&(thistf.q1),4);
        memcpy(newdata+12,&(thistf.q2),4);
        memcpy(newdata+16,&(thistf.q3),4);
        memcpy(newdata+20,&(thistf.t0),4);
        memcpy(newdata+24,&(thistf.t1),4);
        memcpy(newdata+28,&(thistf.t2),4);
        memcpy(newdata+32,&(thistf.error),4);

        // add the array to the current packet with DLE stuffing and CRC computation
        if((result = add_bytes_to_packet(newdata,36, packet, packet_length, max_packet_length, &CRC, true, true)) != 0)
            return result;
    }


    // add CRC
    if((result = add_bytes_to_packet(&CRC, 1, packet, packet_length, max_packet_length, &CRC, false, true)) != 0)
        return result;
    
    // add DLE, ETX
    *temp_packet = DLE;
    *(temp_packet + 1) = ETX;
    if((result = add_bytes_to_packet(temp_packet, 2, packet, packet_length, max_packet_length, &CRC, false, false)) != 0)
        return result;

    // done
    return 0;
}


// compute CRC value from an array of bytes (can be one byte in length)
uint8_t crcAddBytes(uint8_t *CRC, uint8_t *byteArray, uint16_t numBytes){

	uint8_t bitNum, thisBit, doInvert;
	uint8_t byteNum = 0;
	uint8_t *pInputByte = byteArray;

	// cycle through all bytes in input string
	while(byteNum < numBytes){

		// cycle through all bits in current byte, starting with the most significant bit
		for(bitNum = 8; bitNum > 0; bitNum--){
			thisBit = !(0 == (*pInputByte & (1<<(bitNum-1))));	// input bit
			doInvert = thisBit ^ ((*CRC & 0x80) >> 7);			// new bit0
			*CRC = *CRC << 1;									// bitshift left by 1 position
			if(doInvert){
				*CRC = *CRC ^ 0b00000111;						// if input bit is 1, invert using x^2 + x + 1 polynomial (0x7)
			}
		}

		// increment input byte counter and pointer
		++byteNum;
		++pInputByte;
	}

	// return, no error handling
	return 0;
}
