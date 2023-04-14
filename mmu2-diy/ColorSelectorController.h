#ifndef COLORSELECTORCONTROLLER_H
#define COLORSELECTORCONTROLLER_H

#include <Arduino.h>


class ColorSelectorController
{
public:
	ColorSelectorController();
    void activate();
    void select(char selection);
    void csTurnAmount(int steps, int direction);
    void initColorSelector();
    void deActivate();
    void syncColorSelector();

    // Variables 
    int status;
    int currentPosition;
};

#endif // COLORSELECTORCONTROLLER_H
