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

NikonLens_Class NikonLens;

void NikonLens_Class::begin(
	const u8 handshakePin_In,
	const u8 handshakePin_Out
	)
{
	// m_handshakePin_In  = handshakePin_In;
	// m_handshakePin_Out = handshakePin_Out;
	
	m_handshakePin_In  = 3;
	m_handshakePin_Out = 4;

	pinMode(m_handshakePin_In,  INPUT_PULLUP);
	pinMode(m_handshakePin_Out, OUTPUT);
	
	SPI.setBitOrder(LSBFIRST);
	// SCLK idles high, data is valid on rising edge
	SPI.setDataMode(SPI_MODE3);
	#if F_CPU == 16000000L
		SPI.setClockDivider(SPI_CLOCK_DIV128);
	#else
		#error "FIXME for your F_CPU because I'm lazy"
	#endif
	SPI.begin();
}

void NikonLens_Class::end()
{
	SPI.end();
	//todo: should we do anything with the handshake pins?
}


NikonLens_Class::tResultCode NikonLens_Class::sendCommand(
	const u8  cmd,
	const u8  byteCountFromLens,
	u8* const bytesFromLens,
	const u8  byteCountToLens,
	u8 const* const bytesToLens
	)
{
	assertHandshake(100/*us*/);
	//todo: Wait 1.6ms for lens to ack, should use interrupts
	while(!isHandshakeAsserted());
	
	// the command is ~ - notted - because the MOSI pin is connected to data line of lens
	// through the base -> collector of a transistor 
	SPI.transfer(~cmd);
	
	// TODO: check assertion timeouts 
	while(isHandshakeAsserted());
	
	// Receive data from lens, if any
	for(u8 i = 0; i < byteCountFromLens; i++) {
		//TODO: Wait 5ms for lens to assert H/S

		long int assert_time = micros();

		// TODO: if assert time is greater than 5000 should stop receiving.
		while ((micros() - assert_time < 5000) && (!isHandshakeAsserted()));
		
		
		// while(!isHandshakeAsserted());
		bytesFromLens[i] = SPI.transfer(0x00);
		//todo: Wait 5ms (?) for lens to release H/S
		while (isHandshakeAsserted());

		// assert_time = micros();
		// while ((micros() - assert_time < 5000) && (isHandshakeAsserted()));
		
	}
	
	// Send data to lens, if any
	for(u8 i = 0; i < byteCountToLens; i++) {
		//todo: Wait 5ms for lens to assert H/S
		// while(!isHandshakeAsserted());

		long int assert_time = micros();
		while ((micros() - assert_time < 5000) && (!isHandshakeAsserted()));
		SPI.transfer(~bytesToLens[i]);
		//todo: Wait 5ms (?) for lens to release H/S
		// while(isHandshakeAsserted());
		assert_time = micros();
		// while ((micros() - assert_time < 5000) && (isHandshakeAsserted()));
		while (isHandshakeAsserted());
	}
	
	return Success;
}


void NikonLens_Class::assertHandshake(u16 microseconds)
{
	digitalWrite(m_handshakePin_Out, 1);
	delayMicroseconds(microseconds);
	digitalWrite(m_handshakePin_Out, 0);
}

//eof
