#include <Arduino.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include "application.h"
#include "FilamentController.h"
#include "config.h"
#include "ColorSelectorController.h"
#include "IdlerController.h"

//Application application;
IdlerController idlerController ;
ColorSelectorController colorSelector;

unsigned long timeCStart, timeCEnd;
int f0Min = 1000, f1Min = 1000, f2Min = 1000, f3Min = 1000, f4Min = 1000;
int f0Max, f1Max, f2Max, f3Max, f4Max = 0;
int f0Avg, f1Avg, f2Avg, f3Avg, f4Avg;
long f0Distance, f1Distance, f2Distance, f3Distance, f4Distance = 0;
int f0ToolChange, f1ToolChange, f2ToolChange, f3ToolChange, f4ToolChange = 0;


FilamentController::FilamentController (){
    // Do Nothing 
}

// part of the 'C' command,  does the last little bit to load into the past the extruder gear
void FilamentController::filamentLoadWithBondTechGear() {
	int findaStatus;
	int i;
	int delayFactor;                            // delay factor (in microseconds) for the filament load loop
	int stepCount;
	int tSteps;
	long timeStart, timeEnd, timeUnparking;

	timeCStart = millis();

	//*****************************************************************************************************************
	//*  added this code snippet to not process a 'C' command that is essentially a repeat command


	if (application.repeatTCmdFlag == ACTIVE) {
		Serial.println(F("filamentLoadWithBondTechGear(): filament already loaded and 'C' command already processed"));
		application.repeatTCmdFlag = INACTIVE;
		return;
	}

	findaStatus = digitalRead(findaPin);

	if (findaStatus == 1) {
		Serial.println(F("filamentLoadWithBondTechGear()  Error, filament sensor thinks there is no filament"));
		return;
	}

	if ((application.currentExtruder < '0')  || (application.currentExtruder > '4')) {
		Serial.println(F("filamentLoadWithBondTechGear(): fixing current extruder variable"));
		application.currentExtruder = '0';
	}


	//*************************************************************************************************
	//* change of approach to speed up the IDLER engagement 10.7.18
	//*  WARNING: THIS APPROACH MAY NOT WORK ... NEEDS TO BE DEBUGGED
	//*  C command assumes there is always a T command right before it
	//*  (IF 2 'C' commands are issued by the MK3 in a row the code below might be an issue)
	//*
	//*************************************************************************************************
	timeStart = millis();
	if (idlerController.status == QUICKPARKED) {                        // make sure idler is  in the pending state (set by quickparkidler() routine)
		// Serial.println(F("'C' Command: quickUnParking the Idler"));
		// quickunParkIdler();
#ifdef NOTDEF
		Serial.println(F("filamentLoadWithBondTechGear()  calling specialunparkidler() routine"));
#endif
		idlerController.specialunParkIdler();                                // PLACEHOLDER attempt to speed up the idler engagement a little more 10.13.18
	}
	if (idlerController.status == INACTIVE) {
		idlerController.unParkIdler();
	}

#ifdef NOTDEF
	else {
		Serial.println(F("filamentLoadWithBondTechGear(): looks like I received two 'C' commands in a row"));
		Serial.println(F("                                ignoring the 2nd 'C' command"));
		return;
	}
#endif


	timeEnd = millis();
	timeUnparking = timeEnd - timeStart;
	//*************************************************************************************************
	//* following line of code is currently disabled (in order to test out the code above
	//*  NOTE: I don't understand why the unParkIdler() command is not used instead ???
	//************************************************************************************************
	// idlerSelector(application.currentExtruder);        // move the idler back into position

	stepCount = 0;
	application.time0 = millis();
	//digitalWrite(greenLED, HIGH);                   // turn on the green LED (for debug purposes)

	//*******************************************************************************************
	// feed the filament from the MMU2 into the bondtech gear for 2 seconds at 10 mm/sec
	// STEPPERMM : 144, 1: duration in seconds,  21: feed rate (in mm/sec)
	// delay: 674 (for 10 mm/sec)
	// delay: 350 (for 21 mm/sec)
	// LOAD_DURATION:  1 second (time to spend with the mmu2 extruder active)
	// LOAD_SPEED: 21 mm/sec  (determined by Slic3r settings
	// INSTRUCTION_DELAY:  25 useconds  (time to do the instructions in the loop below, excluding the delayFactor)
	// #define LOAD_DURATION 1000       (load duration in milliseconds, currently set to 1 second)
	// #define LOAD_SPEED 21    // load speed (in mm/sec) during the 'C' command (determined by Slic3r setting)
	// #defefine INSTRUCTION_DELAY 25  // delay (in microseconds) of the loop

	// *******************************************************************************************
	// compute the loop delay factor (eventually this will replace the '350' entry in the loop)
	//       this computed value is in microseconds of time
	//********************************************************************************************
	// delayFactor = ((LOAD_DURATION * 1000.0) / (LOAD_SPEED * STEPSPERMM)) - INSTRUCTION_DELAY;   // compute the delay factor (in microseconds)

	// for (i = 0; i < (STEPSPERMM * 1 * 21); i++) {

	tSteps =   STEPSPERMM * ((float)LOAD_DURATION / 1000.0) * LOAD_SPEED;             // compute the number of steps to take for the given load duration
	delayFactor = (float(LOAD_DURATION * 1000.0) / tSteps) - INSTRUCTION_DELAY;            // 2nd attempt at delayFactor algorithm

#ifdef NOTDEF
	Serial.print(F("Tsteps: "));
	Serial.println(tSteps);
#endif

	for (i = 0; i < tSteps; i++) {
		digitalWrite(extruderStepPin, HIGH);  // step the extruder stepper in the MMU2 unit
		delayMicroseconds(PINHIGH);
		digitalWrite(extruderStepPin, LOW);
		//*****************************************************************************************************
		// replace '350' with delayFactor once testing of variable is complete
		//*****************************************************************************************************
		// after further testing, the '350' can be replaced by delayFactor
		delayMicroseconds(delayFactor);             // this was calculated in order to arrive at a 10mm/sec feed rate
		++stepCount;
	}
	digitalWrite(greenLED, LOW);                      // turn off the green LED (for debug purposes)

	application.time1 = millis();


	//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	// copied from the MM-control-01/blob/master/motion.cpp routine
	// NO LONGER USED (abandoned in place on 10.7.18) ... came up with a better algorithm (see above)
	//
	//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	//***********************************************************************************************************
	//* THIS CODE WORKS BUT IT LEADS TO SOME GRINDING AT THE MMU2 WHILE THE BONDTECH GEAR IS LOADING THE FILAMENT
	//***********************************************************************************************************
#ifdef NOTDEF
	for (i = 0; i <= 320; i++) {
		digitalWrite(extruderStepPin, HIGH);
		delayMicroseconds(PINHIGH);               // delay for 10 useconds
		digitalWrite(extruderStepPin, LOW);
		//delayMicroseconds(2600);             // originally 2600
		delayMicroseconds(800);              // speed up by a factor of 3

	}
	for (i = 0; i <= 450; i++) {
		digitalWrite(extruderStepPin, HIGH);
		delayMicroseconds(PINHIGH);               // delay for 10 useconds
		digitalWrite(extruderStepPin, LOW);
		// delayMicroseconds(2200);            // originally 2200
		delayMicroseconds(800);             // speed up by a factor of 3
	}
#endif

#ifdef DEBUG
	Serial.println(F("C Command: parking the idler"));
#endif
	//***************************************************************************************************************************
	//*  this disengags the idler pulley after the 'C' command has been exectuted
	//***************************************************************************************************************************
	// quickParkIdler();                           // changed to quickparkidler on 10.12.18 (speed things up a bit)

	idlerController.specialParkIdler();                         // PLACEHOLDER (experiment attempted on 10.13.18)

	//parkIdler();                               // turn OFF the idler rollers when filament is loaded

	timeCEnd = millis();
	//*********************************************************************************************
	//* going back to the fundamental approach with the idler
	//*********************************************************************************************
	idlerController.parkIdler();                               // cleanest way to deal with the idler

	printFilamentStats();   // print current Filament Stats

	Serial.print(F("'T' Command processing time (ms): "));
	Serial.println(application.time5 - application.time4);
	Serial.print(F("'C' Command processing time (ms): "));
	Serial.println(timeCEnd - timeCStart);

#ifdef NOTDEF
	Serial.print(F("Time 'T' Command Received: "));
	Serial.println(application.time4);
	Serial.print(F("Time 'T' Command Completed: "));
	Serial.println(application.time5);
#endif

#ifdef NOTDEF
	Serial.print(F("Time 'C' Command Received: "));
	Serial.println(application.time3);
#endif


	Serial.print(F("Time in Critical Load Loop: "));
	Serial.println(application.time1 - application.time0);

#ifdef NOTDEF
	Serial.print(F("Time at Parking the Idler Complete: "));
	Serial.println(application.time2);
	Serial.print(F("Number of commanded steps to the Extruder: "));
	Serial.println(stepCount);
	Serial.print(F("Computed Delay Factor: "));
	Serial.println(delayFactor);
	Serial.print(F("Time Unparking: "));
	Serial.println(timeUnparking);
#endif

#ifdef DEBUG
	Serial.println(F("filamentLoadToMK3(): Loading Filament to Print Head Complete"));
#endif

}


