#ifndef FILAMENTCONTROLLER_H
#define FILAMENTCONTROLLER_H

#include <Arduino.h>
#include "StepperMotor.h"



class FilamentController
{
public:
	FilamentController();
    int isFilamentLoaded();
    void unloadFilamentToFinda();
    void filamentLoadWithBondTechGear();
    void filamentLoadToMK3();
    void loadFilamentToFinda();
    void feedFilament(unsigned int);
    void loadFilament(int);
    void printFilamentStats();
    void enable();
    void disable();
	
    // Variables
    int filStatus;
    StepperMotor _filamentMotor;

};

#endif // FILAMENTCONTROLLER_H
