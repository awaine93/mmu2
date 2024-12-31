#include <Arduino.h>
#include "application.h"
#include "StepperMotor.h"
#include "config.h"

StepperMotor::StepperMotor(uint8_t enablePin, uint8_t dirPin, uint8_t stepPin)
    : _enablePin(enablePin), _dirPin(dirPin), _stepPin(stepPin) 
    {
        // Do nothing
    }

void StepperMotor::enable() {
    digitalWrite(_enablePin, ENABLE);
    enabled = 1;
}

void StepperMotor::disable() {
    digitalWrite(_enablePin, DISABLE);
    enabled = 0;
}

void StepperMotor::setDirection(int direction) {
    digitalWrite(_dirPin, direction);
}

void StepperMotor::step(int steps, unsigned int delayTime) {
    
    for (int i = 0; i <= (steps * STEPSIZE); i++) {
        digitalWrite(_stepPin, HIGH);
        delayMicroseconds(PINHIGH);
        digitalWrite(_stepPin, LOW);
        delayMicroseconds(PINLOW);
        
        delayMicroseconds(delayTime);
    }
}