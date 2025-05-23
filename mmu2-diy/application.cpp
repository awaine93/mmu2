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
int currentExtruder = 0;

int firstTimeFlag = 0;
int earlyCommands = 0;           // forcing communications with the mk3 at startup

int toolChangeCount = 0;

char receivedChar;
boolean newData = false;

// Global variables
int command = 0;

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

	//  check the serial interface to see if it is active
	waitCount = 0;
	while (!Serial1.available()) {
		Serial.println(F("Sending START command to mk3 controller board"));
		// THIS NEXT COMMAND IS CRITICAL ... IT TELLS THE MK3 controller that an MMU is presenta
		Serial1.print(F("start\n")); // attempt to tell the mk3 that the mmu is present
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
	
	pinMode(idlerEnablePin, OUTPUT);
	pinMode(idlerDirPin, OUTPUT);
	pinMode(idlerStepPin, OUTPUT);

	pinMode(findaPin, INPUT);  // pinda Filament sensor
	pinMode(filamentSwitch, INPUT);
	
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
	idlerController.enable();					// enable the roller bearing motor (motor #1)        
	//filamentController.enable();      //  enable the extruder motor  (motor #2) -- not sure we want to enable this on startup 
	colorSelector.enable(); // enable the color selector motor  (motor #3)

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

	idlerController.disable();
	colorSelector.disable();

	Serial.println(F("Inialialization Complete, let's multicolor print ...."));

} 

void Application::loop() {

	delay(100);                       // wait for 100 milliseconds
	checkSerialInterface();           // check the serial interface for input commands from the mk3

	commandHandler.keyboardCommands();

} 

void Application::disableAllMotors(){
	colorSelector.disable();
	idlerController.disable();
	filamentController.disable();
}

// Handles any command incomming from the printer's serial
void Application::checkSerialInterface() {
	int cnt;
	String inputLine;
	int findaStatus;
	int index = 0;

	if ((cnt = Serial1.available()) > 0) {

		inputLine = Serial1.readString();      // fetch the command from the mmu2 serial input interface
		
		Serial.print(F("MMU Command: "));
		Serial.println(inputLine);
	
process_more_commands:  // parse the inbound command
		Serial.println(F("Processing the inbound command"));
		index = commandHandler.handlePrinterCommand(inputLine, index);

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
	filamentController.disable(); // turn off the extruder stepper motor as well
	colorSelector.disable(); // turn off the selector stepper motor

	//quickParkIdler();                   // move the idler out of the way
	// specialParkIdler();

	while (!Serial.available()) {
		//  wait until key is entered to proceed  (this is to allow for operator intervention)
	}
	Serial.readString();  // clear the keyboard buffer
	filamentController.enable();
	delay(1);
	idlerController.unParkIdler();  // put the idler stepper motor back to its' original position
	colorSelector.disable();  		// turn ON the selector stepper motor

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
		if (idlerController._idlerMotor.enabled == 0){
			idlerController.enable();   // turn on the roller bearing stepper motor
		}
			
		if (colorSelector._colorSelectorMotor.enabled == 0){
			colorSelector._colorSelectorMotor.enable();         // turn on the color selector motor
		}

		idlerController.select(receivedChar);   // move the filament selector stepper motor to the right spot
		colorSelector.select(receivedChar);     // move the color Selector stepper Motor to the right spot

		break;
	case 'd':                             // de-active the bearing roller stepper motor and color selector stepper motor
	case 'D':
		idlerController.parkIdler();
		filamentController.disable(); // turn off the extruder stepper motor as well
		colorSelector.enable();
		break;
	case 'l':                            // start the load process for the filament
	case 'L':
		// idlerController.unParkIdler();
		if (idlerController.status == INACTIVE)
			filamentController.enable();
			delay(1);
			idlerController.unParkIdler();
		filamentController.loadFilament(CCW);
		idlerController.parkIdler();          // move the bearing rollers out of the way after a load is complete
		filamentController.disable(); // turn off the extruder stepper motor as well
		break;
	case 'u':                           // unload the filament from the MMU2 device
	case 'U':
		filamentController.enable();
		delay(1);
		idlerController.unParkIdler();           // working on this command
		filamentController.loadFilament(CW);
		idlerController.parkIdler();         // after the unload of the filament, move the bearing rollers out of the way
		filamentController.disable(); // turn off the extruder stepper motor as well
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
void Application::toolChange(char selection) {
	int newExtruder;

	Serial.print(F("Application.toolChange(): filament selected: "));
	Serial.println(selection);

	++toolChangeCount;                             // count the number of tool changes
	++trackToolChanges;

	// reset the tool change count if the colour selector has been to 0 position as we know it will be on the left no matter what
	if (selection == 0)  {
		trackToolChanges = 0;
	}

	Serial.print(F("Application.toolChange(): Tool Change Count: "));
	Serial.println(toolChangeCount);

	newExtruder = selection - 0x30;                // convert ASCII to a number (0-4)

	if (newExtruder == filamentSelection) {  // already at the correct filament selection
		
		if (filamentController.isFilamentLoaded() == 0) {            // no filament loaded
			Serial.println(F("Application.toolChange(): filament not currently loaded, loading ..."));

			idlerController.select(selection);   // move the filament selector stepper motor to the right spot
			colorSelector.select(selection);     // move the color Selector stepper Motor to the right spot
			filamentController.filamentLoadToMK3();
			
			colorSelector.disable();
			idlerController.parkIdler();
			filamentController.disable();
			
			repeatTCmdFlag = INACTIVE;   // used to help the 'C' command
		} else {
			Serial.println(F("Application.toolChange():  filament already loaded to mk3 extruder"));
			
			repeatTCmdFlag = ACTIVE;     // used to help the 'C' command to not feed the filament again
		}

	}  else {                                 // different filament position
		repeatTCmdFlag = INACTIVE;              // turn off the repeat Commmand Flag (used by 'C' Command)
		if (filamentController.isFilamentLoaded()) {
			Serial.println(F("Application.toolChange(): filament currently loaded in selector, unloading ..."));
			idlerController.select(currentExtruder);   	 // point to the current extruder
			filamentController.unloadFilamentToFinda();  // have to unload the filament first
		}

		if (trackToolChanges > TOOLSYNC) {             // reset the color selector stepper motor (gets out of alignment)
			Serial.println(F("Application.toolChange(): syncing the Filament Selector Head"));
			colorSelector.syncColorSelector();

			colorSelector.enable();
			colorSelector.currentPosition = 0;
			trackToolChanges = 0;

		}

		idlerController.select(selection);
		colorSelector.select(selection);

		filamentController.filamentLoadToMK3();

		filamentSelection = newExtruder;
		currentExtruder = selection;

		idlerController.parkIdler();
		colorSelector.disable();
		filamentController.disable(); 
		
	}
}  // end of ToolChange processing



Application::Application()
{
	// nothing to do in the constructor
}