//*********************************************************************************************
// unload Filament using the FINDA sensor
// turns on the extruder motor
//*********************************************************************************************
void FilamentController::unloadFilamentToFinda() {
	int i;
	int findaStatus;
	unsigned int steps;
	unsigned long startTime, currentTime, startTime1;
	int fStatus;

	if (!isFilamentLoaded()) {               // if the filament is already unloaded, do nothing

		Serial.println(F("unloadFilamentToFinda():  filament already unloaded"));
		return;
	}

	digitalWrite(extruderEnablePin, ENABLE);  // turn on the extruder motor
	digitalWrite(extruderDirPin, CW);  // set the direction of the MMU2 extruder motor
	delay(1);

	startTime = millis();
	startTime1 = millis();

loop:

	currentTime = millis();

	fStatus = digitalRead(filamentSwitch);          // read the filament switch (on the top of the mk3 extruder)

	if (fStatus == 0) {                             // filament Switch is still ON, check for timeout condition

		if ((currentTime - startTime1) > 10000) { //2000  // has 2 seconds gone by ?
			("UNLOAD FILAMENT ERROR: filament not unloading properly, stuck in mk3 head");
			startTime1 = millis();
		}
	} else {                                          // check for timeout waiting for FINDA sensor to trigger

		if ((currentTime - startTime) > 10000) {         // 10 seconds worth of trying to unload the filament

			("UNLOAD FILAMENT ERROR: filament is not unloading properly, stuck between mk3 and mmu2");
			startTime = millis();   // reset the start time
		}
	}
	feedFilament(STEPSPERMM);        // 1mm and then check the pinda status

	if (isFilamentLoaded()) {       // keep unloading until we hit the FINDA sensor
		goto loop;
	}


#ifdef NOTDEF
	Serial.println(F("unloadFilamenttoFinda(): Pinda Sensor Triggered during Filament unload"));
#endif
	//
	// for a filament unload ... need to get the filament out of the selector head !!!
	//
	digitalWrite(extruderDirPin, CW);   // back the filament away from the selector
    feedFilament(STEPSPERMM * 23);     // back the filament away from the selector by 23mm

#ifdef NOTDEF
	Serial.println(F("unloadFilamentToFinda(): Unloading Filament Complete ..."));
#endif

	// digitalWrite(ledPin, LOW);     // turn off LED
}

