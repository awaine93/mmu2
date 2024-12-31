#define SERIAL1ENABLED    1
#define ENABLE LOW                // 8825 stepper motor enable is active low
#define DISABLE HIGH              // 8825 stepper motor disable is active high

#define MMU2_VERSION "4.3  03/02/23"

#define STEPSPERMM  9 		      // these are the number of steps required to travel 1 mm using the extruder motor --- This value was set to 144 but this wascalculated for 16th microstepping
#define LENGTHTOMK3GEAR 32		  // this is the length from the the filament sesnor to extruder gear after and will 
								  // extrude this length after the sensor has been triggered

#define FW_VERSION  132//90             // config.h  (MM-control-01 firmware)
#define FW_BUILDNR 132 //80             // config.h  (MM-control-01 firmware)

#define ORIGINALCODE 0            // code that is no longer needed for operational use
extern int command;

// changed from 125 to 115 (10.13.18)
#define MAXROLLERTRAVEL 125         // number of steps that the roller bearing stepper motor can travel

#define FULL_STEP  1
#define HALF_STEP  2
#define QUARTER_STEP 4
#define EIGTH_STEP 8
#define SIXTEENTH_STEP 16

#define STEPSIZE SIXTEENTH_STEP    // setup for each of the three stepper motors (jumper settings for M0,M1,M2) on the RAMPS 1.x board

#define STEPSPERREVOLUTION 200     // 200 steps per revolution  - 1.8 degree motors are being used

#define MAXSELECTOR_STEPS   1890   // maximum number of selector stepper motor (used to move all the way to the right or left

#define MMU2TOEXTRUDERSTEPS STEPSIZE*STEPSPERREVOLUTION*19   // for the 'T' command 

#define CW 0
#define CCW 1

#define INACTIVE 0                           // used for 3 states of the idler stepper motor (parked)
#define ACTIVE 1                             // not parked 
#define QUICKPARKED 2                            // quick parked


//************************************************************************************
//* this resets the selector stepper motor after the selected number of tool changes
//* changed from 25 to 10 (10.10.18)
//* chagned from 10 to 8 (10.14.18)
//*************************************************************************************
#define TOOLSYNC 20                         // number of tool change (T) commands before a selector resync is performed



#define PINHIGH 10                    // how long to hold stepper motor pin high in microseconds
#define PINLOW  10                    // how long to hold stepper motor pin low in microseconds



// the MMU2 currently runs at 21mm/sec (set by Slic3r) for 2 seconds (good stuff to know)
//
// the load duration was chagned from 1 second to 1.1 seconds on 10.8.18 (as an experiment)
// increased from 1.1 to 1.5 seconds on 10.13.18 (another experiment)
#define LOAD_DURATION 1600                 // duration of 'C' command during the load process (in milliseconds)


// changed from 21 mm/sec to 30 mm/sec on 10.13.18
#define LOAD_SPEED 30                   // load speed (in mm/second) during the 'C' command (determined by Slic3r setting)
#define INSTRUCTION_DELAY 25          // delay (in microseconds) of the loop



#define IDLERSTEPSIZE 25 //original:23    // steps to each roller bearing  
// float bearingAbsPos[5] = {1, 24, 48, 72, 96}; // absolute position of roller bearing stepper motor
extern float bearingAbsPos[5];

#define CSSTEPS 355 // original: 357                                            


//*************************************************************************************************
//  Delay values for each stepper motor 
//*************************************************************************************************
#define IDLERMOTORDELAY  530     //530 useconds      (idler motor)
#define EXTRUDERMOTORDELAY 50     // 150 useconds    (controls filament feed speed to the printer)
#define COLORSELECTORMOTORDELAY 60 // 60 useconds    (selector motor)


// added this pin as a debug pin (lights a green LED so I can see the 'C0' command in action
#define greenLED 14

// modified code on 10.2.18 to accomodate RAMPS 1.6 board mapping


// Y - Axis
#define idlerDirPin		A7
#define idlerStepPin	A6
#define idlerEnablePin	A2

// Z - axis
#define extruderDirPin		48 //  pin 48 for extruder motor direction pin
#define extruderStepPin		46 //  pin 48 for extruder motor stepper motor pin
#define extruderEnablePin	A8 //  pin A8 for extruder motor rst/sleep motor pin

// X - Axis
#define colorSelectorDirPin		A1 //color selector stepper motor (driven by trapezoidal screw)
#define colorSelectorStepPin	A0
#define colorSelectorEnablePin	38

#define findaPin  A3 
// this is pin D3 on the arduino MEGA 2650
#define filamentSwitch 3       // this switch was added on 10.1.18 to help with filament loading (X- signal on the RAMPS board)

extern Application application;
//extern FilamentController filamentController;
//extern IdlerController IdlerController;