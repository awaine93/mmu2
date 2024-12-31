#include <Arduino.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include "application.h"
#include "config.h"
#include "IdlerController.h"

int oldBearingPosition = 0;      // this tracks the roller bearing position (top motor on the MMU)
int idlerStatus = INACTIVE;

IdlerController::IdlerController(){
    // Do Nothing
}

// Perform this function only at power up/reset
void IdlerController::initIdlerPosition() {
	Serial.println(F("initIdlerPosition(): resetting the Idler Roller Bearing position"));

	digitalWrite(idlerEnablePin, ENABLE);   // turn on the roller bearing motor
	delay(1);
	oldBearingPosition = 125;                // points to position #1
	turnamount(MAXROLLERTRAVEL, CW);
	turnamount(MAXROLLERTRAVEL, CCW);                // move the bearings out of the way
	digitalWrite(idlerEnablePin, DISABLE);   // turn off the idler roller bearing motor

	application.filamentSelection = 0;       // keep track of filament selection (0,1,2,3,4))
	application.currentExtruder = 0;
}

//*********************************************************************************************
//  this routine is called by the 'C' command to re-engage the idler bearing
//*********************************************************************************************
void IdlerController::quickunParkIdler() {
	int rollerSetting;

	//*********************************************************************************************************
	//* don't need to turn on the idler ... it is already on (from the 'T' command)
	//*********************************************************************************************************

	//digitalWrite(idlerEnablePin, ENABLE);   // turn on the roller bearing motor
	//delay(1);                              // wait for 1 millisecond
	//if (status != QUICKPARKED) {
	//    Serial.println(F("quickunParkIdler(): idler already parked"));
	//    return;                              // do nothing since the idler is not 'quick parked'
	//}

#ifdef NOTDEF
	Serial.print(F("quickunparkidler():  application.currentExtruder: "));
	Serial.println(application.currentExtruder);
#endif


	// re-enage the idler bearing that was only moved 1 position (for quicker re-engagement)
	//
#ifdef CRAZYIVAN
	if (application.currentExtruder == 4) {
		rollerSetting = oldBearingPosition + IDLERSTEPSIZE;
		turnamount(IDLERSTEPSIZE, CCW);
	} else {
#endif

		rollerSetting = oldBearingPosition - IDLERSTEPSIZE;   // go back IDLERSTEPSIZE units (hopefully re-enages the bearing
		turnamount(IDLERSTEPSIZE, CW);   // restore old position

#ifdef CRAZYIVAN
	}
#endif

	//Serial.print(F("unParkIdler() Idler Setting: "));
	//Serial.println(rollerSetting);

	//************************************************************************************************
	//* track the absolute position of the idler  (changed on 10.13.18
	//***********************************************************************************************
	oldBearingPosition = rollerSetting - IDLERSTEPSIZE;    // keep track of the idler position

	status = ACTIVE;                   // mark the idler as active

}

// turn on the idler bearing rollers
void IdlerController::unParkIdler() {
	int rollerSetting;

	digitalWrite(idlerEnablePin, ENABLE);  
	digitalWrite(extruderEnablePin, ENABLE);
	delay(1);
	
	rollerSetting = MAXROLLERTRAVEL - bearingAbsPos[application.filamentSelection];
	oldBearingPosition = bearingAbsPos[application.filamentSelection]; // update the idler bearing position

	turnamount(rollerSetting, CW);    // restore the old position
	status = ACTIVE;

}

// move the filament Roller pulleys away from the filament, sets the idler to the starting position
void IdlerController::parkIdler() {
	int newSetting;

	digitalWrite(idlerEnablePin, ENABLE);
	delay(1);

	newSetting = MAXROLLERTRAVEL - oldBearingPosition;

	turnamount(newSetting, CCW);     // move the bearing roller out of the way
	oldBearingPosition = MAXROLLERTRAVEL;   // record the current roller status  (CSK)

	status = INACTIVE;
	digitalWrite(idlerEnablePin, DISABLE);    // turn off the roller bearing stepper motor  (nice to do, cuts down on CURRENT utilization)
	digitalWrite(extruderEnablePin, DISABLE); // turn off the extruder stepper motor as well

}

// attempt to disengage the idler bearing after a 'T' command instead of parking the idler
//  this is trying to save significant time on re-engaging the idler when the 'C' command is activated
void IdlerController::quickParkIdler() {
	int newSetting;

	digitalWrite(idlerEnablePin, ENABLE);
	delay(1);

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

	// DO NOT TURN OFF THE IDLER ... needs to be held in position ---- why ? 
	digitalWrite(idlerEnablePin, DISABLE);    // turn off the roller bearing stepper motor  (nice to do, cuts down on CURRENT utilization)
	digitalWrite(extruderEnablePin, DISABLE); // turn off the extruder stepper motor as well

}