int FilamentController::isFilamentLoaded() {
	int findaStatus;

	// Currently the logic of the finda sensor is inverted 
	// therefore digitalRead will return a 0 when filiment is present 
	// and will return 1 when there is no filiment detected
	if(digitalRead(findaPin) == 1){
		findaStatus = 0;
	}else{
		findaStatus = 1;
	
	}

	return (findaStatus);
}

//
// this routine feeds filament by the amount of steps provided
//  144 steps = 1mm of filament (using the current mk8 gears in the MMU2)
//
void FilamentController::feedFilament(unsigned int steps) {

	int i;

#ifdef NOTDEF
	if (steps > 1) {
		Serial.print(F("Steps: "));
		Serial.println(steps);
	}
#endif

	for (i = 0; i <= steps; i++) {
		digitalWrite(extruderStepPin, HIGH);
		delayMicroseconds(PINHIGH);               // delay for 10 useconds
		digitalWrite(extruderStepPin, LOW);
		delayMicroseconds(PINLOW);               // delay for 10 useconds

		delayMicroseconds(EXTRUDERMOTORDELAY);         // wait for 400 useconds
	}
}

void FilamentController::loadFilament(int direction) {
	int findaStatus;
	unsigned int steps;

	// digitalWrite(ledPin, HIGH);          // turn on LED to indicate extruder motor is running
	digitalWrite(extruderDirPin, direction);  // set the direction of the MMU2 extruder motor


	switch (direction) {
	case CCW:                     // load filament
loop:
		feedFilament(1);        // 1 step and then check the pinda status

		findaStatus = isFilamentLoaded();
		if (findaStatus == 1)              // keep feeding the filament until the pinda sensor triggers
			goto loop;
		Serial.println(F("Pinda Sensor Triggered"));
		// now feed the filament ALL the way to the printer extruder assembly

		steps = 17 * 200 * STEPSIZE;

		Serial.print(F("steps: "));
		Serial.println(steps);
		feedFilament(steps);    // 17 complete revolutions
		Serial.println(F("Loading Filament Complete ..."));
		break;

	case CW:                      // unload filament
loop1:
		feedFilament(STEPSPERMM);            // 1 mm and then check the pinda status
		findaStatus = isFilamentLoaded();
		if (findaStatus == 0)        // wait for the filament to unload past the pinda sensor
			goto loop1;
		Serial.println(F("Pinda Sensor Triggered, unloading filament complete"));

		feedFilament(STEPSPERMM * 23);      // move 23mm so we are out of the way of the selector


		break;
	default:
		Serial.println(F("loadFilament:  I shouldn't be here !!!!"));


	}
}

