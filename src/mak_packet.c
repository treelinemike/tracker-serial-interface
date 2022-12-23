/* mak_packet.h
 *
 * Custom serial packet definitions.
 *
 * Updated:  2022-12-22
 * Author: 	 M. Kokko
 *
 */

// includes
#include "serial/serial.h"
#include "mak_packet.h"

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
				*CRC = *CRC ^ 0b00110001;						// if input bit is 1, invert using x^8 + x^5 + x^4 + 1 polynomial
			}
		}

		// increment input byte counter and pointer
		++byteNum;
		++pInputByte;
	}

	// return, no error handling
	return 0;
}

// send a byte, stuffing an extra DLE character if necessary
uint8_t usartWriteDLEStuff(serial::Serial* port, char out){

	// write character
    port->write((const uint8_t*)&out,1);

	// repeat if character is DLE
	if(out == DLE){
		port->write((const uint8_t*)&out,1);
	}

	return 0;
}

// send a binary (non-ASCII) serial message with IMU measurement or configuration data
uint8_t sendIMUSerialPacket(serial::Serial* port, uint8_t packetType, uint16_t microTime, uint8_t *imuTimeBytes, uint8_t imuTimeBytesSize, uint8_t *imuDataBytes, uint8_t imuDataBytesSize){

	uint8_t i, microTimeByte;
	uint8_t CRC = 0x00;
    uint8_t this_byte = 0x00;

	// send start sequence
    this_byte = DLE;
    port->write((const uint8_t*)&this_byte,1);
    this_byte = STX;
    port->write((const uint8_t*)&this_byte,1);

	// send packet type
	usartWriteDLEStuff(port, packetType);
	crcAddBytes(&CRC,&packetType,1);

	// send MICRO time (low byte first)
	microTimeByte = (uint8_t)((microTime & 0x00FF));
	usartWriteDLEStuff(port, microTimeByte);
	crcAddBytes(&CRC,&microTimeByte,1);

	microTimeByte = (uint8_t)((microTime & 0xFF00) >> 8);
	usartWriteDLEStuff(port, microTimeByte);
	crcAddBytes(&CRC,&microTimeByte,1);

	// send IMU time (low byte first)
	// these bytes come directly from chip so we read
	// them out of an array rather than from a 24- or 32-bit
	// unsigned integer
	for(i = 0; i < imuTimeBytesSize; i++){
		usartWriteDLEStuff(port, imuTimeBytes[i]);
	}
	crcAddBytes(&CRC,imuTimeBytes,imuTimeBytesSize);

	// send actual IMU data
	// these data bytes come directly from IMU and are just passed on without processing
	for(i = 0; i < imuDataBytesSize; i++){
		usartWriteDLEStuff(port, imuDataBytes[i]);
	}
	crcAddBytes(&CRC,imuDataBytes,imuDataBytesSize);

	// send CRC8 checksum
	usartWriteDLEStuff(port, CRC);

	// send end sequence
    this_byte = DLE;
    port->write((const uint8_t*)&this_byte,1);
    this_byte = ETX;
    port->write((const uint8_t*)&this_byte,1);

	// done
	return 0;
}


// convert two bytes to half precision float
// this function is for debug purposes only, data should not
// be converted on embedded device
float uint16_to_float16(uint16_t uint16_value){
	float float16_value = 0x0000;
	float signbit  = (float)((uint16_value & 0x8000)>>15);
	float exponent = (float)((uint16_value & 0x7C00)>>10);
	float mantissa = (float)((uint16_value & 0x03FF)>>0);

	float signfactor = pow(-1,signbit);

	if(exponent == 0){
		if(mantissa == 0){
			float16_value = (signfactor) * 0;
		} else {
			float16_value = (float)(signfactor * (pow(2,-14)) * (0 + mantissa*pow(2,-10)));
		}
	} else if(exponent == 31){
		if(mantissa == 0){
			float16_value = (signfactor) * INFINITY;
		} else {
			float16_value = NAN;
		}
	} else {
		float16_value = (double)(signfactor * pow(2,(exponent-15)) * (1+mantissa*pow(2,-10)));
	}
	return float16_value;
}
