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
    uint8_t data0[3] = {0x01, 0x38, 0x12};
    uint8_t data1[3] = {0x0f, 0xf1, 0x20};
    uint8_t data2[3] = {0x30, 0x00, 0xc9};

    //strobe cs linex
    sfe_external_gpio_set(h, 0, 0);
    sfe_spi_transfer(h, data2, 3);
    sfe_external_gpio_set(h, 0, 1);

    sfe_external_gpio_set(h, 0, 0);
    sfe_spi_transfer(h, data1, 3);
    sfe_external_gpio_set(h, 0, 1);

	usleep(10000);

    sfe_external_gpio_set(h, 0, 0);
    sfe_spi_transfer(h, data0, 3);
    sfe_external_gpio_set(h, 0, 1);
    
    sfe_close(h);
}
