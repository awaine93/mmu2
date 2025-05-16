#include <Arduino.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include "application.h"
#include "config.h"
#include "CommandHandler.h"
#include "FilamentController.h"
#include "IdlerController.h"
#include "ColorSelectorController.h"

//Application application;
extern FilamentController filamentController;
extern IdlerController idlerController;
extern ColorSelectorController colorSelector;

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
			Serial.println(F("Processing 'H' Command : print commands list"));
			printKeyboardCommandList();
			printPrinterCommandList();
			break;
		case 'P':
			Serial.println(F("Processing 'P' Command : Filament Status"));
			Serial.println(filamentController.isFilamentLoaded());
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
			filamentController.disable(); // turn off the extruder stepper motor as well
			if (idlerController.status == QUICKPARKED) {
				idlerController.quickunParkIdler();             // un-park the idler from a quick park
			}
			if (idlerController.status == INACTIVE) {
				filamentController.enable();
				delay(1);
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
			printKeyboardCommandList();
			
			break;
		}
	}
}

void(* resetFunc) (void) = 0;

int CommandHandler::handlePrinterCommand(String inputLine, int index){
		unsigned char c1, c2, c3;

		c1 = inputLine[index++];  
		c2 = inputLine[index++]; 
		c3 = inputLine[index++];
		
		int c2Int = static_cast<int>(c2) - '0';

		// process commands coming from the mk3 controller : Extract this to a method so that 
		// both USB and Serial printer can run the commands
		//***********************************************************************************
		// Commands still to be implemented: F0 (Filament type select),
		// E0->E4 (Eject Filament), R0 (recover from eject)
		//***********************************************************************************
		switch (c1) {
		case 'X':
			// RESET COMMAND
			Serial.println("X: Resetting MMU");
			resetFunc();
			break;
		case 'T':
			Serial.println("T: Tool change");
		
			application.time4 = millis();  // grab the current time
			
			// Request for idler and selector based on filament number
			Serial.print(F("T: filamentSelection = "));
			Serial.println(c2Int);

			if (c2Int >= 0 && c2Int <= 4) {
				application.toolChange(c2Int);
			} else {
				Serial.println(F("T: Invalid filament Selection"));
			}

			Serial1.print(F("ok\n"));              // send command acknowledge back to mk3 controller
			application.time5 = millis();          // grab the current time
			printMotorStatus();
			break;
		case 'C':
			// This command expects the T command to be run before it
			Serial.println(F("C: Moving filament to extruder"));
			// move filament from selector ALL the way to print head
			
			//filamentController.filamentLoadToMK3();
			filamentController.filamentLoadWithBondTechGear();
			// delay(200);
			Serial1.print(F("ok\n"));
			printMotorStatus();
			break;
		case 'U':
			// UNLOAD FILIMENT COMMAND
			// request for filament unload

			Serial.println(F("U: Filament Unload Selected"));
			//*******************************************************************************************************
			//*  FIX:  don't go all the way to the end ... be smarter
			//******************************************************************************************************
			//* unparking is more elegant 10.12.1
			Serial.print(F("idlerController.status"));
			Serial.println(idlerController.status);


			if (idlerController.status == QUICKPARKED) {
				idlerController.quickunParkIdler();             // un-park the idler from a quick park
			}
			if (idlerController.status == INACTIVE) {
				filamentController.enable();
				delay(1);
				idlerController.unParkIdler();                    // turn on the idler motor
			}

			if ((c2Int >= 0) && (c2Int <= 4)) {

				filamentController.unloadFilamentToFinda();
				idlerController.parkIdler();
				filamentController.disable(); // turn off the extruder stepper motor as well
				Serial.println(F("U: Sending Filament Unload Acknowledge to MK3"));
				delay(200);
				Serial1.print(F("ok\n"));

			} else {
				Serial.println(F("U: Invalid filament Unload Requested"));
				delay(200);
				Serial1.print(F("ok\n"));
			}
			filamentController.disable();
			printMotorStatus();
			break;
		case 'L':
			Serial.println(F("L: Filament Load Selected"));

			if (idlerController.status == QUICKPARKED) {
				idlerController.quickunParkIdler();
			}
			
			if (idlerController.status == INACTIVE) {
				filamentController.enable();
				delay(1);
				idlerController.unParkIdler();                   
			}
		
			if (colorSelector._colorSelectorMotor.enabled == 0){
				colorSelector.enable();         
			}

			if ((c2Int >= 0) && (c2Int <= 4)) {
				idlerController.select(c2Int);   
				colorSelector.select(c2Int); 

				filamentController.loadFilamentToFinda();
				
				idlerController.parkIdler();             
				filamentController.disable();
				colorSelector.disable();

				Serial.println(F("L: Sending Filament Load Acknowledge to MK3"));

				delay(200);

				Serial1.print(F("ok\n"));

			} else {
				Serial.println(F("Error: Invalid Filament Number Selected"));
			}
			printMotorStatus();
			break;

		case 'S':
			// request for firmware version
			switch (c2) {
			case '0':
				Serial.println(F("S: Sending back OK to MK3"));
				Serial1.print(F("ok\n"));
				break;
			case '1':
				Serial.println(F("S: FW Version Request"));
				Serial1.print(FW_VERSION);
				Serial1.print(F("ok\n"));
				break;
			case '2':
				Serial.println(F("S: Build Number Request"));
				Serial.println(F("Initial Communication with MK3 Controller: Successful"));
				Serial1.print(FW_BUILDNR);
				Serial1.print(F("ok\n"));
				break;
			default:
				Serial.println(F("S: Unable to process S Command"));
				break;
			}
			printMotorStatus();
			break;
		case 'P':
			// check FINDA status
			Serial.println(F("Check FINDA Status Request"));
			if (filamentController.isFilamentLoaded() == 1) {
				Serial1.println(F("1"));
			}
			else {
				Serial1.println(F("0"));
			}
			Serial1.print(F("ok\n"));
			printMotorStatus();
			break;
		case 'Q':
			Serial.println(F("Processing 'Q' Command : Disable Motors"));
			application.disableAllMotors();
			printMotorStatus();
			break;
		case 'F':  // 'F' command is acknowledged but no processing goes on at the moment
			// will be useful for flexible material down the road
			Serial.println(F("Filament Type Selected: "));
			Serial.println(c2);
			Serial1.print(F("ok\n"));                        // send back OK to the mk3
			printMotorStatus();
			break;
		case 'M': 
			Serial.println(F("Motor Status Request"));
			printMotorStatus();
			break;
		default:
			Serial.print(F("ERROR: unrecognized command from the MK3 controller"));
			Serial1.print(F("ok\n"));
		}  // end of switch statement
	return index;
}


