/*

Copyright (c) 2019, Ning Wang <nwang.cooper@gmail.com> All rights reserved.


Redistribution and use in source and binary forms, with or without modification, 
are permitted provided that the following conditions are met:
    
     Redistributions of source code must retain the above copyright notice, 
     this list of conditions and the following disclaimer.

     Redistributions in binary form must reproduce the above copyright notice, 
     this list of conditions and the following disclaimer in the 
     documentation and/or other materials provided with the distribution.
 
     Neither the name of its contributors can be used to endorse or promote 
     products derived from this software witthout specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, 
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR 
PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS 
BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, 
OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, 
STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED 
OF THE POSSIBILITY OF SUCH DAMAGE.
*/


#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "simpleFE.h"

#ifdef _MSC_VER
#include <windows.h>
void usleep(DWORD waitTime) {
	LARGE_INTEGER perfCnt, start, now;

	QueryPerformanceFrequency(&perfCnt);
	QueryPerformanceCounter(&start);

	do {
		QueryPerformanceCounter((LARGE_INTEGER*)&now);
	} while ((now.QuadPart - start.QuadPart) / (float)(perfCnt.QuadPart) * 1000 * 1000 < waitTime);
}

#else
#include <unistd.h>
#endif
int main(int argc, char* argv[])
{
    sfe *h = sfe_init();
    uint8_t data0[3] = {0x00, 0xFA, 0x02}; //N 
    uint8_t data1[3] = {0x0f, 0xf9, 0x20}; //C 
    uint8_t data2[3] = {0x30, 0x00, 0xc9}; //R

    uint16_t A = 2;
    uint16_t B = 200;
    uint16_t R = 40;

    uint32_t Nval = ((B & 0x1FFF)<<8) | ((A & 0x1F) << 2) | 0x02;
    data0[0] = (Nval & 0x00FF0000)>>16;
    data0[1] = (Nval & 0x0000FF00)>>8;
    data0[2] = (Nval & 0x000000FF);
    printf("N: 0x%02x, 0x%02x, 0x%02x\n", data0[0], data0[1], data0[2]);

    uint32_t Rval = (0x03 << 20) | ((R & 0x3FFF) << 2) | 0x01;
    data2[0] = (Rval & 0x00FF0000)>>16;
    data2[1] = (Rval & 0x0000FF00)>>8;
    data2[2] = (Rval & 0x000000FF);
    printf("R: 0x%02x, 0x%02x, 0x%02x\n", data2[0], data2[1], data2[2]);

    uint32_t Cval = 0x0ff920;
    Cval = (Cval & ~(0x03<<12)) | (0x01 <<12); //output power settings
    Cval = (Cval & ~(0x3F<<14)) | (0x1B <<14); //1.25mA CP current
    data1[0] = (Cval & 0x00FF0000)>>16;
    data1[1] = (Cval & 0x0000FF00)>>8;
    data1[2] = (Cval & 0x000000FF);
    printf("C: 0x%02x, 0x%02x, 0x%02x\n", data1[0], data1[1], data1[2]);

    //strobe cs linex
    sfe_external_gpio_set(h, 0, 0);
    sfe_spi_transfer(h, data2, 3);
    sfe_external_gpio_set(h, 0, 1);

    sfe_external_gpio_set(h, 0, 0);
    sfe_spi_transfer(h, data1, 3);
    sfe_external_gpio_set(h, 0, 1);

    usleep(20000);

    sfe_external_gpio_set(h, 0, 0);
    sfe_spi_transfer(h, data0, 3);
    sfe_external_gpio_set(h, 0, 1);
    
    sfe_close(h);
}
