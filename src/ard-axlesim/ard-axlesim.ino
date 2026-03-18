/*************************************************************************
Title:    Axle Counter Simulator
Authors:  Michael Petersen <railfan@drgw.net>
File:     ard-axlesim.ino
License:  GNU General Public License v3

LICENSE:
    Copyright (C) 2026 Michael Petersen & Nathan Holmes

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

*************************************************************************/

#define   PINA     12
#define   PINB     11
#define   SWITCH   A5

//  Timing in ms (25 MPH HO)
#define   WHEEL       49
#define   TRUCK      148
#define   BOLSTER   1285
#define   COUPLER    396
#define   DELAY      198

/*
//  Timing in ms (120 MPH HO)
#define   WHEEL       10
#define   TRUCK       31
#define   BOLSTER    268
#define   COUPLER     82
#define   DELAY       41
*/

uint8_t buttonsPressed = 0;

unsigned long debounceMillis = 0;

uint8_t shifter[(DELAY+4) / 8];

void setup()
{
	digitalWrite(PINA, 0);
	digitalWrite(PINB, 0);
	pinMode(PINA, INPUT);
	pinMode(PINB, INPUT);
	pinMode(SWITCH, INPUT_PULLUP);
	
	delay(100);
	buttonsPressed = !digitalRead(SWITCH);  // pre load
}

uint8_t debounce(uint8_t debouncedState, uint8_t newInputs)
{
	static uint8_t clock_A = 0, clock_B = 0;
	uint8_t delta = newInputs ^ debouncedState; // Find all of the changes
	uint8_t changes;

	clock_A ^= clock_B; //Increment the counters
	clock_B  = ~clock_B;

	clock_A &= delta; //Reset the counters if no changes
	clock_B &= delta; //were detected.

	changes = ~((~delta) | clock_A | clock_B);
	debouncedState ^= changes;
	return(debouncedState);
}

uint8_t stateA = 0;
uint8_t stateB = 0;

unsigned long millisA = 0;
unsigned long millisB = 0;

void setPin(uint8_t pin, bool active)
{
	if(active)
	{
		pinMode(pin, OUTPUT);
	}
	else
	{
		pinMode(pin, INPUT);
	}
}

void loop()
{
	uint8_t inputStatus;
	
	if( (millis() - debounceMillis) >= 10 )
	{
		// Every 10ms
		inputStatus = !digitalRead(SWITCH);
		buttonsPressed = debounce(buttonsPressed, inputStatus);
		debounceMillis += 10;
	}

	switch(stateA)
	{
		case 0:
			if(buttonsPressed)
				stateA++;
			break;
		case 1:
			setPin(PINA, true);
			millisA = millis();
			stateA++;
			break;
		case 2:
			if( (millis() - millisA) >= (WHEEL) )
			{
				setPin(PINA, false);
				millisA = millis();
				stateA++;
			}
			break;
		case 3:
			if( (millis() - millisA) >= (TRUCK-WHEEL) )
			{
				setPin(PINA, true);
				millisA = millis();
				stateA++;
			}
			break;
		case 4:
			if( (millis() - millisA) >= (WHEEL) )
			{
				setPin(PINA, false);
				millisA = millis();
				stateA++;
			}
			break;
		case 5:
			if( (millis() - millisA) >= (BOLSTER - (WHEEL + TRUCK)) )
			{
				setPin(PINA, true);
				millisA = millis();
				stateA++;
			}
			break;
		case 6:
			if( (millis() - millisA) >= (WHEEL) )
			{
				setPin(PINA, false);
				millisA = millis();
				stateA++;
			}
			break;
		case 7:
			if( (millis() - millisA) >= (TRUCK-WHEEL) )
			{
				setPin(PINA, true);
				millisA = millis();
				stateA++;
			}
			break;
		case 8:
			if( (millis() - millisA) >= (WHEEL) )
			{
				setPin(PINA, false);
				millisA = millis();
				stateA++;
			}
			break;
		case 9:
			if( (millis() - millisA) >= (COUPLER - (WHEEL + TRUCK)) )
			{
				stateA++;
			}
			break;

		default:
			stateA = 0;
			break;
	}

	if( (millis() - millisB) >= 1 )
	{
		// Every 1ms

		for(uint8_t i = 0; i<sizeof(shifter); i++)
		{
			if(0 == i)
			{
				// First byte, pull off bit and drive PINB
				if(shifter[i] & 0x01)
					setPin(PINB, false);
				else
					setPin(PINB, true);
			}
			
			shifter[i] = shifter[i] >> 1;

			if(i < (sizeof(shifter) - 1))
			{
				// Not the last byte
				if(shifter[i+1] & 0x01)
					shifter[i] |= 0x80;
			}
			else
			{
				// Last byte, add PINA current state
				if(digitalRead(PINA))
					shifter[i] |= 0x80;
			}
		}

		millisB += 1;
	}




/*
		digitalWrite(PINA, 1);
		delay(WHEEL);
		digitalWrite(PINA, 0);
		delay(TRUCK-WHEEL);
		digitalWrite(PINA, 1);
		delay(WHEEL);
		digitalWrite(PINA, 0);

		delay(BOLSTER - (WHEEL + TRUCK));

		digitalWrite(PINA, 1);
		delay(WHEEL);
		digitalWrite(PINA, 0);
		delay(TRUCK-WHEEL);
		digitalWrite(PINA, 1);
		delay(WHEEL);
		digitalWrite(PINA, 0);

		delay(COUPLER - (WHEEL + TRUCK));
*/

}