//***********************************************************************************
//* this routine is executed as part of the 'T' Command (Load Filament)
//***********************************************************************************
void FilamentController::filamentLoadToMK3() {
	int findaStatus;
	int flag;
	int filamentDistance;
	int fStatus;
	int startTime, currentTime;


	if ((application.currentExtruder < '0')  || (application.currentExtruder > '4')) {
		Serial.println(F("filamentLoadToMK3(): fixing current extruder variable"));
		application.currentExtruder = '0';
	}
#ifdef DEBUG
	Serial.println(F("Attempting to move Filament to Print Head Extruder Bondtech Gears"));
	//idlerController.nParkIdler();
	Serial.print(F("filamentLoadToMK3():  application.currentExtruder: "));
	Serial.println(application.currentExtruder);
#endif

	// idlerSelector(application.currentExtruder);        // active the idler before the filament load

	colorSelector.deActivate();

	digitalWrite(extruderEnablePin, ENABLE); // turn on the extruder stepper motor (10.14.18)
	digitalWrite(extruderDirPin, CCW);      // set extruder stepper motor to push filament towards the mk3
	delay(1);                               // wait 1 millisecond

	startTime = millis();

loop:
	// feedFilament(1);        // 1 step and then check the pinda status
	feedFilament(STEPSPERMM);  // feed 1 mm of filament into the bowden tube

	findaStatus = isFilamentLoaded();              // read the FINDA sensor in the MMU2
	currentTime = millis();

	// added this timeout feature on 10.4.18 (2 second timeout)
	if ((currentTime - startTime) > 2000) {
		("FILAMENT LOAD ERROR:  Filament not detected by FINDA sensor, check the selector head in the MMU2");

		startTime = millis();
	}
	if (findaStatus == 1)              // keep feeding the filament until the pinda sensor triggers
		goto loop;
	//***************************************************************************************************
	//* added additional check (10.10.18) - if the filament switch is already set this might mean there is a switch error or a clog
	//*       this error condition can result in 'air printing'
	//***************************************************************************************************************************
loop1:
	fStatus = digitalRead(filamentSwitch);
	if (fStatus == 0) {                    // switch is active (this is not a good condition)
		("FILAMENT LOAD ERROR: Filament Switch in the MK3 is active (see the RED LED), it is either stuck open or there is debris");
		goto loop1;
	}

	//Serial.println(F("filamentLoadToMK3(): Pinda Sensor Triggered during Filament Load"));
	// now loading from the FINDA sensor all the way down to the NEW filament sensor

	feedFilament(STEPSPERMM * 350);       // go 350 mm then look for the 2nd filament sensor
	filamentDistance = 350;

	//delay(15000);                         //wait 15 seconds
	//feedFilament(STEPSPERMM*100);         //go 100 more mm
	//delay(15000);
	//goto skipeverything;

	startTime = millis();
	flag = 0;
	//filamentDistance = 0;

	// wait until the filament sensor on the mk3 extruder head (microswitch) triggers
	while (flag == 0) {

		currentTime = millis();
		if ((currentTime - startTime) > 8000) { // only wait for 8 seconds
			("FILAMENT LOAD ERROR: Filament not detected by the MK3 filament sensor, check the bowden tube for clogging/binding");
			startTime = millis();         // reset the start Time

		}

		feedFilament(STEPSPERMM);        // step forward 1 mm
		filamentDistance++;
		fStatus = digitalRead(filamentSwitch);             // read the filament switch on the mk3 extruder
		if (fStatus == 0) {
			// Serial.println(F("filament switch triggered"));
			flag = 1;

			Serial.print(F("Filament distance traveled (mm): "));
			Serial.println(filamentDistance);

			switch (application.filamentSelection) {
			case 0:
				if (filamentDistance < f0Min) {
					f0Min = filamentDistance;
				}
				if (filamentDistance > f0Max) {
					f0Max = filamentDistance;
				}
				f0Distance += filamentDistance;
				f0ToolChange++;
				f0Avg = f0Distance / f0ToolChange;
				break;
			case 1:
				if (filamentDistance < f1Min) {
					f1Min = filamentDistance;
				}
				if (filamentDistance > f1Max) {
					f1Max = filamentDistance;
				}
				f1Distance += filamentDistance;
				f1ToolChange++;
				f1Avg = f1Distance / f1ToolChange;
				break;

			case 2:
				if (filamentDistance < f2Min) {
					f2Min = filamentDistance;
				}
				if (filamentDistance > f2Max) {
					f2Max = filamentDistance;
				}
				f2Distance += filamentDistance;
				f2ToolChange++;
				f2Avg = f2Distance / f2ToolChange;
				break;
			case 3:
				if (filamentDistance < f3Min) {
					f3Min = filamentDistance;
				}
				if (filamentDistance > f3Max) {
					f3Max = filamentDistance;
				}
				f3Distance += filamentDistance;
				f3ToolChange++;
				f3Avg = f3Distance / f3ToolChange;
				break;
			case 4:
				if (filamentDistance < f4Min) {
					f4Min = filamentDistance;
				}
				if (filamentDistance > f4Max) {
					f4Max = filamentDistance;
				}

				f4Distance += filamentDistance;
				f4ToolChange++;
				f4Avg = f4Distance / f4ToolChange;
				break;
			default:
				Serial.println(F("Error, Invalid Filament Selection"));

			}
			// printFilamentStats();

		}
	}
	// feed filament an additional 32 mm to hit the middle of the bondtech gear
	// go an additional 32mm (increased to 32mm on 10.4.18)

	feedFilament(STEPSPERMM * LENGTHTOMK3GEAR); 
//	feedFilament(13 * 10); 

	//#############################################################################################################################
	//# NEWEXPERIMENT:  removed the parkIdler() command on 10.5.18 to improve timing between 'T' command followng by 'C' command
	//#############################################################################################################################
	// parkIdler();              // park the IDLER (bearing) motor

	//delay(200);             // removed on 10.5.18
	//Serial1.print(F("ok\n"));    // send back acknowledge to the mk3 controller (removed on 10.5.18)

}

