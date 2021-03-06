
//  Author: avishorp@gmail.com
//
//  This library is free software; you can redistribute it and/or
//  modify it under the terms of the GNU Lesser General Public
//  License as published by the Free Software Foundation; either
//  version 2.1 of the License, or (at your option) any later version.
//
//  This library is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//  Lesser General Public License for more details.
//
//  You should have received a copy of the GNU Lesser General Public
//  License along with this library; if not, write to the Free Software
//  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

extern "C" {
  #include <stdlib.h>
  #include <string.h>
  #include <inttypes.h>
}

#include <TM1637.h>
#include <Arduino.h>

#define TM1637_I2C_COMM1    0x40
#define TM1637_I2C_COMM2    0xC0
#define TM1637_I2C_COMM3    0x80

//
//      A
//     ---
//  F |   | B
//     -G-
//  E |   | C
//     ---
//      D
const uint8_t digitToSegment[] = {
 // XGFEDCBA
  0b00111111,    // 0
  0b00000110,    // 1
  0b01011011,    // 2
  0b01001111,    // 3
  0b01100110,    // 4
  0b01101101,    // 5
  0b01111101,    // 6
  0b00000111,    // 7
  0b01111111,    // 8
  0b01101111,    // 9
  0b01110111,    // A
  0b01111100,    // b
  0b00111001,    // C
  0b01011110,    // d
  0b01111001,    // E
  0b01110001     // F
  };


TM1637::TM1637(uint8_t pinClk, uint8_t pinDIO)
{
	// Copy the pin numbers
	m_pinClk = pinClk;
	m_pinDIO = pinDIO;
	
	// Set the pin direction and default value.
	// Both pins are set as inputs, allowing the pull-up resistors to pull them up
    pinMode(m_pinClk, INPUT);
    pinMode(m_pinDIO,INPUT);
	digitalWrite(m_pinClk, LOW);
	digitalWrite(m_pinDIO, LOW);
	BL(3);//如果不设置默认中间亮度，解决不先设置亮度无法显示
}

void TM1637::BL(uint8_t brightness)//0-7级亮度
{
	m_brightness = brightness+8;
	start();
	writeByte(TM1637_I2C_COMM3 + (m_brightness & 0x0f));
	stop();
}


void TM1637::setSegments(const uint8_t segments[], uint8_t length, uint8_t pos)
{
    // Write COMM1
	start();
	writeByte(TM1637_I2C_COMM1);
	stop();
	
	// Write COMM2 + first digit address
	start();
	writeByte(TM1637_I2C_COMM2 + (pos & 0x03));
	
	// Write the data bytes
	for (uint8_t k=0; k < length; k++) 
	  writeByte(segments[k]);
	  
	stop();

}


void TM1637::Num(int num, bool leading_zero, uint8_t length, uint8_t pos)
{
	uint8_t digits[4];
	const static int divisors[] = { 1, 10, 100, 1000 };
	bool leading = true;
	
	for(int8_t k = 0; k < 4; k++) {
	    int divisor = divisors[4 - 1 - k];
		int d = num / divisor;
		
		if (d == 0) {
		  if (leading_zero || !leading || (k == 3))
		    digits[k] = encodeDigit(d);
	      else
		    digits[k] = 0;
		}
		else {
			digits[k] = encodeDigit(d);
			num -= d * divisor;
			leading = false;
		}
	}
	
	setSegments(digits + (4 - length), length, pos);
}

void TM1637::bitDelay()
{
	delayMicroseconds(50);
}
   
void TM1637::start()
{
  pinMode(m_pinDIO, OUTPUT);
  bitDelay();
}
   
void TM1637::stop()
{
	pinMode(m_pinDIO, OUTPUT);
	bitDelay();
	pinMode(m_pinClk, INPUT);
	bitDelay();
	pinMode(m_pinDIO, INPUT);
	bitDelay();
}
  
bool TM1637::writeByte(uint8_t b)
{
  uint8_t data = b;

  // 8 Data Bits
  for(uint8_t i = 0; i < 8; i++) {
    // CLK low
    pinMode(m_pinClk, OUTPUT);
    bitDelay();
    
	// Set data bit
    if (data & 0x01)
      pinMode(m_pinDIO, INPUT);
    else
      pinMode(m_pinDIO, OUTPUT);
    
    bitDelay();
	
	// CLK high
    pinMode(m_pinClk, INPUT);
    bitDelay();
    data = data >> 1;
  }
  
  // Wait for acknowledge
  // CLK to zero
  pinMode(m_pinClk, OUTPUT);
  pinMode(m_pinDIO, INPUT);
  bitDelay();
  
  // CLK to high
  pinMode(m_pinClk, INPUT);
  bitDelay();
  uint8_t ack = digitalRead(m_pinDIO);
  if (ack == 0)
    pinMode(m_pinDIO, OUTPUT);
	
	
  bitDelay();
  pinMode(m_pinClk, OUTPUT);
  bitDelay();
  
  return ack;
}

uint8_t TM1637::encodeDigit(uint8_t digit)
{
	return digitToSegment[digit & 0x0f];
}

   
 void TM1637::DNum(uint8_t num1,uint8_t num2,bool piont)
{
	uint8_t digits[4];
	num1=num1%100;
	num2=num2%100;
	digits[0]=num1/10;
	digits[1]=num1%10;	
	digits[2]=num2/10;
	digits[3]=num2%10;	
	for(int i=0;i<4;i++)
	{
		digits[i]=encodeDigit(digits[i]);		
	}
	if(piont) digits[1]=digits[1]+0x80;//在第二位加点
	setSegments(digits);
}

void TM1637::show(bool BS)
{
	uint8_t BLNUM;	
	 if(BS) 
	 {
		 BLNUM=m_brightness;
	 } 
	else 
	{
		BLNUM=0 ;
	}
	start();
	writeByte(TM1637_I2C_COMM3 + (BLNUM & 0x0f));
	stop();
}
void TM1637::cls()
{
	uint8_t digits[4]={0,0,0,0};
	setSegments(digits);
}