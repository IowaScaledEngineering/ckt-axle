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

#define   GPIO_A     7
#define   GPIO_B     6
#define   GPIO_C     5
#define   GPIO_D     4
#define   GPIO_E     3

// Structure to hold the 5 time variables (in ms)
struct AxleTiming {
	unsigned long wheel;
	unsigned long truck;
	unsigned long bolster;
	unsigned long coupler;
	unsigned long delay;
};

// Lookup table of configurable speeds (in MPH) indexed by 4-bit DIP/GPIO input (0-15)
const uint8_t SPEED_LOOKUP[16] = {
	5,  7, 10, 12, 15, 18, 20, 25,
	30, 33, 37, 42, 55, 75, 90, 120
};

// Global active timing parameters
AxleTiming currentTiming;

uint8_t buttonsPressed = 0;
unsigned long debounceMillis = 0;

// Maximum expected delay in ms for buffer sizing (~10 MPH worst-case scaling)
#define MAX_DELAY_MS 1000
uint8_t shifter[(MAX_DELAY_MS + 4) / 8];

/**
 * Calculates scale timing parameters (HO Scale) given target speed in MPH.
 */
AxleTiming calculateTiming(uint8_t speedMph)
{
	AxleTiming t;
	if (speedMph == 0) speedMph = 1; // Prevent division by zero

	// Scale velocity in inches per millisecond
	// 1 MPH = 17.6 inches per second
	// v_ho = (speedMph * 17.6 in/s) / (87 scale factor * 1000 ms/s)
	float v_ho = (speedMph * 17.6f) / (87.0f * 1000.0f);

	// Physical dimensions (in real-world inches) converted to millisecond intervals
	t.wheel   = (unsigned long)(((22.0f / 87.0f) / v_ho) + 0.5f);        // 22" (0.25" HO) sensor chord block length
	t.truck   = (unsigned long)(((66.0f / 87.0f) / v_ho) + 0.5f);        // 5'6" truck axle spacing
	t.bolster = (unsigned long)(((573.0f / 87.0f) / v_ho) + 0.5f);      // 47'9" bolster distance
	t.coupler = (unsigned long)(((176.0f / 87.0f) / v_ho) + 0.5f);      // 14'8" coupler spacing
	t.delay   = (unsigned long)((1 / v_ho) + 0.5f);       // Sensor physical offset delay

	return t;
}

/**
 * Reads GPIO_A (LSB) to GPIO_D (MSB) to get lookup table index.
 * Inputs are active low due to INPUT_PULLUP.
 */
uint8_t readSpeedIndex()
{
	uint8_t bitA = !digitalRead(GPIO_A);
	uint8_t bitB = !digitalRead(GPIO_B);
	uint8_t bitC = !digitalRead(GPIO_C);
	uint8_t bitD = !digitalRead(GPIO_D);

	return (bitD << 3) | (bitC << 2) | (bitB << 1) | bitA;
}

/**
 * Prints current hardware state, selected speed, and calculated timing values over Serial.
 */
void logDebugInfo(uint8_t speedIdx, uint8_t speedMph, const AxleTiming &t)
{
	Serial.print(F("[DEBUG] DIP Index: "));
	Serial.print(speedIdx);
	Serial.print(F(" | Selected Speed: "));
	Serial.print(speedMph);
	Serial.println(F(" MPH"));

	Serial.print(F("        WHEEL:   "));
	Serial.print(t.wheel);
	Serial.println(F(" ms"));

	Serial.print(F("        TRUCK:   "));
	Serial.print(t.truck);
	Serial.println(F(" ms"));

	Serial.print(F("        BOLSTER: "));
	Serial.print(t.bolster);
	Serial.println(F(" ms"));

	Serial.print(F("        COUPLER: "));
	Serial.print(t.coupler);
	Serial.println(F(" ms"));

	Serial.print(F("        DELAY:   "));
	Serial.print(t.delay);
	Serial.println(F(" ms"));
}

void setup()
{
	Serial.begin(115200);

	digitalWrite(PINA, 0);
	digitalWrite(PINB, 0);
	pinMode(PINA, OUTPUT);
	pinMode(PINB, OUTPUT);
	pinMode(SWITCH, INPUT_PULLUP);

	pinMode(GPIO_A, INPUT_PULLUP);
	pinMode(GPIO_B, INPUT_PULLUP);
	pinMode(GPIO_C, INPUT_PULLUP);
	pinMode(GPIO_D, INPUT_PULLUP);
	pinMode(GPIO_E, INPUT_PULLUP);
	
	delay(100);
	buttonsPressed = !digitalRead(SWITCH);  // pre load

	// Initialize timing parameters from hardware setting
	uint8_t speedIdx = readSpeedIndex();
	uint8_t speedMph = SPEED_LOOKUP[speedIdx];
	currentTiming = calculateTiming(speedMph);

	logDebugInfo(speedIdx, speedMph, currentTiming);
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
unsigned long millisA = 0;
unsigned long millisB = 0;

void setPin(uint8_t pin, bool active)
{
	digitalWrite(pin, active ? 1 : 0);
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
			{
				// Refresh active timing parameters when initiating sequence
				uint8_t speedIdx = readSpeedIndex();
				uint8_t speedMph = SPEED_LOOKUP[speedIdx];
				currentTiming = calculateTiming(speedMph);

				logDebugInfo(speedIdx, speedMph, currentTiming);
				stateA++;
			}
			break;
		case 1:
			setPin(PINA, true);
			millisA = millis();
			stateA++;
			break;
		case 2:
			if( (millis() - millisA) >= currentTiming.wheel )
			{
				setPin(PINA, false);
				millisA = millis();
				stateA++;
			}
			break;
		case 3:
			if( (millis() - millisA) >= (currentTiming.truck - currentTiming.wheel) )
			{
				setPin(PINA, true);
				millisA = millis();
				stateA++;
			}
			break;
		case 4:
			if( (millis() - millisA) >= currentTiming.wheel )
			{
				setPin(PINA, false);
				millisA = millis();
				stateA++;
			}
			break;
		case 5:
			if( (millis() - millisA) >= (currentTiming.bolster - (currentTiming.wheel + currentTiming.truck)) )
			{
				setPin(PINA, true);
				millisA = millis();
				stateA++;
			}
			break;
		case 6:
			if( (millis() - millisA) >= currentTiming.wheel )
			{
				setPin(PINA, false);
				millisA = millis();
				stateA++;
			}
			break;
		case 7:
			if( (millis() - millisA) >= (currentTiming.truck - currentTiming.wheel) )
			{
				setPin(PINA, true);
				millisA = millis();
				stateA++;
			}
			break;
		case 8:
			if( (millis() - millisA) >= currentTiming.wheel )
			{
				setPin(PINA, false);
				millisA = millis();
				stateA++;
			}
			break;
		case 9:
			if( (millis() - millisA) >= (currentTiming.coupler - (currentTiming.wheel + currentTiming.truck)) )
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
		// Every 1ms bit shifter for PINB delay tracking
		uint16_t activeBytes = (currentTiming.delay + 4) / 8;
		if (activeBytes > sizeof(shifter)) activeBytes = sizeof(shifter);

		for(uint8_t i = 0; i < activeBytes; i++)
		{
			if(0 == i)
			{
				// First byte, pull off bit and drive PINB
				if(shifter[i] & 0x01)
					setPin(PINB, true);
				else
					setPin(PINB, false);
			}
			
			shifter[i] = shifter[i] >> 1;

			if(i < (activeBytes - 1))
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
}
