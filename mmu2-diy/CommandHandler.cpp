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
			printCommandList();
			break;
		case 'I':

			if (idlerController.status == QUICKPARKED) {
				idlerController.quickunParkIdler();             // un-park the idler from a quick park
			}
			if (idlerController.status == INACTIVE) {
				idlerController.unParkIdler();                    // turn on the idler motor
			}

			idlerController.select(inputCommand[1]);
			break;
		case 'P':
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

int CommandHandler::handlePrinterCommand(String inputLine, int index){
		unsigned char c1, c2, c3;

		c1 = inputLine[index++];  
		c2 = inputLine[index++]; 
		c3 = inputLine[index++];
		
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
			break;
		case 'T':
			Serial.println("T: Tool change");
		
			
			application.time4 = millis();           // grab the current time
			
			// request for idler and selector based on filament number
			if ((c2 >= '0')  && (c2 <= '4')) {
				application.toolChange(c2);
			} else {
				Serial.println(F("T: Invalid filament Selection"));
			}

			Serial1.print(F("ok\n"));              // send command acknowledge back to mk3 controller
			application.time5 = millis();          // grab the current time

			break;
		case 'C':
			Serial.println(F("C: Moving filament to extruder"));
			// move filament from selector ALL the way to printhead
			
			// filamentLoadToMK3();
			filamentController.filamentLoadWithBondTechGear();
			// delay(200);
			Serial1.print(F("ok\n"));
			break;

		case 'U':
			// UNLOAD FILIMENT COMMAND
			// request for filament unload

			Serial.println(F("U: Filament Unload Selected"));
			//*******************************************************************************************************
			//*  FIX:  don't go all the way to the end ... be smarter
			//******************************************************************************************************
			//* unparking is more elegant 10.12.1
			if (idlerController.status == QUICKPARKED) {
				idlerController.quickunParkIdler();             // un-park the idler from a quick park
			}
			if (idlerController.status == INACTIVE) {
				idlerController.unParkIdler();                    // turn on the idler motor
			}

			if ((c2 >= '0') && (c2 <= '4')) {

				filamentController.unloadFilamentToFinda();
				idlerController.parkIdler();
				Serial.println(F("U: Sending Filament Unload Acknowledge to MK3"));
				delay(200);
				Serial1.print(F("ok\n"));

			} else {
				Serial.println(F("U: Invalid filament Unload Requested"));
				delay(200);
				Serial1.print(F("ok\n"));
			}
			break;
		case 'L':
			Serial.println(F("L: Filament Load Selected"));

			if (idlerController.status == QUICKPARKED) {
				idlerController.quickunParkIdler();  // un-park the idler from a quick park
				Serial.println("idlerStatus == QUICKPARKED");				           
			}
			if (idlerController.status == INACTIVE) {
				idlerController.unParkIdler();                    // turn on the idler motor
				Serial.println("idlerStatus == INACTIVE");
			}

			if (colorSelector.status == INACTIVE)
				colorSelector.activate();         // turn on the color selector motor


			if ((c2 >= '0') && (c2 <= '4')) {
				Serial.print("c2: ");Serial.println((char)c2);
				Serial.println(F("L: Moving the bearing idler"));
				idlerController.select(c2);   // move the filament bearing selector stepper motor to the right spot
				// TODO :: ^^^^ comment the fuck out of the idler process here

				Serial.println(F("L: Moving the color selector"));
				colorSelector.select(c2);     // move the color Selector stepper Motor to the right spot
				Serial.println(F("L: Loading the Filament"));
				filamentController.loadFilamentToFinda();
				idlerController.parkIdler();             // turn off the idler roller

				Serial.println(F("L: Sending Filament Load Acknowledge to MK3"));

				delay(200);

				Serial1.print(F("ok\n"));

			} else {
				Serial.println(F("Error: Invalid Filament Number Selected"));
			}
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
			break;
		case 'P':
			// check FINDA status
			// Serial.println(F("Check FINDA Status Request"));
			if (filamentController.isFilamentLoaded() == 1) {
				Serial1.println(F("1"));
			}
			else {
				Serial1.println(F("0"));
			}
			Serial1.print(F("ok\n"));

			break;
		case 'F':  // 'F' command is acknowledged but no processing goes on at the moment
			// will be useful for flexible material down the road
			Serial.println(F("Filament Type Selected: "));
			Serial.println(c2);
			Serial1.print(F("ok\n"));                        // send back OK to the mk3
			break;
		default:
			Serial.println(F("ERROR: unrecognized command from the MK3 controller"));
			Serial1.print(F("ok\n"));

		}  // end of switch statement
	return index;
}



// Lists all available keyboard commands 
// TODO :: Keep this updated with new commands 
void CommandHandler::printCommandList(){
	Serial.println(F("C    : "));
	Serial.println(F("H    : Help / CMD menu : Dsplays this command list"));
	Serial.println(F("I#   : Idler Select    : Sets the position of the idler selector"));
	Serial.println(F("Q    : Disable Motors  : Disables all Setpper motors"));
	Serial.println(F("T<x> : Change tool     : Changes the colour selector to the given filament and then loads it  into the MMU"));
	Serial.println(F("U    : Unload filament : Unloads the filament that is currently loaded"));
	Serial.println(F("X    : Reset 		     : Resets the MMU unit"));
}

