/*BSD 2-Clause License

Copyright (c) 2016, Tilo Nitzsche
Copyright (c) 2019, Sam Wilson
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

* Redistributions of source code must retain the above copyright notice, this
  list of conditions and the following disclaimer.

* Redistributions in binary form must reproduce the above copyright notice,
  this list of conditions and the following disclaimer in the documentation
  and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <DMAChannel.h>
#include <array>
#include <Arduino.h>

struct RisingFallingPair {
    volatile uint16_t timer_at_rising;
    volatile uint16_t timer_at_falling;
};

class PwmMeasureDMA
{
    public:
        PwmMeasureDMA(void);
        bool begin(uint32_t frequency, uint16_t resolution); 
        float readPwmFreq();
        float readPwmValue();
        void setFrequency(uint32_t frequency);
    private:
        DMAChannel dma;
        void isr(void);
        const size_t buffer_capture_cnt = 16;
        std::array<RisingFallingPair, 16> buffer;
        size_t buffer_byte_cnt;
        uint16_t pwmResolution;
        uint32_t pwmFrequency;
        uint16_t clockDiv;
        uint32_t prev;
        bool available_flag;
        size_t getCurrentDmaIndex();
        int getClockDiv(float frequency);
           
};