// turns on the extruder motor
void FilamentController::loadFilamentToFinda() {
	int i;
	int findaStatus;
	unsigned int steps;
	unsigned long startTime, currentTime;

	digitalWrite(extruderEnablePin, ENABLE);  // added on 10.14.18
	digitalWrite(extruderDirPin, CCW);  // set the direction of the MMU2 extruder motor
	delay(1);

	startTime = millis();

loop:
	currentTime = millis();
	if ((currentTime - startTime) > 10000) { // 10 seconds worth of trying to unload the filament
		("UNLOAD FILAMENT ERROR:   timeout error, filament is not unloading past the FINDA sensor");
		startTime = millis();   // reset the start time clock
	}
	// changed this on 10.12.18 to step 1 mm instead of a single step at a time

	// feedFilament(1);        // 1 step and then check the pinda status
	feedFilament(STEPSPERMM);  // go 144 steps (1 mm) and then check the finda status

	findaStatus = isFilamentLoaded();
	if (findaStatus == 1)              // keep feeding the filament until the pinda sensor triggers
		goto loop;

#ifdef NOTDEF
	Serial.println(F("Pinda Sensor Triggered during Filament Load"));
#endif
	//
	// for a filament load ... need to get the filament out of the selector head !!!
	//
	digitalWrite(extruderDirPin, CW);   // back the filament away from the selector

#ifdef NOTDEF
	steps = 200 * STEPSIZE + 50;
	feedFilament(steps);
#endif

	feedFilament(STEPSPERMM * 23);      // after hitting the FINDA sensor, back away by 23 mm
#ifdef NOTDEF
	Serial.println(F("Loading Filament Complete ..."));
#endif

	// digitalWrite(ledPin, LOW);     // turn off LED
}

