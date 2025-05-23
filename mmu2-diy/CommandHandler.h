#ifndef COMMANDHANDLER_H
#define COMMANDHANDLER_H

#include <Arduino.h>


class CommandHandler
{
public:
	CommandHandler();
	void keyboardCommands();
	int handlePrinterCommand(String, int);
 	void printKeyboardCommandList();
	void printPrinterCommandList();
	void printMotorStatus();
};


#endif // COMMANDHANDLER_H
