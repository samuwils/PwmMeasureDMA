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

#include "PwmMeasureDMA.h"

PwmMeasureDMA::PwmMeasureDMA()
{
    buffer_byte_cnt = sizeof(buffer[0]) * buffer.size();
}



bool PwmMeasureDMA::begin(uint32_t frequency, uint16_t resolution)
{
    pwmResolution = resolution;
    pwmFrequency = frequency;
    
    int clockDivBit = getClockDiv(pwmFrequency );
   
    // setup FTM1 in free running mode, counting from 0 - 0xFFFF
    FTM1_SC = 0;
    FTM1_CNT = 0;
    FTM1_MOD = 0xFFFF;
    
    // pin 3 config to FTM mode; pin 3 is connected to FTM1 channel 0
    CORE_PIN3_CONFIG = PORT_PCR_MUX(3);
    
    // set FTM1 clock source to system clock; 
    switch(clockDivBit)
    {
        case 0 : FTM1_SC = (FTM_SC_CLKS(1) | FTM_SC_PS(0)); clockDiv = 1; break;
        case 1 : FTM1_SC = (FTM_SC_CLKS(1) | FTM_SC_PS(1)); clockDiv = 2; break;
        case 2 : FTM1_SC = (FTM_SC_CLKS(1) | FTM_SC_PS(2)); clockDiv = 4; break;
        case 3 : FTM1_SC = (FTM_SC_CLKS(1) | FTM_SC_PS(3)); clockDiv = 8; break;
        case 4 : FTM1_SC = (FTM_SC_CLKS(1) | FTM_SC_PS(4)); clockDiv = 16; break;
        case 5 : FTM1_SC = (FTM_SC_CLKS(1) | FTM_SC_PS(5)); clockDiv = 32; break;
        case 6 : FTM1_SC = (FTM_SC_CLKS(1) | FTM_SC_PS(6)); clockDiv = 64; break;
        case 7 : FTM1_SC = (FTM_SC_CLKS(1) | FTM_SC_PS(7)); clockDiv = 128; break;
    }    

    // need to unprotect FTM1_COMBINE register
    FTM1_MODE = FTM_MODE_WPDIS;
    // set FTM1 CH0 to dual edge capture, paired channels
    FTM1_COMBINE = FTM_COMBINE_DECAP0 | FTM_COMBINE_DECAPEN0;
    FTM1_MODE = FTM_MODE_WPDIS | FTM_MODE_FTMEN;
    
    // We read 2 bytes from FTM1_C0V and 2 bytes from FTM1_C1V
    dma.TCD->SADDR = &FTM1_C0V;
    dma.TCD->ATTR_SRC = 1; // 16-bit read from source (timer value is 16 bits)
    dma.TCD->SOFF = 8;     // increment source address by 8 (switch from reading FTM1_C0V to FTM1_C1V)
    // transfer 4 bytes total per minor loop (2 from FTM1_C0V + 2 from FTM1_C1V); go back to
    // FTM1_C0V after minor loop (-16 bytes).
    dma.TCD->NBYTES_MLOFFYES = DMA_TCD_NBYTES_SMLOE | DMA_TCD_NBYTES_MLOFFYES_NBYTES(4) |
                               DMA_TCD_NBYTES_MLOFFYES_MLOFF(-16);
    // source addr adjustment at major loop end (the minor loop adjustment doesn't get executed)
    dma.TCD->SLAST = -16;
    dma.TCD->DADDR = buffer.data();
    dma.TCD->DOFF = 2;     // 2 bytes destination increment
    dma.TCD->ATTR_DST = 1; // 16-bit write to dest
    // set major loop count
    dma.TCD->BITER = buffer_capture_cnt;
    dma.TCD->CITER = buffer_capture_cnt;
    // use buffer as ring buffer, go back 'buffer_byte_cnt' after buffer_capture_cnt captures
    // have been done
    dma.TCD->DLASTSGA = -buffer_byte_cnt;
    // disable channel linking, keep DMA running continously
    dma.TCD->CSR = 0;

    // trigger a DMA transfer whenever a new pulse has been captured
    dma.triggerAtHardwareEvent(DMAMUX_SOURCE_FTM1_CH1);
    dma.enable();
    
        // channel 0, capture rising edge; FTM_CSC_MSA --> continous capture mode
    FTM1_C0SC = FTM_CSC_ELSA | FTM_CSC_MSA;
    // channel 1, capture falling edge and trigger DMA, FTM_CSC_MSA --> continous capture mode
    FTM1_C1SC = FTM_CSC_CHIE | FTM_CSC_DMA| FTM_CSC_ELSB | FTM_CSC_MSA;
    return true;
}

