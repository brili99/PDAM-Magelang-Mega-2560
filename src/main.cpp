#include <Arduino.h>

#define sim Serial1
#define countof(a) (sizeof(a) / sizeof(a[0]))

void initSim800();

String _buffer;
String number = "+628987295940"; //-> destination number

void setup()
{
  Serial.begin(9600);
  initSim800();
}

void loop()
{
  // put your main code here, to run repeatedly:
}

void initSim800()
{
  _buffer.reserve(50);
  sim.begin(9600);
  delay(1000);
}