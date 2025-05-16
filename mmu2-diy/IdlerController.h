#ifndef IDLERCONTROLLER_H
#define IDLERCONTROLLER_H

#include <Arduino.h>
#include "StepperMotor.h"


class IdlerController
{

public:
	IdlerController();
    
	void initIdlerPosition();
    void quickunParkIdler();
    void unParkIdler();
    void parkIdler();
    void quickParkIdler();
    void select(int filament);
    void turnamount(int steps, int dir);
    void enable();
    void disable();
    
    // Variables
    int status;

    StepperMotor _idlerMotor;
};

#endif // IDLERCONTROLLER_H
