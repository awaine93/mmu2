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
	application.currentExtruder = 0;
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

// this routine drives the 5 position bearings (aka idler) on the top of the MMU2 carriage
void IdlerController::select(int filament) {
	int steps;
	int newBearingPosition;
	int newSetting;

	if ((filament < 0) || (filament > 4)) {
		Serial.println(F("idlerController.select() ERROR, invalid filament selection"));
		Serial.print(F("idlerController.select() filament: "));
		Serial.println(filament);
		return;
	}
	// move the selector back to it's origin state

	newBearingPosition = bearingAbsPos[filament];                         // idler set to 1st position
	application.filamentSelection = filament;
	application.currentExtruder = filament;
	
	newSetting = newBearingPosition - oldBearingPosition;

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
	int i;
	int delayValue;

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