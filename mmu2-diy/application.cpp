// CSK MMU2 Controller Version
//
//  Code developed by Chuck Kozlowski
//  September 19, 2018
//
//  Code was developed because I am impatiently waiting for my MMU2 to arrive (due in December, 2018) so I thought
//  I would develop some code to operate the PRUSA MMU2 hardware
//
// This code uses 3 stepper motor controllers and 1 Pinda filament sensor, and 1 additional filament sensor on the mk3 extruder top
//
//
//  Work to be done:  Interface Control with the Einsy Board (MK3) - (work completed on 9.25.18)
//                    Refine speed and acceleration settings for each stepper motor
//                    Failure Recovery Modes - basically non-existent (work completed on 10.5.18)
//
//                    Uses the serial interface with a host computer at the moment - probably could do some smarter things
//                                                                                   like selection switches and some LEDs.
//                   10.14.18 Leave the Selector Stepper Motor ON ... appear to be losing position with the selector during operation
//                            [now using the J-4218HB2301 Stepper Motor - (45 N-cm torque) as the idler stepper motor]
//                   10.14.18 Added yet another idler command set (specialparkidler() and specialunParkIdler) - used within the 'C' Command
//                            Idler management is a bit of challenge,  probably has more to do with my coding skills (or lack thereof).
//                   10.12.18 Minor fix to idler parking ... now use quickparkidler() after 'C' command and quickunParkIdler() at beginning of 'T' command
//                              This helps to speed up the idler movements during 'T' and 'C' commands
//                   10.5.18  Made major tweak to the 'C' command, now matches the speed of the mk3 extruder gear (see slic3r 'load' setting)
//                   10.2.18  Moved from breadboard to RAMPS 1.6 Board and remapped ALL addresses
//                            Discovered the filament idler motor needed to be set at a higher torque (more current)
//                            (this was affected filament load consistency)
//                   10.2.18  Major Disaster, lost my codebase on my PC (I am an idiot)
//                            Thank God for github so I could recover a week old version of my code
//                   10.1.18  Added filament sensor to the extruder head (helps reliability


//#include <SoftwareSerial.h>
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

CommandHandler commandHandler;
extern FilamentController	filamentController;
extern IdlerController idlerController;
extern ColorSelectorController colorSelector;


int trackToolChanges = 0;
int extruderMotorStatus = INACTIVE;


int repeatTCmdFlag = INACTIVE;    // used by the 'C' command processor to avoid processing multiple 'C' commands

int filamentSelection = 0;       // keep track of filament selection (0,1,2,3,4))
int dummy[100];
char currentExtruder = '0';

int firstTimeFlag = 0;
int earlyCommands = 0;           // forcing communications with the mk3 at startup

int toolChangeCount = 0;

char receivedChar;
boolean newData = false;

// Global variables
int command = 0;
float bearingAbsPos[5] = {0, IDLERSTEPSIZE, IDLERSTEPSIZE * 2, IDLERSTEPSIZE * 3, IDLERSTEPSIZE * 4};


//SoftwareSerial Serial1(10,11); // RX, TX (communicates with the MK3 controller board

unsigned long time0, time1, time2, time3, time4, time5;


