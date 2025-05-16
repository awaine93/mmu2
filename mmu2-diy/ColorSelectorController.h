#ifndef COLORSELECTORCONTROLLER_H
#define COLORSELECTORCONTROLLER_H

#include <Arduino.h>
#include "StepperMotor.h"


class ColorSelectorController
{
public:
	ColorSelectorController();
    void enable();
    void disable();
    void select(char selection);
    void csTurnAmount(int steps, int direction);
    void initColorSelector();
    
    void syncColorSelector();

    // Variables 
    int csStatus;
    int currentPosition;

    StepperMotor _colorSelectorMotor;
};

#endif // COLORSELECTORCONTROLLER_H