size_t PwmMeasureDMA::getCurrentDmaIndex() {
    auto dest_addr = static_cast<decltype(buffer.data())>(dma.destinationAddress());
    return size_t(dest_addr - buffer.data());
}

int PwmMeasureDMA::getClockDiv(float freq)
{
    int clockDivB = 0;
    float tot = 65536;
    if(freq > (1 / (tot / (float)F_BUS)))
    {
        clockDivB = 0;
    }
    else if(freq > (1 / ((tot * 2) / (float)F_BUS)) && freq < (1 / (tot / (float)F_BUS)))
    {
        clockDivB = 1;
    }
    else if(freq > (1 / ((tot * 4) / (float)F_BUS)) && freq < (1 / ((tot * 2) / (float)F_BUS)))
    {
        clockDivB = 2;
    }
    else if(freq > (1 / ((tot * 8) / (float)F_BUS)) && freq < (1 / ((tot * 4) / (float)F_BUS)))
    {
        clockDivB = 3;
    }
    else if(freq > (1 / ((tot * 16) / (float)F_BUS)) && freq < (1 / ((tot * 8) / (float)F_BUS)))
    {
        clockDivB = 4;
    }    
    else if(freq > (1 / ((tot * 32) / (float)F_BUS)) && freq < (1 / ((tot * 16) / (float)F_BUS)))
    {
        clockDivB = 5;
    } 
    else if(freq > (1 / ((tot * 64) / (float)F_BUS)) && freq < (1 / ((tot * 32) / (float)F_BUS)))
    {
        clockDivB = 6;
    }     
    else if(freq > (1 / ((tot * 128) / (float)F_BUS)) && freq < (1 / ((tot * 64) / (float)F_BUS)))
    {
        clockDivB = 7;
    } 
    else
    {
       // Serial.println("Too slow");
    }
    
    return clockDivB;
}

void PwmMeasureDMA::setFrequency(uint32_t frequency)
{
    pwmFrequency = frequency;
    
    int clockDivBit = getClockDiv(pwmFrequency );
    
    switch(clockDivBit)
    {
        case 0 : FTM1_SC = (FTM_SC_CLKS(1) | FTM_SC_PS(0)); clockDiv = 1; break;
        case 1 : FTM1_SC = (FTM_SC_CLKS(1) | FTM_SC_PS(1)); clockDiv = 2; break;
        case 2 : FTM1_SC = (FTM_SC_CLKS(1) | FTM_SC_PS(2)); clockDiv = 4; break;
        case 3 : FTM1_SC = (FTM_SC_CLKS(1) | FTM_SC_PS(3)); clockDiv = 8; break;
        case 4 : FTM1_SC = (FTM_SC_CLKS(1) | FTM_SC_PS(4)); clockDiv = 16; break;
        case 5 : FTM1_SC = (FTM_SC_CLKS(1) | FTM_SC_PS(5)); clockDiv = 32; break;
        case 6 : FTM1_SC = (FTM_SC_CLKS(1) | FTM_SC_PS(6)); clockDiv = 64; break;
        case 7 : FTM1_SC = (FTM_SC_CLKS(1) | FTM_SC_PS(7)); clockDiv = 128; break;
    } 
    
    return;
}

float PwmMeasureDMA::readPwmFreq()
{
    RisingFallingPair p1, p2;
    size_t buffer_index = getCurrentDmaIndex();
    p1 = buffer[(buffer_index - 2) % buffer.size()];
    p2 = buffer[(buffer_index - 1) % buffer.size()];
    
    uint16_t period = (p2.timer_at_rising - p1.timer_at_rising);
   
    float freq = 1 / ((float)period / (float)(F_BUS / clockDiv));
    /*Serial.printf("Pulse 1: %u %u\n", p1.timer_at_rising, p1.timer_at_falling);
    Serial.printf("Pulse 2: %u %u\n", p2.timer_at_rising, p2.timer_at_falling);
    Serial.printf("Pulse length: %u\n", (uint16_t)(p2.timer_at_rising- p1.timer_at_rising));
    */
    
    return freq;
}

float PwmMeasureDMA::readPwmValue()
{
    RisingFallingPair p1, p2;
    size_t buffer_index = getCurrentDmaIndex();
    p1 = buffer[(buffer_index - 2) % buffer.size()];
    p2 = buffer[(buffer_index - 1) % buffer.size()];
    
    uint16_t width = p1.timer_at_falling - p1.timer_at_rising;
    uint16_t period = (p2.timer_at_rising - p1.timer_at_rising);
    
    float rate =  (float)pwmResolution * ((float)width / (float)period );
    /*Serial.printf("Pulse 1: %u %u\n", p1.timer_at_rising, p1.timer_at_falling);
    Serial.printf("Pulse 2: %u %u\n", p2.timer_at_rising, p2.timer_at_falling);*/
    
    return rate;
}