void Application::setup() {
	// static int findaStatus;

	int waitCount;

	Serial.begin(500000);  // startup the local serial interface (changed to 2 Mbaud on 10.7.18
	while (!Serial) {
		; // wait for serial port to connect. needed for native USB port only
		Serial.println(F("waiting for serial port"));
	}

	Serial.println(MMU2_VERSION);
	// THIS DELAY IS CRITICAL DURING POWER UP/RESET TO PROPERLY SYNC WITH THE MK3 CONTROLLER BOARD
	delay(4000); // this is key to syncing to the MK3 controller - currently 4 seconds

	Serial1.begin(115200); // startup the mk3 serial

	//Serial.println(F("started the mk3 serial interface"));
	delay(100);


	Serial.println(F("Sending START command to mk3 controller board"));
	// THIS NEXT COMMAND IS CRITICAL ... IT TELLS THE MK3 controller that an MMU is present
	Serial1.print(F("start\n")); // attempt to tell the mk3 that the mmu is present

	//  check the serial interface to see if it is active
	waitCount = 0;
	while (!Serial1.available()) {
		Serial.println(F("Waiting for message from mk3"));
		delay(1000);
		++waitCount;
		if (waitCount >= 10) {
			Serial.println(F("10 seconds have passed, aborting wait for mk3 to respond"));
			goto continue_processing;
		}
	}
	Serial.println(F("inbound message from mk3"));

continue_processing:

	pinMode(idlerDirPin, OUTPUT);
	pinMode(idlerStepPin, OUTPUT);

	pinMode(findaPin, INPUT);  // pinda Filament sensor
	pinMode(filamentSwitch, INPUT);

	pinMode(idlerEnablePin, OUTPUT);
	// pinMode(bearingRstPin, OUTPUT);

	pinMode(extruderEnablePin, OUTPUT);
	pinMode(extruderDirPin, OUTPUT);
	pinMode(extruderStepPin, OUTPUT);

	pinMode(colorSelectorEnablePin, OUTPUT);
	pinMode(colorSelectorDirPin, OUTPUT);
	pinMode(colorSelectorStepPin, OUTPUT);

	pinMode(greenLED, OUTPUT);                         // green LED used for debug purposes

	Serial.println(F("finished setting up input and output pins"));

	// Turn on all three stepper motors
	digitalWrite(idlerEnablePin, ENABLE);           // enable the roller bearing motor (motor #1)
	digitalWrite(extruderEnablePin, ENABLE);        //  enable the extruder motor  (motor #2)
	digitalWrite(colorSelectorEnablePin, ENABLE);  // enable the color selector motor  (motor #3)

	Serial.println(F("Syncing the Idler Selector Assembly"));             // do this before moving the selector motor
	idlerController.initIdlerPosition();    // reset the roller bearing position

 	// TODO :: Write a function to check if their is filament loaded then to unload and calibrate the colour selector
#ifdef NOTDEF
	if (filamentController.isFilamentLoaded()) {               // check to see if filament in the bowden tube (between the mmu2 and mk3
		Serial.println(F("Filament was in the bowden tube at startup, unloading filament automatically"));
		filamentController.unloadFilamentToFinda();            //
	}
#endif 

	Serial.println(F("Syncing the Filament Selector Assembly"));
	if (!filamentController.isFilamentLoaded()) {
		colorSelector.initColorSelector();   // reset the color selector if there is NO filament present
	} else {
		Serial.println(F("Unable to clear the Color Selector, please remove filament"));
	}

	Serial.println(F("Inialialization Complete, let's multicolor print ...."));

} 

void Application::loop() {

	delay(100);                       // wait for 100 milliseconds
	checkSerialInterface();           // check the serial interface for input commands from the mk3

	commandHandler.keyboardCommands();

} 

void Application::disableAllMotors(){
	digitalWrite(colorSelectorEnablePin, DISABLE); 
	digitalWrite(extruderEnablePin, DISABLE);  
	digitalWrite(idlerEnablePin, DISABLE);  
}

