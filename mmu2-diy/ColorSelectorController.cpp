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

ColorSelectorController::ColorSelectorController (): _colorSelectorMotor((uint8_t)colorSelectorEnablePin, (uint8_t)colorSelectorDirPin, (uint8_t)colorSelectorStepPin)
{
        // Do Nothing 
}

void ColorSelectorController::enable() {
	_colorSelectorMotor.enable();
}

void ColorSelectorController::disable() {
	_colorSelectorMotor.disable();
}

void ColorSelectorController::select(char selection) {

	int findaStatus;

	if ((selection < '0') || (selection > '4')) {
		Serial.println(F("select():  Error, invalid filament selection"));
		return;
	}

loop:
	findaStatus = filamentController.isFilamentLoaded();    // check the pinda status ( DO NOT MOVE THE COLOR SELECTOR if filament is present)
	// inverted finda logic : was originaly 0
	if (findaStatus == 1) {
		application.fixTheProblem("select(): Error, filament is present between the MMU2 and the MK3 Extruder:  UNLOAD FILAMENT!!!");
		goto loop;
	}

	// position '0' is always just a move to the left
	if(selection == 0){
		// added the '+10' on 10.5.18 (force selector carriage all the way to the left
		// the '+10' is an attempt to move the selector ALL the way left (puts the selector into known position)
		csTurnAmount(currentPosition + 10, CCW);      
		currentPosition = selectorAbsPos[0];
	}else{
		// here we have to subtract the ASCII id/number from the char and then cast it to an int to get the correct numeric value
		int intSelection = (int) selection - 0x30;

		if (currentPosition <= selectorAbsPos[intSelection]) {
			csTurnAmount((selectorAbsPos[intSelection] - currentPosition), CW);
		} else {
			csTurnAmount((currentPosition - selectorAbsPos[intSelection]), CCW);
		}
		currentPosition = selectorAbsPos[intSelection];
	}	
}  


void ColorSelectorController::csTurnAmount(int steps, int direction) {
	int i;

	_colorSelectorMotor.enable();

	if (direction == CW){
		_colorSelectorMotor.setDirection(LOW);
	} else{
		_colorSelectorMotor.setDirection(HIGH);
	}

	delayMicroseconds(1500);                

	_colorSelectorMotor.step(steps, COLORSELECTORMOTORDELAY);
}

// perform this function only at power up/reset
//
void ColorSelectorController::initColorSelector() {
	
	_colorSelectorMotor.enable();
	csTurnAmount(MAXSELECTOR_STEPS, CW);             // move to the right
	csTurnAmount(MAXSELECTOR_STEPS+20, CCW);        // move all the way to the left
	_colorSelectorMotor.disable();

}

// This function is performed by the 'T' command after so many moves to make sure the colorselector is synchronized
void ColorSelectorController::syncColorSelector() {
	int moveSteps;

	_colorSelectorMotor.enable();

	Serial.print(F("syncColorSelelector()   current Filament selection: "));
	Serial.println(application.filamentSelection);
	moveSteps = MAXSELECTOR_STEPS - selectorAbsPos[application.filamentSelection];

	Serial.print(F("syncColorSelector()   moveSteps: "));
	Serial.println(moveSteps);

	csTurnAmount(moveSteps, CW);                    // move all the way to the right
	csTurnAmount(MAXSELECTOR_STEPS+20, CCW);        // move all the way to the left

}


