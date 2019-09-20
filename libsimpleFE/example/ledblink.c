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
#include "usb_access.h"
#include "chip_select.h"


#ifdef _MSC_VER
void usleep(DWORD waitTime) {
	LARGE_INTEGER perfCnt, start, now;

	QueryPerformanceFrequency(&perfCnt);
	QueryPerformanceCounter(&start);

	do {
		QueryPerformanceCounter((LARGE_INTEGER*)&now);
	} while ((now.QuadPart - start.QuadPart) / (float)(perfCnt.QuadPart) * 1000 * 1000 < waitTime);
}
#endif


int main(int argc, char* argv[])
{
    sfe_usb *h = usb_init();
    int i;
    unsigned status;
    unsigned char dac[2];
    for(i=0; i<20; i++){
        set_gpio(h, GPIO_LED, i&0x01);
        usleep(50*1000);
    }

    printf("clkrate on board: %d\n", get_clkrate(h));

    {
        unsigned cdiv;
        int tx_i, tx_q, rx_i, rx_q, sys_en;
        get_fpga_status(h, &cdiv, &tx_i, &tx_q, &rx_i, &rx_q, &sys_en);
        printf("sys_en: %d, tx_i: %d, tx_q: %d, rx_i: %d, rx_q: %d\n", sys_en, tx_i, tx_q, rx_i, rx_q);
        printf("cdiv: %d\n", cdiv);
    }
    {
        unsigned adc, dac;
        
        get_fifo_status(h, &adc, &dac);

        printf("adc: 0x%x\n", adc);
        printf("dac: 0x%x\n", dac);
    }

    printf("set AUXDAC\n");
    dac[0] = (0x00<<6) | (0x01<<4) | 0;
    dac[1] = 0;
    set_gpio(h, AUXDAC_CS, 0);
    usb_xfer_spi(h, &dac[0], 2);
    set_gpio(h, AUXDAC_CS, 1);

    dac[0] = (0x01<<6) | (0x01<<4) | 0x04;
    dac[1] = 0;
    set_gpio(h, AUXDAC_CS, 0);
    usb_xfer_spi(h, &dac[0], 2);
    set_gpio(h, AUXDAC_CS, 1);

    dac[0] = (0x02<<6) | (0x01<<4) | 0x08;
    dac[1] = 0;
    set_gpio(h, AUXDAC_CS, 0);
    usb_xfer_spi(h, &dac[0], 2);
    set_gpio(h, AUXDAC_CS, 1);

    dac[0] = (0x03<<6) | (0x01<<4) | 0x0F;
    dac[1] = 0xF0;
    set_gpio(h, AUXDAC_CS, 0);
    usb_xfer_spi(h, &dac[0], 2);
    set_gpio(h, AUXDAC_CS, 1);
    
    
    usb_close(h);
}