// Handles any command incomming from the printer's serial
void Application::checkSerialInterface() {
	unsigned char c1, c2, c3;
	int cnt;
	String inputLine;
	int findaStatus;
	int index = 0;

	if ((cnt = Serial1.available()) > 0) {

		inputLine = Serial1.readString();      // fetch the command from the mmu2 serial input interface

		if (inputLine[0] != 'P') {
			Serial.print(F("MMU Command: "));
			Serial.println(inputLine);
		}
process_more_commands:  // parse the inbound command

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
		
			
			time4 = millis();           // grab the current time
			
			// request for idler and selector based on filament number
			if ((c2 >= '0')  && (c2 <= '4')) {
				toolChange(c2);
			} else {
				Serial.println(F("T: Invalid filament Selection"));
			}

			Serial1.print(F("ok\n"));              // send command acknowledge back to mk3 controller
			time5 = millis();          // grab the current time

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
			//* unparking is more elegant 10.12.18
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
				idlerController.quickunParkIdler();             // un-park the idler from a quick park
			}
			if (idlerController.status == INACTIVE) {
				idlerController.unParkIdler();                    // turn on the idler motor
			}

			if (colorSelector.status == INACTIVE)
				colorSelector.activate();         // turn on the color selector motor

			if ((c2 >= '0') && (c2 <= '4')) {
				Serial.println(F("L: Moving the bearing idler"));
				idlerController.idlerSelector((int)c2);   // move the filament selector stepper motor to the right spot
				Serial.println(F("L: Moving the color selector"));
				colorSelector.select(c2);     // move the color Selector stepper Motor to the right spot
				Serial.println(F("L: Loading the Filament"));
				// filamentController.loadFilament(CCW);
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
			findaStatus = filamentController.isFilamentLoaded();
			if (findaStatus == 1) {
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
			Serial.print(F("ERROR: unrecognized command from the MK3 controller"));
			Serial1.print(F("ok\n"));


		}  // end of switch statement
#ifdef NOTDEF
		if (cnt != 3) {

			Serial.print(F("Index: "));
			Serial.print(index);
			Serial.print(F(" cnt: "));
			Serial.println(cnt);
		}
#endif
	}  // end of cnt > 0 check

	if (index < cnt) {
#ifdef NOTDEF
		Serial.println(F("More commands in the buffer"));
#endif

		goto process_more_commands;
	}
	// }  // check for early commands

}

//****************************************************************************************************
//* this routine is the common routine called for fixing the filament issues (loading or unloading)
//****************************************************************************************************
void Application::fixTheProblem(String statement) {
	Serial.println(F(""));
	Serial.println(F("********************* ERROR ************************"));
	Serial.println(statement);       // report the error to the user
	Serial.println(F("********************* ERROR ************************"));
	Serial.println(F("Clear the problem and then hit any key to continue "));
	Serial.println(F(""));

	idlerController.parkIdler();                                    // park the idler stepper motor
	digitalWrite(colorSelectorEnablePin, DISABLE);  // turn off the selector stepper motor

	//quickParkIdler();                   // move the idler out of the way
	// specialParkIdler();

	while (!Serial.available()) {
		//  wait until key is entered to proceed  (this is to allow for operator intervention)
	}
	Serial.readString();  // clear the keyboard buffer

	idlerController.unParkIdler();                             // put the idler stepper motor back to its' original position
	digitalWrite(colorSelectorEnablePin, ENABLE);  // turn ON the selector stepper motor
	delay(1);                                  // wait for 1 millisecond

	//specialunParkIdler();
	//idlerController.unParkIdler();
	//quickunParkIdler();                 // re-enage the idler
}

void recvOneChar() {
	if (Serial.available() > 0) {
		receivedChar = Serial.read();
		newData = true;
	}
}

void showNewData() {
	if (newData == true) {
		Serial.print(F("This just in ... "));
		Serial.println(receivedChar);
		newData = false;
	}
}

#ifdef ORIGINALCODE

void processKeyboardInput() {


	while (newData == false) {
		recvOneChar();
	}

	showNewData();      // character received

	Serial.print(F("Filament Selected: "));
	Serial.println(receivedChar);

	switch (receivedChar) {
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
		if (idlerController.status == INACTIVE){
			digitalWrite(idlerEnablePin, ENABLE);   // turn on the roller bearing stepper motor
			idlerController.status = ACTIVE;
		}
			
		if (colorSelector.status == INACTIVE)
			colorSelector.activate();         // turn on the color selector motor


		idlerController.idlerSelector((int)receivedChar);   // move the filament selector stepper motor to the right spot
		colorSelector.select(receivedChar);     // move the color Selector stepper Motor to the right spot

		break;
	case 'd':                             // de-active the bearing roller stepper motor and color selector stepper motor
	case 'D':
		idlerController.parkIdler();
		colorSelector.deActivate();
		break;
	case 'l':                            // start the load process for the filament
	case 'L':
		// idlerController.unParkIdler();
		if (idlerController.status == INACTIVE)
			idlerController.unParkIdler();
		filamentController.loadFilament(CCW);
		idlerController.parkIdler();          // move the bearing rollers out of the way after a load is complete
		break;
	case 'u':                           // unload the filament from the MMU2 device
	case 'U':
		idlerController.unParkIdler();           // working on this command
		filamentController.loadFilament(CW);
		idlerController.parkIdler();         // after the unload of the filament, move the bearing rollers out of the way
		break;
	case 't':
	case 'T':
		colorSelector.csTurnAmount(200, CW);
		delay(1000);
		colorSelector.csTurnAmount(200, CCW);
		break;
	default:
		Serial.println(F("Invalid Serial Output Selection"));
	} // end of switch statement
}
#endif

