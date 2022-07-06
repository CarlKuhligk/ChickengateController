#include "LinearActuator.h"

void LinearActuator::readPreferences()
{
  this->homeDirection = this->MyPreferences->getBool(HOME_DIRECTION_KEY);
  this->countsPerUnit = this->MyPreferences->getFloat(COUNT_PER_UNIT_KEY);
}

void LinearActuator::setCountPerUnit(float value)
{
  // write only different values
  if (value != countsPerUnit)
  {
    this->countsPerUnit = value;
    this->MyPreferences->putFloat(COUNT_PER_UNIT_KEY, this->countsPerUnit);
  }
}

float LinearActuator::getPosition()
{
  return (float)Encoder1->getCount() * countsPerUnit;
}

LinearActuator::LinearActuator(uint8_t pinEN, uint8_t pinPWM, uint8_t pinCS, uint8_t pinDirA, uint8_t pinDirB, uint8_t pinEncChA, uint8_t pinEncChB, Preferences *Preference)
{
  this->MyPreferences = Preference;
  this->Motor1 = new Motor(pinEN, pinPWM, pinCS, pinDirA, pinDirB, Preference);
  this->Encoder1 = new Encoder(pinEncChA, pinEncChB);

  readPreferences();

  Serial.printf("countsPerUnit: %.3f homeDirection: %d\n", countsPerUnit, homeDirection);
}

void LinearActuator::enable()
{
  this->Motor1->enable();
}

void LinearActuator::disable()
{
  this->Motor1->disable();
}

void LinearActuator::startCalibration(bool direction, float throttle, uint16_t movementTime)
{
  this->Motor1->stop();
  this->calibrationCounter = (uint16_t)this->Encoder1->getCount();
  this->Motor1->start(direction, throttle);
  delay(movementTime);
  this->Motor1->stop();
}

float LinearActuator::finishCalibration(float traveledWayInMM)
{
  float factor = traveledWayInMM / ((uint16_t)Encoder1->getCount() - calibrationCounter);
  setCountPerUnit(factor);
  return factor;
}

void LinearActuator::setCurrentLimit(float value)
{
  this->Motor1->setCurrentLimit(value);
}

void LinearActuator::setInvertDirection(bool value)
{
  this->Motor1->setInvertDirection(value);
}

void LinearActuator::setHomeDirection(bool direction)
{
  // write only different values
  if (direction != homeDirection)
  {
    this->homeDirection = direction;
    this->MyPreferences->putBool(HOME_DIRECTION_KEY, this->homeDirection);
  }
}

void LinearActuator::moveHome(float throttle)
{
  float current;
  float smoothThrottle = MINIMUM_THROTTLE;
  do
  {
    if (smoothThrottle < throttle)
      smoothThrottle += 1.5;
    if (smoothThrottle > throttle)
      smoothThrottle = throttle;
    Motor1->start(homeDirection, smoothThrottle);
    delay(50);
    current = Motor1->getCurrent();
  } while (current > 0 && current < Motor1->getCurrentLimit());

  Motor1->stop();
  Encoder1->clear();
}

void LinearActuator::moveAbsolute(float targetPosition, float throttle, callbackFunc response)
{
  if (throttle < MINIMUM_THROTTLE)
    throttle = MINIMUM_THROTTLE;

  bool specialCaseTargetZero = false;
  bool specialCaseExitLoop = false;

  if (targetPosition <= 0.) {
    specialCaseTargetZero = true;
    targetPosition = 0.;
  }

  float current;

  float currentPosition = getPosition();
  float p0 = currentPosition;                                        // start position
  float sA = abs((throttle - MINIMUM_THROTTLE) / ACCELERATION_RATE); // way of acceleration
  float p1, p2;                                                      // position to start deaccelerate
  float sX;                                                          // can be sA or delta/2
  float moved;                                                       // traveld way
  float smoothThrottle = MINIMUM_THROTTLE;


  float delta = targetPosition - currentPosition; // distance from target position
  bool direction;

  /*
         p1,p2
           ^
          / \
         /   \
        /     \
       /       \
      /         \
    ___/           \___
     |<- delta ->|
     |           |
     p0      targetPosition
  */

  if (abs(delta) > (2.0f * sA))
    sX = sA;
  else
    sX = abs(delta) / 2.0f;

  /*
      ____________MOVE OUT____________            _____________MOVE IN______________

           p1                 p2                        p2                 p1
     |< sX >|________V_________|< sX >|           |< sX >|________V_________|< sX >|
     |      /        |         \      |           |      /        |         \      |
     |     /  current position  \     |           |     /  current position  \     |
     |    /          |           \    |           |    /          |           \    |
     |   /           |->          \   |           |   /         <-|            \   |
     |  /            |             \  |           |  /            |             \  |
     | /             |              \ |           | /             |              \ |
    __!/              |               \!__       __!/              |               \!__
     |<---------- delta ------------->|           |<---------- delta ------------->|
     |<--- moved --->|       targetPosition   targetPosition      |<---  moved --->|
     p0                                                                            p0
  */


  do
  {
    delta = targetPosition - currentPosition;
    if (delta > 0)
    {
      direction = MOVE_OUT;
      p1 = p0 + sX;
      p2 = targetPosition - sX - 5.0f;
    }
    else
    {
      direction = MOVE_IN;
      p1 = p0 - sX;
      p2 = targetPosition + sX + 5.0f;
    }

    currentPosition = getPosition();
    delta = abs(delta);
    moved = abs(p0 - currentPosition);

    if ((currentPosition < p1 && direction == MOVE_OUT) || (currentPosition > p1 && direction == MOVE_IN))
    {
      // accelerate
      smoothThrottle = MINIMUM_THROTTLE + moved * ACCELERATION_RATE;
    }
    else if ((currentPosition > p2 && direction == MOVE_OUT) || (currentPosition < p2 && direction == MOVE_IN))
    {
      // deaccelerate
      smoothThrottle = throttle - (sX - delta) * ACCELERATION_RATE;
    }

    if (smoothThrottle > throttle)
      smoothThrottle = throttle;
    if (smoothThrottle < MINIMUM_THROTTLE)
      smoothThrottle = MINIMUM_THROTTLE;

    Motor1->start(direction, smoothThrottle);

    current = Motor1->getCurrent();
    if (response != 0) {
      response(currentPosition);
    }
    if (specialCaseTargetZero && current <= 0 && currentPosition < 5){
      specialCaseExitLoop = true;
    }
      
  } while ((delta > 0.1f) && current < Motor1->getCurrentLimit() && !specialCaseExitLoop);
  Motor1->stop();
  return;
}

void LinearActuator::moveRelative(float deltaPosition, float throttle)
{
  moveAbsolute(getPosition() + deltaPosition, throttle);
}
