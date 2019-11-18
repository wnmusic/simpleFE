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


#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <errno.h>
#include <pthread.h>
#include "simpleFE.h"
#include "blkconv.h"
#include "ringbuf.h"
#include "rrc_taps.h"


#define SYMBOL_RATE           (100000)
#define SAMPLES_PER_SYMBOL    (10)
#define SAMPLE_RATE           (SAMPLES_PER_SYMBOL *  SYMBOL_RATE)
#define SCALING_FACTOR        (.85f / 1.35f)
#define RC_TEST

static int exitRequested = 0;

pthread_cond_t buf_cond = PTHREAD_COND_INITIALIZER; 
pthread_mutex_t buf_mutex = PTHREAD_MUTEX_INITIALIZER;

#if (SAMPLES_PER_SYMBOL == 10)
static float *rrc_prototype = &RRC_TAPS_111[0];
static int rrc_filter_len = 111;
static int blk_conv_fft_size = 2048;
#elif (SAMPLES_PER_SYMBOL == 50)
static float *rrc_prototype = &RRC_TAPS_551[0];
static int rrc_filter_len = 551;
static int blk_conv_fft_size = 8192;
#endif

static void sigintHandler(int signum)
{
    exitRequested = 1;
}

static int calc_num_samples(int bytes)
{
    return bytes/5 * 4;
}

static int convert_samples_to_bytes(void* dst, void* src, int src_len)
{

    float *in = static_cast<float*>(src);
    unsigned char *out = static_cast<unsigned char*>(dst);
    int j = 0;
    for (int i=0; i<src_len; i+=4){
        
        float x0 = in[i];
        float x1 = in[i+1];
        float x2 = in[i+2];
        float x3 = in[i+3];

        unsigned short u0 = ((short)(x0*511) + 512) & 0x3FF;
        unsigned short u1 = ((short)(x1*511) + 512) & 0x3FF;
        unsigned short u2 = ((short)(x2*511) + 512) & 0x3FF;
        unsigned short u3 = ((short)(x3*511) + 512) & 0x3FF;
        
        out[j++] = (u0 >>8) | ((u1>>8)<<2) | ((u2>>8)<<4) | ((u3>>8)<<6);
        out[j++] = u0 & 0xFF;
        out[j++] = u1 & 0xFF;
        out[j++] = u2 & 0xFF;
        out[j++] = u3 & 0xFF;
    }
    return j;
}


static int tx_callback(unsigned char* buffer, int length, void* userdata)
{
    ring_buffer<float> *buf = (ring_buffer<float> *)userdata;

    pthread_mutex_lock(&buf_mutex);
    if (buf->get_count() < calc_num_samples(length)){
        fprintf(stderr, "U");
    }
    else{
        buf->read(buffer, length, convert_samples_to_bytes, calc_num_samples);
        pthread_cond_signal(&buf_cond);
    }
    pthread_mutex_unlock(&buf_mutex);
            
    return exitRequested;
}


void* process(void* data)
{
    ring_buffer<float> *buf = (ring_buffer<float> *)data;
    blkconv pulse_filter(rrc_prototype, rrc_filter_len, blk_conv_fft_size);
    int blk_size = pulse_filter.get_blksize();
    float *proc_buf = pulse_filter.get_process_buf();
    int n_input = 0, n_phase = 0;

    while (!exitRequested)
    {
        pthread_mutex_lock(&buf_mutex);
        if (buf->get_space() >= blk_size){
            n_input = 0;
            // prepare the input buffer
            if (n_phase > 0)
            {
                for (int i=n_phase; i<SAMPLES_PER_SYMBOL; i++)
                {
                    proc_buf[n_input++] = 0.0f;
                }
                n_phase = 0;
            }
                        
            while(n_input < blk_size)
            {
                int word = rand();
                // take the first 32 bits,
                for (int j=0; j<32 && n_input < blk_size; j++)
                {
                    float bit = (word & (1<<j)) ? -SCALING_FACTOR : SCALING_FACTOR;
                    proc_buf[n_input++] = bit;

                    for (n_phase=1; n_phase<SAMPLES_PER_SYMBOL && n_input<blk_size; n_phase++)
                    {
                        proc_buf[n_input++] = 0.0f;
                    }
                }
            }

            //pulse shaping 
            pulse_filter.process();
            //write to buffer
            buf->write(proc_buf, blk_size);
        }
        else{
            
            pthread_cond_wait(&buf_cond, &buf_mutex);
        }
        pthread_mutex_unlock(&buf_mutex);
    }

    return NULL;
}

int main(int argc, char* argv[])
{
    sfe* h = sfe_init();
    unsigned sample_rate = SAMPLE_RATE;
    ring_buffer<float> dev_buf(SAMPLE_RATE);
    pthread_t proc_thread;
    void* ret = NULL;
    
    sfe_reset_board(h);

    if (sfe_set_sample_rate(h, sample_rate) ){
        fprintf(stderr, "set sample rate\n");
        exit(1);
    }

    sample_rate = get_real_sample_rate(h);
    fprintf(stderr, "Real sample rate: %d, Real symbol rate: %.2f",
           sample_rate, sample_rate*1.0/SAMPLES_PER_SYMBOL
           );

    //enable TX_I
    sfe_tx_enable(h, 1, 0);
    //setup signal hanlder
    signal(SIGINT, sigintHandler);


    //start processing thread
    pthread_create(&proc_thread, NULL, process, &dev_buf);

    // start tx
    sfe_tx_start(h, tx_callback, &dev_buf);

    //wait until finish
    pthread_join(proc_thread, &ret);
    
    sfe_stop_tx(h);
    signal(SIGINT, SIG_DFL);
    sfe_close(h);
    
}