//
// (T) Tool Change Command - this command is the core command used my the mk3 to drive the mmu2 filament selection
//
void Application::toolChange( char selection) {
	int newExtruder;

	++toolChangeCount;                             // count the number of tool changes
	++trackToolChanges;

	//**********************************************************************************
	// * 10.10.18 added an automatic reset of the tracktoolchange counter since going to
	//            filament position '0' move the color selection ALL the way to the left
	//*********************************************************************************
	if (selection == '0')  {
		trackToolChanges = 0;
	}

	Serial.print(F("Tool Change Count: "));
	Serial.println(toolChangeCount);

	newExtruder = selection - 0x30;                // convert ASCII to a number (0-4)

	//***********************************************************************************************
	// code snippet added on 10.8.18 to help the 'C' command processing (happens after 'T' command
	//***********************************************************************************************
	if (newExtruder == filamentSelection) {  // already at the correct filament selection
		
		if (digitalRead(findaPin) == 1) {            // no filament loaded
			Serial.println(F("toolChange: filament not currently loaded, loading ..."));

			idlerController.idlerSelector((int)selection);   // move the filament selector stepper motor to the right spot
			colorSelector.select(selection);     // move the color Selector stepper Motor to the right spot
			// &&&&&&&&&&&&&&&&&&&&&&&&&&&&&
			filamentController.filamentLoadToMK3();
			idlerController.quickParkIdler();           // command moved here on 10.13.18
			//****************************************************************************************
			//*  added on 10.8.18 to help the 'C' command
			//***************************************************************************************
			repeatTCmdFlag = INACTIVE;   // used to help the 'C' command
		} else {
			Serial.println(F("toolChange:  filament already loaded to mk3 extruder"));
			//*********************************************************************************************
			//* added on 10.8.18 to help the 'C' Command
			//*********************************************************************************************
			repeatTCmdFlag = ACTIVE;     // used to help the 'C' command to not feed the filament again
		}

	}  else {                                 // different filament position
		//********************************************************************************************
		//* added on 19.8.18 to help the 'C' Command
		//************************************************************************************************
		repeatTCmdFlag = INACTIVE;              // turn off the repeat Commmand Flag (used by 'C' Command)
		if (filamentController.isFilamentLoaded()) {
			idlerController.idlerSelector((int)currentExtruder);    // point to the current extruder
			filamentController.unloadFilamentToFinda();          // have to unload the filament first
		}


		if (trackToolChanges > TOOLSYNC) {             // reset the color selector stepper motor (gets out of alignment)
			Serial.println(F("Synchronizing the Filament Selector Head"));
			colorSelector.syncColorSelector();

			colorSelector.activate();                  // turn the color selector motor back on
			colorSelector.currentPosition = 0;                   // reset the color selector

			// colorSelector('0');                       // move selector head to position 0

			trackToolChanges = 0;

		}

		idlerController.idlerSelector((int)selection);
		colorSelector.select(selection);

		filamentController.filamentLoadToMK3();                // moves the idler and loads the filament

		filamentSelection = newExtruder;
		currentExtruder = selection;
		//quickParkIdler();                 // command moved here on 10.13.18
		idlerController.parkIdler();
	}

	
	                            // move the idler away


}  // end of ToolChange processing



Application::Application()
{
	// nothing to do in the constructor
}
