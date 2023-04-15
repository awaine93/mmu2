#ifndef IDLERCONTROLLER_H
#define IDLERCONTROLLER_H

#include <Arduino.h>


class IdlerController
{

public:
	IdlerController();
    
	void initIdlerPosition();
    void quickunParkIdler();
    void unParkIdler();
    void specialunParkIdler();
    void parkIdler();
    void quickParkIdler();
    void specialParkIdler();
    void select(char filament);
    void turnamount(int steps, int dir);
    // Variables
    int status;
};

#endif // IDLERCONTROLLER_H
