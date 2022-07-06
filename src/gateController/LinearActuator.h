#ifndef LINEARACTUATOR_H
#define LINEARACTUATOR_H

#include <Preferences.h>

#include "Motor.h"
#include "Encoder.h"

#define MOVE_IN 0
#define MOVE_OUT 1

#define MINIMUM_THROTTLE 65.0f
#define ACCELERATION_RATE 10.f

// PREFERENCES KEYS
#define COUNT_PER_UNIT_KEY "countPerUnit"
#define HOME_DIRECTION_KEY "homeDirection"

typedef void (*callbackFunc)(int);

class LinearActuator
{
private:
    Motor *Motor1;
    Encoder *Encoder1;
    Preferences *MyPreferences;

    uint16_t calibrationCounter;
    float countsPerUnit; // for example, counter value 6800 equals = 1 unit = 1mm or 500 = 360° = 1 unit
    bool homeDirection;

    void readPreferences();
    void setCountPerUnit(float value);

public:
    LinearActuator(uint8_t pinEN, uint8_t pinPWM, uint8_t pinCS, uint8_t pinDirA, uint8_t pinDirB, uint8_t pinEncChA, uint8_t pinEncChB, Preferences *Preference);

    void enable();
    void disable();

    // 1. axis gets stoped
    // 2. encoder value is stored
    // 3. axis will move in the given direction at a given throttle for a given time
    // 4. axis gets stoped
    // 5. nextstep call finishCallibration(float traveledWayInMM) methode to finish the callibration
    void startCalibration(bool direction, float throttle, uint16_t movementTime);

    // 1. the traveld way gets devided by the difference between the encoder values, the result (countsPerUnit) is set and stored in the eeprom
    float finishCalibration(float traveledWayInMM);

    void setHomeDirection(bool direction);
    void setCurrentLimit(float value);
    void setInvertDirection(bool value);

    float getPosition(); // contains the automatic converted unit value that is calculated by counter value * countsPerUnit

    // clears also the encoder conter
    void moveHome(float throttle = 100.f);
    void moveAbsolute(float position, float throttle = 100.f, callbackFunc response = 0);
    void moveRelative(float deltaPosition, float throttle = 100.f);
};

#endif