void CommandHandler::printMotorStatus(){

	Serial.println(F("Motor Status:"));
	Serial.print(F("Extruder: "));
	Serial.println(filamentController._filamentMotor.enabled);
	Serial.print(F("Idler: "));
	Serial.println(idlerController._idlerMotor.enabled);
	Serial.print(F("Color Selector: "));
	Serial.println(colorSelector._colorSelectorMotor.enabled);

	Serial.print(F("Idler Park Status:"));
	Serial.println(idlerController.status);
}


// Lists all available keyboard commands 
// TODO :: Keep this updated with new commands 
void CommandHandler::printKeyboardCommandList(){
	Serial.println(F("Serial Commands:"));
	Serial.println(F("C    : "));
	Serial.println(F("H    : Help / CMD menu : Dsplays this command list"));
	Serial.println(F("P    : Filament Status : Returns 1 if filament is loaded and 0 if not loaded"));
	Serial.println(F("Q    : Disable Motors  : Disables all Setpper motors"));
	Serial.println(F("T<x> : Change tool     : Changes the colour selector to the given filament and then loads it into the MMU"));
	Serial.println(F("U    : Unload filament : Unloads the filament that is currently loaded"));
	Serial.println(F("X    : Reset 		     : Resets the MMU unit"));
	Serial.println();
	Serial.println();
}

void CommandHandler::printPrinterCommandList(){
	Serial.println(F("Printer Commands:"));
  	Serial.println(F("X    : Reset           : Resets the MMU"));
  	Serial.println(F("T<x> : Tool Change     : Changes the tool to the specified filament number (0-4)"));
  	Serial.println(F("C    : Load Filament   : Loads filament to the Mk3 extruder"));
  	Serial.println(F("U<x> : Unload Filament : Unloads the specified filament number (0-4) to the FINDA sensor"));
  	Serial.println(F("L<x> : Load Filament   : Loads the specified filament number (0-4) into the MMU"));
  	Serial.println(F("S0   : Acknowledge     : Sends an OK response to the MK3 controller"));
  	Serial.println(F("S1   : Firmware Info   : Requests firmware version"));
  	Serial.println(F("S2   : Build Number    : Requests build number and initial communication status"));
  	Serial.println(F("P    : FINDA Status    : Checks if filament is loaded (returns 1 for loaded, 0 for not loaded)"));
  	Serial.println(F("Q    : Disable Motors  : Disables all Setpper motors"));
  	Serial.println(F("F<x> : Filament Type   : Acknowledges selected filament type (future use)"));
	Serial.println();
	Serial.println();
}