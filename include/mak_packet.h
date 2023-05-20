/* mak_packet.h
 *
 * Custom serial packet definitions.
 *
 * Updated:  2022-12-22
 * Author: 	 M. Kokko
 *
 */

#ifndef MAK_PACKET_H_
#define MAK_PACKET_H_

// includes
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#include <vector>
#include <cstring>

// application definitions
#define PKT_TYPE_TRANSFORM_DATA   0x01
#define PKT_LENGTH_TRANSFORM_DATA 43
#define PKT_TYPE_ACK              0x06
#define PKT_TYPE_TRK_START        0x11
#define PKT_TYPE_TRK_STOP         0x12
#define PKT_TYPE_GET_PROBE_TFORM  0x13
#define PKT_TYPE_NAK			  0x15
#define DLE 					  0x10
#define STX						  0x02
#define ETX						  0x03
#define SIZE_Q                    4
#define SIZE_T                    3
#define MAX_PACKET_LENGTH         255

// struct for transform data
struct tform{
    uint32_t id;
    float q0,q1,q2,q3,t0,t1,t2,error;
};

/* * * * * Function Prototypes * * * * */

// build a tracker transform data packet
int compose_tracker_packet(uint8_t* packet, size_t *packet_length, uint32_t frame_num, std::vector<tform>& tforms);

// add some number of bytes to a byte array serial packet
// stuffing DLE and/or updating CRC if required
int add_bytes_to_packet(uint8_t* chars_to_add, size_t num_chars_to_add, uint8_t* packet, size_t* packet_length, uint8_t max_packet_length, uint8_t* CRC, bool update_crc_flag, bool stuff_dle_flag);

// compute CRC value from an array of bytes (can be one byte in length)
uint8_t crcAddBytes(uint8_t *CRC, uint8_t *byteArray, uint16_t numBytes);

// conversion functions for reference and debugging
// not for use in actual application (do conversions on PC side)
float uint16_to_float16(uint16_t uint16_value);


#endif