void FilamentController::printFilamentStats() {
	Serial.println(F(" "));
	Serial.print(F("F0 Min: "));
	Serial.print(f0Min);
	Serial.print(F("  F0 Max: "));
	Serial.print(f0Max);
	Serial.print(F("  F0 Avg: "));
	Serial.print(f0Avg);
	Serial.print(F("  F0 Length: "));
	Serial.print(f0Distance);
	Serial.print(F("  F0 count: "));
	Serial.println(f0ToolChange);

	Serial.print(F("F1 Min: "));
	Serial.print(f1Min);
	Serial.print(F("  F1 Max: "));
	Serial.print(f1Max);
	Serial.print(F("  F1 Avg: "));
	Serial.print(f1Avg);
	Serial.print(F("  F1 Length: "));
	Serial.print(f1Distance);
	Serial.print(F("  F1 count: "));
	Serial.println(f1ToolChange);

	Serial.print(F("F2 Min: "));
	Serial.print(f2Min);
	Serial.print(F("  F2 Max: "));
	Serial.print(f2Max);
	Serial.print(F("  F2 Avg: "));
	Serial.print(f2Avg);
	Serial.print(F("  F2 Length: "));
	Serial.print(f2Distance);
	Serial.print(F("  F2 count: "));
	Serial.println(f2ToolChange);

	Serial.print(F("F3 Min: "));
	Serial.print(f3Min);
	Serial.print(F("  F3 Max: "));
	Serial.print(f3Max);
	Serial.print(F("  F3 Avg: "));
	Serial.print(f3Avg);
	Serial.print(F("  F3 Length: "));
	Serial.print(f3Distance);
	Serial.print(F("  F3 count: "));
	Serial.println(f3ToolChange);

	Serial.print(F("F4 Min: "));
	Serial.print(f4Min);
	Serial.print(F("  F4 Max: "));
	Serial.print(f4Max);
	Serial.print(F("  F4 Avg: "));
	Serial.print(f4Avg);
	Serial.print(F("  F4 Length: "));
	Serial.print(f4Distance);
	Serial.print(F("  F4 count: "));
	Serial.println(f4ToolChange);

}

