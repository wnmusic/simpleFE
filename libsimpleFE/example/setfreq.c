#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "usb_access.h"
#include "chip_select.h"
#include <math.h>


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
    float freq = 0.0f;
    int freqi = 0;
    const unsigned short n_div = 200;
    const unsigned ref_clk_Hz = 30000000;
    unsigned short a_div;
    
    if (argc > 1){
        freq = atof(argv[1]);
    }else{
        freq = 25e6f;
    }

    a_div = (int)floorf(freq/(ref_clk_Hz/n_div));
    freqi = a_div*(ref_clk_Hz/n_div);
    

    set_pll_div(h, n_div, a_div);
    
    printf("set freqeuncy to: %d Hz, A=%d, N=%d\n", freqi, a_div, n_div);
}



