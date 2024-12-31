#ifndef STEPPERMOTOR_H
#define STEPPERMOTOR_H

#include <Arduino.h>
#include "application.h"

class StepperMotor {
    public:
        StepperMotor(uint8_t enablePin, uint8_t dirPin, uint8_t stepPin);
        void enable();
        void disable();
        void setDirection(int direction);
        void step(int steps, unsigned int delayTime);

        int enabled;

    private:
        uint8_t _enablePin;
        uint8_t _dirPin;
        uint8_t _stepPin;
};

#endif