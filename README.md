# PwmMeasureDMA
Teensy 3.2 PWM reading with DMA

This library uses DMA to read a PWM wave and its value. It works on most frequencies from 6Hz to 2MHz(the limits of what I tested). The higher the frequency though the worse resolution it will be able to give. 

It reads on pin 3 on a Teensy 3.2. 

It uses the FTM1 TIMER. Reading different frequencies requires changing the clock divider in the FTM1, so knowing your PWM frequency(a rough estimate of it) is needed. 
