#ifndef FILAMENTCONTROLLER_H
#define FILAMENTCONTROLLER_H

#include <Arduino.h>


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
    void activate();
    void deActivate();
	
    // Variables
    int filStatus;

};

#endif // FILAMENTCONTROLLER_H
