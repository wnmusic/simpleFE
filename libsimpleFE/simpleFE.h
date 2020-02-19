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

#ifndef SIMPLE_FE_H_
#define SIMPLE_FE_H_

#ifdef __cplusplus
extern "C"{
#endif
    
#define SIMPLE_FE_NUM_SAMPLE_RATES    128

typedef struct sfe_s sfe;
typedef int (sfe_callback)(unsigned char* buffer, int length, void* userdata);
sfe* sfe_init();
void sfe_close(sfe* h);
/* pr should at least hold SIMPLE_FE_NUM_SAMPLE_RATES integers */
void sfe_query_sample_rates(unsigned *pr);

unsigned sfe_get_num_data_per_transfer(sfe *h);
/* these are threaded functions */
int sfe_set_sample_rate(sfe *h, unsigned samplerate);
void sfe_tx_enable(sfe *h, int tx_i, int tx_q);
void sfe_rx_enable(sfe *h, int rx_i, int rx_q);
    
int  sfe_tx_start(sfe *h,
                  sfe_callback* tx_cb,
                  void* cbdata
                  );

int sfe_rx_start(sfe *h,
                 sfe_callback* rx_cb,
                 void* cbdata
                 );
    
void sfe_stop_tx(sfe *h);
void sfe_stop_rx(sfe *h);

unsigned get_real_sample_rate(sfe* h);
void sfe_reset_board(sfe* h);

void sfe_external_gpio_set(sfe* h, int gpio, int val);
int sfe_spi_transfer(sfe *h, unsigned char *data, unsigned len);
void sfe_auxdac_set(sfe* h, int ch, unsigned val);
void sfe_i2c_read(sfe* h, unsigned char addr, char *data, unsigned len);
    void sfe_i2c_write(sfe* h, unsigned char addr,  char *data, unsigned len);



#ifdef __cplusplus
}
#endif

#endif
