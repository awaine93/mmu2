#ifndef APPLICATION_H
#define APPLICATION_H

#include <Arduino.h>


class Application
{
public:
	Application();

	void setup();
	void loop();
	void disableAllMotors();
	void fixTheProblem(String);
	void toolChange(char);
	void checkSerialInterface();

	// Variables
	int repeatTCmdFlag;
	int filamentSelection;
	char currentExtruder; 
	unsigned long time0, time1, time2, time3, time4, time5;

};

#endif // APPLICATION_H