//***************************************************************************************************************
//* called by 'C' command to park the idler
//***************************************************************************************************************
void IdlerController::specialParkIdler() {
	int newSetting, idlerSteps;

	digitalWrite(idlerEnablePin, ENABLE);                          // turn on the idler stepper motor
	delay(1);

	// oldBearingPosition = bearingAbsPos[application.filamentSelection];          // fetch the bearing position based on the filament state

	//*************************************************************************************************
	//*  this is a new approach to moving the idler just a little bit (off the filament)
	//*  in preparation for the 'C' Command

	//*************************************************************************************************
	if (IDLERSTEPSIZE % 2) {
		idlerSteps = IDLERSTEPSIZE / 2 + 1;                         // odd number processing, need to round up

	} else {
		idlerSteps = IDLERSTEPSIZE / 2;

	}

#ifdef NOTDEF
	Serial.print(F("SpecialParkIdler()   idlersteps: "));
	Serial.println(idlerSteps);
#endif

	newSetting = oldBearingPosition + idlerSteps;     // try to move 6 units (just to disengage the roller)
	turnamount(idlerSteps, CCW);

	//************************************************************************************************
	//* record the idler position  (get back to where we were)
	//***********************************************************************************************
	oldBearingPosition = oldBearingPosition + idlerSteps;       // record the current position of the IDLER bearingT

#ifdef DEBUGIDLER
	Serial.print(F("SpecialParkIdler()  oldBearingPosition: "));
	Serial.println(oldBearingPosition);
#endif

	status = QUICKPARKED;                 // use this new state to show the idler is pending the 'C0' command

	//* SPECIAL DEBUG (10.13.18 - evening)
	//* turn off the idler stepper motor
	// digitalWrite(idlerEnablePin, DISABLE);    // turn off the roller bearing stepper motor  (nice to do, cuts down on CURRENT utilization)

#ifdef NOTDEF
	   digitalWrite(extruderEnablePin, DISABLE);
	extruderMotorStatus = INACTIVE;
#endif

}

//*********************************************************************************************
//  this routine is called by the 'C' command to re-engage the idler bearing
//*********************************************************************************************
void IdlerController::specialunParkIdler() {
	int newSetting, idlerSteps;

	// re-enage the idler bearing that was only moved 1 position (for quicker re-engagement)
	//
	if (IDLERSTEPSIZE % 2) {
		idlerSteps = IDLERSTEPSIZE / 2 + 1;                         // odd number processing, need to round up

	} else {
		idlerSteps = IDLERSTEPSIZE / 2;
	}

#ifdef NOTDEF
	Serial.print(F("SpecialunParkIdler()   idlersteps: "));
	Serial.println(idlerSteps);
#endif

#ifdef DEBUGIDLER
	Serial.print(F("SpecialunParkIdler()   oldBearingPosition (beginning of routine): "));
	Serial.println(oldBearingPosition);
#endif

	newSetting = oldBearingPosition - idlerSteps; // go back IDLERSTEPSIZE units (hopefully re-enages the bearing
	turnamount(idlerSteps, CW); // restore old position

	// MIGHT BE A BAD IDEA
	oldBearingPosition = oldBearingPosition - idlerSteps;    // keep track of the idler position

#ifdef DEBUGIDLER
	Serial.print(F("SpecialunParkIdler()  oldBearingPosition: (end of routine):  "));
	Serial.println(oldBearingPosition);
#endif

	status = ACTIVE;                   // mark the idler as active

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
// turn the idler stepper motor
//
void IdlerController::turnamount(int steps, int dir) {
	int i;
	int delayValue;



	Serial.println(F("moving the idler ..."));
	Serial.print(F("steps: "));
	Serial.print(steps);
	Serial.print(F("dir: "));
	Serial.println(dir);

	digitalWrite(idlerEnablePin, ENABLE);   // turn on motor
	digitalWrite(idlerDirPin, dir);
	delay(1);                               // wait for 1 millisecond

	// digitalWrite(ledPin, HIGH);

	//digitalWrite(idlerDirPin, dir);
	//delay(1);                               // wait for 1 millsecond

	// these command actually move the IDLER stepper motor
	//
	for (i = 0; i < steps * STEPSIZE; i++) {
		digitalWrite(idlerStepPin, HIGH);
		delayMicroseconds(PINHIGH);               // delay for 10 useconds
		digitalWrite(idlerStepPin, LOW);
		//delayMicroseconds(PINLOW);               // delay for 10 useconds (removed on 10.7.18

		delayMicroseconds(IDLERMOTORDELAY);

	}
}  // end of turnamount() routine
