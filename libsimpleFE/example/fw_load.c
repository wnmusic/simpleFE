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
#include "ezusb.h"


#ifdef _MSC_VER
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
    sfe_usb *h = usb_init();
    int i;
    unsigned status;

    if (get_cdone(h) != -1){
        fprintf(stderr, "firmware already loaded\n");
        return 1;
    }

    if (argc < 2){
        fprintf(stderr, "usage: fw_load `path to simplefe.hex'\n");
        return 2;
    }

    return ezusb_load_ram(h->dev, argv[1], FX_TYPE_FX2LP, IMG_TYPE_HEX, 0);
    
    usb_close(h);
}
