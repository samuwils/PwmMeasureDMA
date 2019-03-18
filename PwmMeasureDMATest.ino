#include "PwmMeasureDMA.h"

PwmMeasureDMA pwmMeas;
int pwmFreq = 1;
int pwmVal = 1586;
const int pwmOutputPin = 20;

void setup() {
  pwmMeas.begin( pwmFreq, 4096);
  analogWriteResolution(12);
  analogWriteFrequency( pwmOutputPin, pwmFreq);
  analogWrite( pwmOutputPin, pwmVal);
}

void loop() {
  delay(500);
  float pwmF = pwmMeas.readPwmFreq();
  float pwmV = pwmMeas.readPwmValue();
  Serial.print("PWM Frequency ");
  Serial.println(pwmFreq);
  Serial.print("PWM Frequency Read ");
  Serial.println(pwmF);
  Serial.print("PWM Value ");
  Serial.println(pwmVal);
  Serial.print("PWM Value Read ");
  Serial.println(pwmV);
  Serial.println("");
  delay(500);
  analogWriteFrequency(pwmOutputPin , ++pwmFreq);
  pwmMeas.setFrequency(pwmFreq);
  analogWrite( pwmOutputPin, pwmVal);
}
