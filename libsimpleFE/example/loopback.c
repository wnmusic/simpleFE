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
#include <signal.h>
#include <errno.h>
#include "simpleFE.h"



static int exitRequested = 0;

static unsigned char waveform[1280];
static unsigned nSamples = 0;

static long long rx_bytes = 0;
static unsigned  rx_pkts = 0;
static FILE *fp = 0;
static unsigned recpos = 0;
static unsigned char recwav[1024*2*80];

static void
sigintHandler(int signum)
{
    printf ("sig int entered, rx_pkts: %d", rx_pkts);
    fwrite(recwav, 1024*2*80, 1, fp);
    fclose(fp);
    
    exitRequested = 1;
}

static int tx_callback(unsigned char* buffer, int length, void* userdata)
{
    for (int i=0; i<length; i++){
        buffer[i] = waveform[nSamples];
        nSamples = (nSamples + 1 ) % 1280;
    }
            
    return exitRequested;
}

static int rx_callback(unsigned char* buffer, int length, void* userdata)
{
    static unsigned rx_cnt = 0;
    rx_bytes += length;
    rx_pkts ++ ;
        
    if(rx_pkts <= 80 ){
        memcpy(recwav + recpos, buffer, length);
        recpos += length;
    }
	else {
		recpos = 0;
	}
    
    if (rx_pkts % 8000 == 0){
        rx_cnt++;
        printf("adc throughput: %d\n", rx_bytes*8000/rx_pkts);
        if (rx_cnt == 5){
            rx_bytes = 0;
            rx_pkts = 0;
            rx_cnt = 0;
        }
    }
    return exitRequested;
}


int main(int argc, char* argv[])
{
    sfe *h_tx = sfe_init();
    
    unsigned sample_rate =5000000;
    if(!h_tx){
        fprintf(stderr, "Cannot open tx simpleFE device\n");
        return 1;
    }

    //fill waveform buffer
    for(int i=0, j=0; i<1280; i+=5){
        unsigned data0 = j++;
        unsigned data1 = j++;
        unsigned data2 = j++;
        unsigned data3 = j++;
        
        waveform[i] = (data0>>8)| ((data1>>8)<<2) | ((data2>>8)<<4) | ((data3>>8)<<6);
        waveform[i+1] = data0 & 0xFF;
        waveform[i+2] = data1 & 0xFF;
        waveform[i+3] = data2 & 0xFF;
        waveform[i+4] = data3 & 0xFF;
    }

    sfe_reset_board(h_tx);


    if (sfe_set_sample_rate(h_tx, sample_rate) ){
        fprintf(stderr, "set sample rate\n");
        exit(1);
    }

    sfe_tx_enable(h_tx, 0, 1);
    sfe_rx_enable(h_tx, 1, 0);

    fp = fopen("rec.dat", "wb");
    signal(SIGINT, sigintHandler);

    if (sfe_rx_start(h_tx, rx_callback, NULL)){        
        printf("rx start failed\n");
        exit(1);
    }
    
    if(sfe_tx_start(h_tx, tx_callback, NULL)){
        printf("tx start failed\n");
        exit(1);
    }
    
    while (!exitRequested){
#ifdef _MSC_VER
        _sleep(1000);
#else
	sleep(1);
#endif
    }

    sfe_stop_tx(h_tx);
    sfe_stop_rx(h_tx);

    signal(SIGINT, SIG_DFL);

    sfe_close(h_tx);
}
    
    
