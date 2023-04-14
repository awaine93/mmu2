#include <Arduino.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include "application.h"
#include "config.h"
#include "ColorSelectorController.h"
#include "FilamentController.h"

Application application;
FilamentController filamentController;


int csStatus = INACTIVE;
int currentPosition = 0;
int selectorAbsPos[5] = {0, CSSTEPS * 1, CSSTEPS * 2, CSSTEPS * 3, CSSTEPS * 4}; // absolute position of selector stepper motor


ColorSelectorController::ColorSelectorController (){
        // Do Nothing 
}

void ColorSelectorController::activate() {
	digitalWrite(colorSelectorEnablePin, ENABLE);
	delay(1);
	csStatus = ACTIVE;
}

void ColorSelectorController::select(char selection) {

	int findaStatus;

	// this error check was added on 10.4.18

	if ((selection < '0') || (selection > '4')) {
		Serial.println(F("select():  Error, invalid filament selection"));
		return;
	}

	// Serial.println(F("Entering select() routine"));

loop:
	findaStatus = filamentController.isFilamentLoaded();    // check the pinda status ( DO NOT MOVE THE COLOR SELECTOR if filament is present)
	// inverted finda logic : was originaly 0
	if (findaStatus == 1) {
		application.fixTheProblem("select(): Error, filament is present between the MMU2 and the MK3 Extruder:  UNLOAD FILAMENT!!!");
		goto loop;
	}



	switch (selection) {
	case '0':                                       // position '0' is always just a move to the left
		// added the '+10' on 10.5.18 (force selector carriage all the way to the left
		csTurnAmount(currentPosition + 10, CCW);       // the '+10' is an attempt to move the selector ALL the way left (puts the selector into known position)
		currentPosition = selectorAbsPos[0];
		break;
	case '1':
		if (currentPosition <= selectorAbsPos[1]) {
			csTurnAmount((selectorAbsPos[1] - currentPosition), CW);
		} else {
			csTurnAmount((currentPosition - selectorAbsPos[1]), CCW);
		}
		currentPosition = selectorAbsPos[1];
		break;
	case '2':
		if (currentPosition <= selectorAbsPos[2]) {
			csTurnAmount((selectorAbsPos[2] - currentPosition), CW);
		} else {
			csTurnAmount((currentPosition - selectorAbsPos[2]), CCW);

		}
		currentPosition = selectorAbsPos[2];
		break;
	case '3':
		if (currentPosition <= selectorAbsPos[3]) {
			csTurnAmount((selectorAbsPos[3] - currentPosition), CW);
		} else {
			csTurnAmount((currentPosition - selectorAbsPos[3]), CCW);

		}
		currentPosition = selectorAbsPos[3];
		break;
	case '4':
		if (currentPosition <= selectorAbsPos[4]) {
			csTurnAmount((selectorAbsPos[4] - currentPosition), CW);
		} else {
			csTurnAmount((currentPosition - selectorAbsPos[4]), CCW);

		}
		currentPosition = selectorAbsPos[4];
		break;

	}

}  // end of colorSelector routine()


// this is the selector motor with the lead screw (final stage of the MMU2 unit)

void ColorSelectorController::csTurnAmount(int steps, int direction) {
	int i;

	digitalWrite(colorSelectorEnablePin, ENABLE );    // turn on the color selector motor
	// delayMicroseconds(1500);                                       // wait for 1.5 milliseconds          added on 10.4.18

	if (direction == CW)
		digitalWrite(colorSelectorDirPin, LOW);      // set the direction for the Color Extruder Stepper Motor
	else
		digitalWrite(colorSelectorDirPin, HIGH);
	// wait 1 milliseconds
	delayMicroseconds(1500);                      // changed from 500 to 1000 microseconds on 10.6.18, changed to 1500 on 10.7.18)


	for (i = 0; i <= (steps * STEPSIZE); i++) {                      // fixed this to '<=' from '<' on 10.5.18
		digitalWrite(colorSelectorStepPin, HIGH);
		delayMicroseconds(PINHIGH);               // delay for 10 useconds
		digitalWrite(colorSelectorStepPin, LOW);
		delayMicroseconds(PINLOW);               // delay for 10 useconds  (added back in on 10.8.2018)
		delayMicroseconds(COLORSELECTORMOTORDELAY);         // wait for 400 useconds

	}

#ifdef TURNOFFSELECTORMOTOR                         // added on 10.14.18
	digitalWrite(colorSelectorEnablePin, DISABLE);    // turn off the color selector motor
#endif

}

// perform this function only at power up/reset
//
void ColorSelectorController::initColorSelector() {
	digitalWrite(colorSelectorEnablePin, ENABLE);   // turn on the stepper motor
	delay(1);                                       // wait for 1 millisecond

	csTurnAmount(MAXSELECTOR_STEPS, CW);             // move to the right
	csTurnAmount(MAXSELECTOR_STEPS+20, CCW);        // move all the way to the left

	digitalWrite(colorSelectorEnablePin, DISABLE);   // turn off the stepper motor

}

void ColorSelectorController::deActivate() {

#ifdef TURNOFFSELECTORMOTOR
	digitalWrite(colorSelectorEnablePin, DISABLE);    // turn off the color selector stepper motor  (nice to do, cuts down on CURRENT utilization)
	delay(1);
	csStatus = INACTIVE;
#endif

}

// this function is performed by the 'T' command after so many moves to make sure the colorselector is synchronized
//
void ColorSelectorController::syncColorSelector() {
	int moveSteps;

	digitalWrite(colorSelectorEnablePin, ENABLE);   // turn on the selector stepper motor
	delay(1);                                       // wait for 1 millecond

	Serial.print(F("syncColorSelelector()   current Filament selection: "));
	Serial.println(application.filamentSelection);
	moveSteps = MAXSELECTOR_STEPS - selectorAbsPos[application.filamentSelection];

	Serial.print(F("syncColorSelector()   moveSteps: "));
	Serial.println(moveSteps);

	csTurnAmount(moveSteps, CW);                    // move all the way to the right
	csTurnAmount(MAXSELECTOR_STEPS+20, CCW);        // move all the way to the left

#ifdef TURNOFFSELECTORMOTOR                        // added on 10.14.18
	digitalWrite(colorSelectorEnablePin, DISABLE);   // turn off the selector stepper motor
#endif
}

