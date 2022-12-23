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
#incldue "serial/serial.h"   //https://github.com/wjwwood/serial
#include <math.h>
#include <util/delay.h>

// application definitions
#define PKT_TYPE_ST_DATA 		0x01
#define PKT_TYPE_BOSCH_DATA 	0x02
#define PKT_TYPE_ST_CONFIG		0xA1
#define PKT_TYPE_BOSH_CONFIG	0xA2
#define PKT_TYPE_ENCODER_DATA	0xAA
#define DLE 					0x10
#define STX						0x02
#define ETX						0x03
#define CONFIG_MSG_DT   		10    // period of configuration message delivery (# of overflows of TCC0)


/* * * * * Function Prototypes * * * * */


// compute CRC value from an array of bytes (can be one byte in length)
uint8_t crcAddBytes(uint8_t *CRC, uint8_t *byteArray, uint16_t numBytes);

// write a byte to the serial port, twice in the case of a DLE byte
uint8_t usartWriteDLEStuff(USART_t * usartPort, char out);

// send a packet of IMU measurement or configuration data out over the serial port
uint8_t sendIMUSerialPacket(uint8_t packetType, uint16_t microTime, uint8_t *imuTimeBytes, uint8_t imuTimeBytesSize, uint8_t *imuDataBytes, uint8_t imuDataBytesSize);

// conversion functions for reference and debugging
// not for use in actual application (do conversions on PC side)
float uint16_to_float16(uint16_t uint16_value);


#endif
