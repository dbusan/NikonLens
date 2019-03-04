/*
 * NikonLens.cpp
 *
 *  Author: D Busan
 *  Created: 2019/03/04 
 *  
 *  contact@dbusan.com
 * 
 * 
 *  Original Copyright notice:
 *  Created: 11/14/2013 02:45:51
 *
 *  Lain A.
 *  code@hacktheinter.net
 *  http://hacktheinter.net
 *
 *  Copyright (c) 2013 Lain A.
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a copy
 *  of this software and associated documentation files (the "Software"), to deal
 *  in the Software without restriction, including without limitation the rights
 *  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *  copies of the Software, and to permit persons to whom the Software is
 *  furnished to do so, subject to the following conditions:
 *
 *  The above copyright notice and this permission notice shall be included in
 *  all copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 *  THE SOFTWARE.
 */

#include <Arduino.h>
#include <SPI.h>

#include <numeric.h>

#include "NikonLens.h"

namespace lens
{

NikonLens_Class NikonLens;

void NikonLens_Class::begin(
	const u8 handshakePin_In,
	const u8 handshakePin_Out)
{
	// initialise input and output buffer
	for (u8 i = 0; i < 255; i++)
	{
		if (i < 8)
			outputBuffer = 0;
		inputBuffer[i] = 0;
	}
	inputBuffer_length = 0;
	outputBuffer_length = 0;

	// m_handshakePin_In  = handshakePin_In;
	// m_handshakePin_Out = handshakePin_Out;

	// Initialise SPI 
	m_handshakePin_In = 3;
	m_handshakePin_Out = 4;

	pinMode(m_handshakePin_In, INPUT_PULLUP);
	pinMode(m_handshakePin_Out, OUTPUT);

	SPI.setBitOrder(LSBFIRST);
	// SCLK idles high, data is valid on rising edge
	SPI.setDataMode(SPI_MODE3);

	#if F_CPU == 16000000L
		SPI.setClockDivider(SPI_CLOCK_DIV128);
	#else
		#error "FIXME for your F_CPU"
	#endif
		SPI.begin();

	// when booting, aperture will be set to F1_4 initially
	// set aperture to f1_4
}

void NikonLens_Class::end()
{
	SPI.end();
	//todo: should we do anything with the handshake pins?
}


NikonLens_Class::tResultCode NikonLens_Class::sendCommand(
	const u8 cmd,
	const u8 byteCountFromLens,
	u8 *const bytesFromLens,
	const u8 byteCountToLens,
	u8 const *const bytesToLens)
{
	assertHandshake(100 /*us*/);
	//todo: Wait 1.6ms for lens to ack, should use interrupts
	while (!isHandshakeAsserted())
		;

	// the command is ~ - notted - because the MOSI pin is connected to data line of lens
	// through the base -> collector of a transistor
	SPI.transfer(~cmd);

	// TODO: check assertion timeouts
	while (isHandshakeAsserted())
		;

	// Receive data from lens, if any
	for (u8 i = 0; i < byteCountFromLens; i++)
	{
		//TODO: Wait 5ms for lens to assert H/S

		long int assert_time = micros();

		// TODO: if assert time is greater than 5000 should stop receiving.
		while ((micros() - assert_time < 5000) && (!isHandshakeAsserted()))
			;

		// while(!isHandshakeAsserted());
		bytesFromLens[i] = SPI.transfer(0x00);
		//todo: Wait 5ms (?) for lens to release H/S
		while (isHandshakeAsserted())
			;

		// assert_time = micros();
		// while ((micros() - assert_time < 5000) && (isHandshakeAsserted()));
	}

	// Send data to lens, if any
	for (u8 i = 0; i < byteCountToLens; i++)
	{
		//todo: Wait 5ms for lens to assert H/S
		// while(!isHandshakeAsserted());

		long int assert_time = micros();
		while ((micros() - assert_time < 5000) && (!isHandshakeAsserted()))
			;
		SPI.transfer(~bytesToLens[i]);
		//todo: Wait 5ms (?) for lens to release H/S
		// while(isHandshakeAsserted());
		assert_time = micros();
		// while ((micros() - assert_time < 5000) && (isHandshakeAsserted()));
		while (isHandshakeAsserted())
			;
	}

	return Success;
}

/**
 * NikonLens::setAperture
 * 
 * @param aperture - aperture value as defined by the ApertureValue enum
 * @return NikonLens_Class::tResultCode - success or timeout
 */
NikonLens_Class::tResultCode NikonLens_Class::setAperture(
	ApertureValue aperture
)
{
	tResultCode result = Success;

	// series of commands required to set aperture


	return result;
}

// privates
void NikonLens_Class::assertHandshake(u16 microseconds)
{
	digitalWrite(m_handshakePin_Out, 1);
	delayMicroseconds(microseconds);
	digitalWrite(m_handshakePin_Out, 0);
}


void NikonLens_Class::initLens()
{
	// Initialisation sequences has 3 initial commands: 0x40, 0x41 and 0x40.
	// 0x40 receives 7 bytes and sends 2 bytes. This initialises the lens at 
	// maximum comms speed.

	// 0x41 receives 2 bytes. These should match the 2 bytes sent by 0x40. 
	// This confirms that the maximum comms speed was set.

	// 0x40 receives 7 bytes and sends 2 bytes. Identical to first command.
	// The main difference lies in the comms speed, now set to 153.6 kHz.


	// cmd byte 0x40. receive 7 bytes send 2 bytes - 2 bytes correspond 
	// to lens or cam body.
	u8 _cmd_byte = 0x40;
	u8 initPayload[2] = {0x02, 0x21};
	// send byte
	NikonLens.sendCommand(_cmd_byte, 7, inputBuffer, 2, initPayload);
	// recv 2 bytes from 0x41 - should match 2 byte send payload from 0x40
	_cmd_byte = 0x41;
	NikonLens.sendCommand(_cmd_byte, 2, inputBuffer, 0, nullptr);
	// cmd byte 0x40. Recv 7 bytes send 2 bytes. 
	_cmd_byte = 0x40;
	NikonLens.sendCommand(_cmd_byte, 7, inputBuffer, 2, initPayload);


	// CMD_GET_INFO - 0x28
	// sends the 0x28 byte then retrieves 44 bytes from the lens.
	// commands 0x22, 0x26 and others retrieve a subset of 0x28.
	_cmd_byte = CMD_GET_INFO;
	NikonLens.sendCommand(_cmd_byte, 44, inputBuffer, 0, nullptr);
	// delay is to match the timing observed between camera and lens original
	// communication protocol.
	delay(3);

	// 0xE7, 0xEA and 0xC2 are commands that are constantly sent by the 
	// camera to the lens with 10-20ms delays between them. This cycle
	// is peppered with CMD_GET_INFO commands every 4-5 of these repeating
	// commands.
	// 0xE7 and 0xEA send 1 byte as payload and do not receive any data.
	// 0xC2 receives 4 bytes.

	// Currently I do not know what they do exactly, however the lens needs them
	// for the aperture/focus to work properly.
	_cmd_byte = 0xE7;
	initPayload[0] = 0x51;
	NikonLens.sendCommand(_cmd_byte, 0, nullptr, 1, initPayload);
	delay(3);
	
	_cmd_byte = 0xC2;
	NikonLens.sendCommand(_cmd_byte, 4, inputBuffer, 0, nullptr);
	delay(3);

	// SET APERTURE
	// should set aperture to fully open. Unsure why 0xFF 0xFF does it as the actual
	// aperture values are different. However this matches the lens init procedure
	// observed between the camera and the lens
	_cmd_byte = CMD_SET_APERTURE;
	initPayload[0] = 0xFF;
	initPayload[1] = 0xFF;
	NikonLens.sendCommand(_cmd_byte, 0, nullptr, 2, initPayload);

	// more of the same repeated commands - 0xC2, 0xEA	
	// can probably get rid of these.
	_cmd_byte = 0xC2;
	NikonLens.sendCommand(_cmd_byte, 4, inputBuffer, 0, nullptr);

	_cmd_byte = 0xEA;
	initPayload[0] = 0x03;
	NikonLens.sendCommand(_cmd_byte, 0, nullptr, 1, initPayload);
}

} /* end namespace lens */
//eof
