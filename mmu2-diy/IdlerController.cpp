#include <Arduino.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include "application.h"
#include "config.h"
#include "IdlerController.h"
#include "FilamentController.h"

int oldBearingPosition = 0;      // this tracks the roller bearing position (top motor on the MMU)
int idlerStatus = INACTIVE;

float bearingAbsPos[5] = {0, IDLERSTEPSIZE, IDLERSTEPSIZE * 2, IDLERSTEPSIZE * 3, IDLERSTEPSIZE * 4};

IdlerController::IdlerController() :  _idlerMotor((uint8_t)idlerEnablePin, (uint8_t)idlerDirPin, (uint8_t)idlerStepPin)
{
    // Do Nothing
}

void IdlerController::enable(){
	_idlerMotor.enable();
}

void IdlerController::disable(){
	_idlerMotor.disable();
}

// Perform this function only at power up/reset
void IdlerController::initIdlerPosition() {
	Serial.println(F("initIdlerPosition(): resetting the Idler Roller Bearing position"));	 
	delay(1);
	oldBearingPosition = 125;                // points to position #1
	turnamount(MAXROLLERTRAVEL, CW);
	turnamount(MAXROLLERTRAVEL, CCW);                // move the bearings out of the way
	_idlerMotor.disable();

	application.filamentSelection = 0;       // keep track of filament selection (0,1,2,3,4))
	application.currentExtruder = '0';
}

//*********************************************************************************************
//  this routine is called by the 'C' command to re-engage the idler bearing
//*********************************************************************************************
void IdlerController::quickunParkIdler() {
	int rollerSetting;

	_idlerMotor.enable();  // turn on the roller bearing motor
	delay(1);                              // wait for 1 millisecond
	
	if (status != QUICKPARKED) {
		_idlerMotor.disable();
	    return;                              // do nothing since the idler is not 'quick parked'
	}

	// re-enage the idler bearing that was only moved 1 position (for quicker re-engagement)

	rollerSetting = oldBearingPosition - IDLERSTEPSIZE;   // go back IDLERSTEPSIZE units (hopefully re-enages the bearing
	turnamount(IDLERSTEPSIZE, CW);   // restore old position

	oldBearingPosition = rollerSetting - IDLERSTEPSIZE;    // keep track of the idler position

	status = ACTIVE;                   // mark the idler as active
}

// Turns on the idler bearing rollers
void IdlerController::unParkIdler() {
	int rollerSetting;
	
	rollerSetting = MAXROLLERTRAVEL - bearingAbsPos[application.filamentSelection];
	oldBearingPosition = bearingAbsPos[application.filamentSelection]; // update the idler bearing position

	turnamount(rollerSetting, CW);    // restore the old position
	status = ACTIVE;

}

// move the filament Roller pulleys away from the filament, sets the idler to the starting position
void IdlerController::parkIdler() {
	int newSetting;

	newSetting = MAXROLLERTRAVEL - oldBearingPosition;

	turnamount(newSetting, CCW);     // move the bearing roller out of the way
	oldBearingPosition = MAXROLLERTRAVEL;   // record the current roller status  (CSK)

	status = INACTIVE;
	_idlerMotor.disable();   // turn off the roller bearing stepper motor  (nice to do, cuts down on CURRENT utilization)
}

// attempt to disengage the idler bearing after a 'T' command instead of parking the idler
//  this is trying to save significant time on re-engaging the idler when the 'C' command is activated
void IdlerController::quickParkIdler() {
	int newSetting;

	//**************************************************************************************************
	//*  this is flawed logic, if I have done a special park idler the oldBearingPosition doesn't map exactly to the application.filamentSelection
	//*   discovered on 10.13.18
	//*  In fact,  I don't need to update the 'oldBearingPosition' value, it is already VALID
	//********************************************************************************************************************************
	// oldBearingPosition = bearingAbsPos[application.filamentSelection];          // fetch the bearing position based on the filament state


	//newSetting = MAXROLLERTRAVEL - oldBearingPosition;
	//*************************************************************************************************
	//*  this is a new approach to moving the idler just a little bit (off the filament)
	//*  in preparation for the 'C' Command

	//*************************************************************************************************

	newSetting = oldBearingPosition + IDLERSTEPSIZE;       // try to move 12 units (just to disengage the roller)
	turnamount(IDLERSTEPSIZE, CCW);
	oldBearingPosition = oldBearingPosition + IDLERSTEPSIZE; // record the current position of the IDLER bearing

	status = QUICKPARKED; // use this new state to show the idler is pending the 'C0' command

	_idlerMotor.disable();   // turn off the roller bearing stepper motor  (nice to do, cuts down on CURRENT utilization)
}


void IdlerController::select(char filament) {
	int steps;
	int newBearingPosition;
	int newSetting; 


	Serial.print(F("idlerSelector(): Filament Selected: "));
	Serial.println(filament);

	//* added on 10.14.18  (need to turn the extruder stepper motor back on since it is turned off by parkidler()
	digitalWrite(extruderEnablePin, ENABLE);


	if ((filament < '0') || (filament > '4')) {
		Serial.println(F("idlerSelector() ERROR, invalid filament selection"));
		Serial.print(F("idlerSelector() filament: "));
		Serial.println(filament);
		return;
	}
	// move the selector back to it's origin state

	Serial.print(F("Old Idler Roller Bearing Position:"));
	Serial.println(oldBearingPosition);
	Serial.println(F("Moving filament selector"));

	switch (filament) {
	case '0':
		newBearingPosition = bearingAbsPos[0];                         // idler set to 1st position
		application.filamentSelection = 0;
		application.currentExtruder = '0';
		break;
	case '1':
		newBearingPosition = bearingAbsPos[1];
		application.filamentSelection = 1;
		application.currentExtruder = '1';
		break;
	case '2':
		newBearingPosition = bearingAbsPos[2];
		application.filamentSelection = 2;
		application.currentExtruder = '2';
		break;
	case '3':
		newBearingPosition = bearingAbsPos[3];
		application.filamentSelection = 3;
		application.currentExtruder = '3';
		break;
	case '4':
		newBearingPosition = bearingAbsPos[4];
		application.filamentSelection = 4;
		application.currentExtruder = '4';
		break;
	default:
		Serial.println(F("idlerSelector(): ERROR, Invalid Idler Bearing Position"));
		break;
	}

	newSetting = newBearingPosition - oldBearingPosition;

	Serial.print(F("Old Bearing Position: "));
	Serial.println(oldBearingPosition);
	Serial.print(F("New Bearing Position: "));
	Serial.println(newBearingPosition);

	Serial.print(F("New Setting: "));
	Serial.println(newSetting);

	if (newSetting < 0) {
		turnamount(-newSetting, CW);                     // turn idler to appropriate position
	} else {
		turnamount(newSetting, CCW);                     // turn idler to appropriate position
	}

	oldBearingPosition = newBearingPosition;
}



//
// NOTE - THIS MEHTOD WILL LEAVE THE IDLER MOTOR ENABLED AND WILL NEED TO BE HANDLED AFTER THIS METHOD IS CALLED
// turn the idler stepper motor
//
void IdlerController::turnamount(int steps, int dir) {

	Serial.println(F("moving the idler ..."));
	Serial.print(F("steps: "));
	Serial.println(steps);
	Serial.print(F("dir: "));
	Serial.println(dir);

	_idlerMotor.enable();
	_idlerMotor.setDirection(dir);
	delay(1);

	_idlerMotor.step(steps, IDLERMOTORDELAY);

}