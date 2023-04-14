#include <Arduino.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include "application.h"
#include "config.h"
#include "CommandHandler.h"
#include "FilamentController.h"
#include "IdlerController.h"

//Application application;
extern FilamentController filamentController;
extern IdlerController idlerController;

CommandHandler::CommandHandler (){
    // Do Nothing 
}

// Handles any commands input via a USB serial
// NOT the serial to and from the printer
void CommandHandler::keyboardCommands(){
	String inputCommand;

	if (Serial.available()) {
		
		inputCommand = Serial.readString();
		Serial.print(F("Command Input : "));
		Serial.println(inputCommand);

		switch (inputCommand[0])
		{
		case 'C':
			Serial.println(F("Processing 'C' Command"));
			filamentController.filamentLoadWithBondTechGear();
			break;
		case 'H':
			printCommandList();
			break;
		case 'Q':
			Serial.println(F("Processing 'Q' Command : Disable Motors"));
			application.disableAllMotors();
		break;
		case 'T':
			Serial.println(F("Processing 'T' Command : Change Tool"));
			application.toolChange(inputCommand[1]);
			break;
		case 'U':
			Serial.println(F("Processing 'U' Command : Unload Filiment"));
			idlerController.parkIdler(); // reset the idler
			if (idlerController.status == QUICKPARKED) {
				idlerController.quickunParkIdler();             // un-park the idler from a quick park
			}
			if (idlerController.status == INACTIVE) {
				idlerController.unParkIdler();                    // turn on the idler motor
			}
			filamentController.unloadFilamentToFinda();          //unload the filament
			idlerController.parkIdler();   
			application.disableAllMotors();
			break;
		case 'X':
			Serial.println(F("Processing 'X' Command : Reset"));
			//reset Function used for X command 
			break;
		
		default:
			Serial.println(F("!! COMMAND NOT FOUND !!"));
			Serial.println();
			Serial.println();
			printCommandList();
			
			break;
		}
	}
}

// Lists all available keyboard commands 
// TODO :: Keep this updated with new commands 
void CommandHandler::printCommandList(){
	Serial.println(F("C    : "));
	Serial.println(F("H    : Help / CMD menu : Dsplays this command list"));
	Serial.println(F("Q    : Disable Motors  : Disables all Setpper motors"));
	Serial.println(F("T<x> : Change tool     : Changes the colour selector to the given filament and then loads it  into the MMU"));
	Serial.println(F("U    : Unload filament : Unloads the filament that is currently loaded"));
	Serial.println(F("X    : Reset 		     : Resets the MMU unit"));
}