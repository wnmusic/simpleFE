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

#include "simpleFE.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include "usb_access.h"
#include "simpleFE.h"
#include "chip_select.h"
#include "ezusb.h"
#include <pthread.h>
#include "libusb.h"

#define FPGA_CLK   30000000
#ifdef __linux__
#define NUM_PKTS_PER_XFER        120
#define NUM_TRANSFERS            32
#else
#define NUM_PKTS_PER_XFER        240
#define NUM_TRANSFERS            16
#endif


#ifndef _MSC_VER
#include <unistd.h>
#endif


static const unsigned num_pkts_per_sec = 8000; /* 8 packets per 1ms */
static const unsigned num_pkts_per_125ms = 1000;

struct sfe_s{

    sfe_usb* usb;
    unsigned packets_per_xfer;
    unsigned num_xfers;
    struct libusb_transfer **pp_xfers;

    /* control status */
    int clk_div;
    unsigned sample_rate;
    unsigned actual_rate;

    int xfer_ctrl; // tx_i, tx_q, rx_i, rx_q
    
    int num_rx_channels;
    int num_tx_channels;
    
    /* this is DAC rate control  */
    int tx_bytes_adj;
    int tx_bytes_frame_remain;
    unsigned tx_bytes_per_sec;
    unsigned dac_check_pkts;
    unsigned prev_dac_level;
    unsigned rc_hold_count;
    
    /* transfer stats */
    unsigned tx_pkts;
    unsigned rx_pkts;
    
    int rx_data_valid;

    /* callback functions */
    pthread_t tx_thread;
    pthread_t rx_thread;
    
    sfe_callback *tx_callback;
    void *tx_ctx;
    int tx_exit_request;
    
    sfe_callback *rx_callback;
    void *rx_ctx;
    int rx_exit_request;
    

    int status;    
};


static void set_fpga_cdiv(sfe* h, int div)
{
    unsigned char cfg[2];
    assert(div < 128 && div >= 0);
    cfg[0] = (1<<7) | (0x01 << 5); //reg 1
    cfg[1] = div & 0x7F;
    set_gpio(h->usb, FPGA_CS, 0);
    usb_xfer_spi(h->usb, &cfg[0], 2);
    set_gpio(h->usb, FPGA_CS, 1);
}

static void start_fpga(sfe *h)
{
    unsigned char cfg[2];
    int tx_i, tx_q, rx_i, rx_q;
    get_fpga_status(h->usb, NULL, &tx_i, &tx_q, &rx_i, &rx_q, NULL);
    
    cfg[0] = (1<<7); //reg 0
    cfg[1] = (tx_q << 4) | (tx_i << 3) | (rx_q << 2) | (rx_i << 1) | 0x01;
    set_gpio(h->usb, FPGA_CS, 0);
    usb_xfer_spi(h->usb, &cfg[0], 2);
    set_gpio(h->usb, FPGA_CS, 1);
}

static void stop_fpga(sfe *h)
{
    unsigned char cfg[2];    
    cfg[0] = (1<<7); //reg 0
    cfg[1] = 0;
    set_gpio(h->usb, FPGA_CS, 0);
    usb_xfer_spi(h->usb, &cfg[0], 2);
    set_gpio(h->usb, FPGA_CS, 1);
}

void sfe_tx_enable(sfe *h, int tx_i, int tx_q)
{
    int rx_i, rx_q;
    int sys_en;
    unsigned char cfg[2];

    get_fpga_status(h->usb, NULL, NULL, NULL, &rx_i, &rx_q, &sys_en);


    h->xfer_ctrl |= (tx_q << 4) | (tx_i << 3);
    h->num_tx_channels = tx_i + tx_q;

    
    cfg[0] = (1<<7); //reg 0
    cfg[1] = (tx_q << 4) | (tx_i << 3);
    set_gpio(h->usb, FPGA_CS, 0);
    usb_xfer_spi(h->usb, &cfg[0], 2);
    set_gpio(h->usb, FPGA_CS, 1);

#ifdef _MSC_VER
    _sleep(1);
#else            
    usleep(1000);
#endif            

    /* enable fpga */
    sys_en |= (tx_i | tx_q);
    cfg[0] = (1<<7); //reg 0
    cfg[1] = (tx_q << 4) | (tx_i << 3) | (rx_q << 2) | (rx_i << 1) | sys_en;
    set_gpio(h->usb, FPGA_CS, 0);
    usb_xfer_spi(h->usb, &cfg[0], 2);
    set_gpio(h->usb, FPGA_CS, 1);
    
}


void sfe_rx_enable(sfe *h, int rx_i, int rx_q)
{
    int tx_i, tx_q;
    unsigned char cfg[2];
    int sys_en;
    
    get_fpga_status(h->usb, NULL, &tx_i, &tx_q,NULL, NULL, &sys_en);

    h->xfer_ctrl |= (rx_q << 2) | (rx_i << 1);
    h->num_rx_channels = rx_i + rx_q;
    
    sys_en |= (rx_i | rx_q);

    //reset rx path, 
    cfg[0] = (1<<7); //reg 0
    cfg[1] = (tx_q << 4) | (tx_i << 3) | sys_en;
    set_gpio(h->usb, FPGA_CS, 0);
    usb_xfer_spi(h->usb, &cfg[0], 2);
    set_gpio(h->usb, FPGA_CS, 1);
#ifdef _MSC_VER
    _sleep(1);
#else            
    usleep(1000);
#endif            
    //enable rx path
    cfg[0] = (1<<7); //reg 0
    cfg[1] = (tx_q << 4) | (tx_i << 3) | (rx_q << 2) | (rx_i << 1) | sys_en;
    set_gpio(h->usb, FPGA_CS, 0);
    usb_xfer_spi(h->usb, &cfg[0], 2);
    set_gpio(h->usb, FPGA_CS, 1);
    
}

static 
int ensure_stable_clock_reading(sfe *h, unsigned *pclk)
{
    unsigned m_clock;
    /* rate control */
    for (int i=0; i<4; i++){
        
        m_clock = get_clkrate(h->usb);
        if (m_clock > FPGA_CLK + 1000000 || m_clock  < FPGA_CLK - 1000000){
            fprintf(stderr, "Board is not ready(clk=%d), retry in 1 second\n", m_clock);
#ifdef _MSC_VER
            _sleep(1000);
#else            
            sleep(1);
#endif            
        }
        else {
            break;
        }
    }

    if (m_clock > FPGA_CLK + 1000000 || m_clock  < FPGA_CLK - 1000000){
        fprintf(stderr, "Cannot get a correct clock rate, check hardware\n");
        return -1;
    }

    if (pclk) {
        *pclk = m_clock;
    }

    return 0;
}


static void tx_rate_control(sfe* h, unsigned char dac_level)
{
    /* 1k per/sec prime, 125ms cycle of reading fifo */
    if (h->rc_hold_count++ < 2*num_pkts_per_sec/num_pkts_per_125ms){
        h->tx_bytes_adj = 0;
    }
    else{

        if (dac_level > 0x30){
            h->rc_hold_count = 0;
            h->tx_bytes_adj = -1024;
        }else if (dac_level < 0x10){
            h->rc_hold_count = 0;
            h->tx_bytes_adj = 1024;
        }else{
            h->tx_bytes_adj = 0;
        }
    }
}


static void LIBUSB_CALL
usb_get_level_callback(struct libusb_transfer *transfer)
{
    sfe *h = transfer->user_data;
    if (transfer->status == LIBUSB_TRANSFER_COMPLETED){

        unsigned char *data = &transfer->buffer[LIBUSB_CONTROL_SETUP_SIZE];
        unsigned adc_level = data[0]& 0x3F;
        unsigned dac_level = data[1]& 0x3F;
        tx_rate_control(h, dac_level);
        //printf("dac: 0x%02x, adc: 0x%02x\n",dac_level, adc_level);
    }
    else{
        fprintf(stderr, "control transfer status: %d\n", transfer->status);        
        h->status = transfer->status;
    }

    free(transfer->buffer);
    libusb_free_transfer(transfer);
}


static void LIBUSB_CALL
usb_get_clock_callback(struct libusb_transfer *transfer)
{
    sfe *h = transfer->user_data;
    if (transfer->status == LIBUSB_TRANSFER_COMPLETED){

            unsigned char *data = &transfer->buffer[LIBUSB_CONTROL_SETUP_SIZE];
            unsigned rate = (data[0] << 24) | (data[1]<<16) | (data[2]<<8) | data[3];

            rate = rate /(h->clk_div*2 + 4);
            h->sample_rate = rate;
            h->tx_bytes_per_sec = rate * h->num_tx_channels * 10 / 8;
            //printf("onboard rate: %d\n",rate);
    }
    else{
        fprintf(stderr, "control transfer status: %d\n", transfer->status);        
        h->status = transfer->status;
    }

    free(transfer->buffer);
    libusb_free_transfer(transfer);
}

static void
get_usb_fifolevel(sfe* h)
{
    
    void* setup = malloc(LIBUSB_CONTROL_SETUP_SIZE + MAX_VR_BYTES);
    struct libusb_transfer* transfer = libusb_alloc_transfer(0);
    if (!transfer){
        fprintf(stderr, "cannot allocate transfer structure\n");
        return;
    }
    
    libusb_fill_control_setup(setup,
                              LIBUSB_RECIPIENT_DEVICE  | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_ENDPOINT_IN,
                              VR_RATE,
                              0x0000, 0, VR_RATE_STATUS_BYTES);

    transfer->buffer = setup;
    libusb_fill_control_transfer(transfer,
                                 h->usb->dev,
                                 setup,
                                 usb_get_level_callback,
                                 h,
                                 5000
                                 );

                                
    h->status = libusb_submit_transfer(transfer);                              
}

static void
get_board_clockrate(sfe* h)
{
    
    void* setup = malloc(LIBUSB_CONTROL_SETUP_SIZE + MAX_VR_BYTES);
    struct libusb_transfer* transfer = libusb_alloc_transfer(0);
    if (!transfer){
        fprintf(stderr, "cannot allocate transfer structure\n");
        return;
    }
    
    libusb_fill_control_setup(setup,
                              LIBUSB_RECIPIENT_DEVICE  | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_ENDPOINT_IN,
                              VR_RATE,
                              0x0001, 0, VR_RATE_CLOCK_BYTES);

    transfer->buffer = setup;    
    libusb_fill_control_transfer(transfer,
                                 h->usb->dev,
                                 setup,
                                 usb_get_clock_callback,
                                 h,
                                 5000
                                 );
                                
    h->status = libusb_submit_transfer(transfer);                              
}


/* this will return number of bytes need to be transfered 
 * and will a number of multiple of 5 */
static unsigned
set_tx_packet_info(sfe* h, struct libusb_transfer *xfer, int pkt_adj)
{

    unsigned total  = (long long )h->tx_bytes_per_sec * (long long)xfer->num_iso_packets/ num_pkts_per_sec;
    unsigned i, len0, rem;
    const unsigned frame_size = 5;

    total += pkt_adj + h->tx_bytes_frame_remain;
    /* make sure total bytes is multiple of 5 */
    h->tx_bytes_frame_remain = total - (total/frame_size)*frame_size;
    total = total - h->tx_bytes_frame_remain;
    
    len0 = total / xfer->num_iso_packets;
    rem = total - len0 * (xfer->num_iso_packets-1);

    for (i=xfer->num_iso_packets-1; i>0; i--){
        xfer->iso_packet_desc[i].length = len0;
    }
    xfer->iso_packet_desc[0].length = rem;
    xfer->length = total;
    
    return total;
}
    
static void LIBUSB_CALL
usb_in_callback(struct libusb_transfer *transfer)
{
    sfe* h = transfer->user_data;
    int ret = 0;
    
    if (transfer->status == LIBUSB_TRANSFER_COMPLETED){
        unsigned i;
        for (i=0; i<transfer->num_iso_packets; i++){
            struct libusb_iso_packet_descriptor *desc = &transfer->iso_packet_desc[i];

            if (desc->status == LIBUSB_TRANSFER_COMPLETED){
                unsigned int len = desc->actual_length;
                unsigned char *pkt = libusb_get_iso_packet_buffer_simple(transfer, i);
                h->rx_pkts++;
                /* ignore the first packet as it may be rabbish*/
                if (h->rx_data_valid && h->rx_callback){
                    ret = h->rx_callback(pkt, len, h->rx_ctx);
                }
                if (h->rx_pkts > 2 && !h->rx_data_valid) {
                    h->rx_data_valid = 1;
                }
		
            }else{
                fprintf(stderr, "rx desc status: %s\n", libusb_error_name(desc->status));
                h->status = desc->status;
                h->rx_exit_request = 1;
                break;
            }
        }
    }
    else{
        //fprintf(stderr, "rx transfer status: %d\n", transfer->status);
        h->status = transfer->status;
    }
    
    if (ret || h->rx_exit_request){
        /* user indicate exit */
        free(transfer->buffer);
        libusb_free_transfer(transfer);
    }
    else{
        transfer->status = -1;
        h->status = libusb_submit_transfer(transfer);
    }
}


static void LIBUSB_CALL
usb_out_callback(struct libusb_transfer *transfer)
{
    sfe* h = transfer->user_data;
    int ret = 0;
    unsigned tx_size;
            
    if (transfer->status == LIBUSB_TRANSFER_COMPLETED){
        unsigned i;
        for (i=0; i<transfer->num_iso_packets; i++){
            struct libusb_iso_packet_descriptor *desc = &transfer->iso_packet_desc[i];

            if (desc->status == LIBUSB_TRANSFER_COMPLETED){
                h->tx_pkts++;
                h->dac_check_pkts++;
            }else{
                fprintf(stderr, "tx desc status: %d\n", desc->status);
                h->status = desc->status;
                h->tx_exit_request = 1;
            }
        }
        
        tx_size = set_tx_packet_info(h, transfer, h->tx_bytes_adj);
        h->tx_bytes_adj = 0;
        if (h->tx_callback){
            ret = h->tx_callback(transfer->buffer, tx_size, h->tx_ctx);
        }
    }
    else{
        //fprintf(stderr, "wr transfer status: %d\n", transfer->status);
        h->status = transfer->status;
    }
    
    if (ret || h->tx_exit_request){
        /* user indicate exit */
        free(transfer->buffer);
        libusb_free_transfer(transfer);
    }
    else{
        //submit the data transfer
        transfer->status = -1;        
        h->status = libusb_submit_transfer(transfer);            

        //every 125ms get fifo status
        if (h->dac_check_pkts >= num_pkts_per_125ms){
            h->dac_check_pkts = 0;
            get_usb_fifolevel(h);
        }
        //every 10 second get the rate
        if (h->tx_pkts % 10*num_pkts_per_sec == 0){ 
            get_board_clockrate(h);
        }
    }

}


static void submit_tx_transfers(sfe* h)
{
    const unsigned int num_transfers = h->num_xfers;
    const unsigned int num_iso_pkts = h->packets_per_xfer;

    if (h->tx_callback && h->num_tx_channels > 0 ){
        
        for (int i = 0; i < num_transfers; i++){
           
            struct libusb_transfer *transfer;
            unsigned char* buf;
            unsigned buf_size = h->usb->max_out_packet_size * num_iso_pkts;
            unsigned tx_size;
            unsigned prime_bytes = 0;
           
            transfer = libusb_alloc_transfer(num_iso_pkts);
            if (!transfer){
                fprintf(stderr, "cannot allocate transfer structure\n");
                return ;
            }

            /* pre-fill the data */
            buf = malloc(buf_size);
            if (!buf){
                fprintf(stderr, "cannot allocate tx buffer\n");
                return ;
            }
            
            libusb_fill_iso_transfer(transfer, h->usb->dev, h->usb->ep_data_out,
                                     buf, 0, num_iso_pkts,
                                     usb_out_callback,
                                     h, 5000);

            //prime the fifo with addition 2k data
            if (i == 0) prime_bytes = 2000;
            tx_size = set_tx_packet_info(h, transfer, prime_bytes);
            //printf("%d: %d\n", tx_size, transfer->iso_packet_desc[0].length);            

            h->tx_callback(buf, tx_size, h->tx_ctx);

            h->pp_xfers[i] = transfer;
            transfer->status = -1;
            h->status = libusb_submit_transfer(transfer);
            if (h->status){
                fprintf(stderr, "tx submit %dth transfer error\n", i);
                return ;
            }
        }
    }

}

static void submit_rx_transfers(sfe* h)
{
    // total 8 transfers, each transfer contains 10ms of data 
    const unsigned int num_transfers = h->num_xfers;
    const unsigned int num_iso_pkts = h->packets_per_xfer;
    
    if (h->rx_callback && h->num_rx_channels > 0){

       for (int i = 0; i < num_transfers; i++){
            struct libusb_transfer *transfer;
            unsigned char* buf;
            unsigned buf_size = h->usb->max_in_packet_size * num_iso_pkts;

            transfer = libusb_alloc_transfer(num_iso_pkts);
            if (!transfer){
                fprintf(stderr, "cannot allocate transfer structure\n");
                return;
            }

            buf = malloc(buf_size);
            if (!buf){
                fprintf(stderr, "cannot allocate tx buffer\n");
                return ;
            }
            //printf("rx max packet size: %d\n", h->usb->max_in_packet_size);
            libusb_fill_iso_transfer(transfer, h->usb->dev, h->usb->ep_data_in,
                                     buf, buf_size, num_iso_pkts,
                                     usb_in_callback,
                                     h, 5000);
        
            libusb_set_iso_packet_lengths(transfer, h->usb->max_in_packet_size);

            h->pp_xfers[i] = transfer;
            transfer->status = -1;
            h->status = libusb_submit_transfer(transfer);
            if (h->status){
                fprintf(stderr, "rx submit %dth transfer error\n", i);
                return ;
            }
        }
    }
    return ;
}

int sfe_set_sample_rate(sfe *h, unsigned samplerate)
{
    const unsigned clk = FPGA_CLK;
    int  div, cdiv, sys_en;
    div = (clk / samplerate - 4)/2;

    get_fpga_status(h->usb, &cdiv, NULL, NULL
                    ,NULL, NULL, &sys_en);

    if (sys_en && cdiv != div){
        fprintf(stderr, "simpleFE is running, cannot change sample rate(%d), set to: %d \n", cdiv, div);
        return -1;
    }

    if (cdiv != div){
        set_fpga_cdiv(h, div);
    }
    
    h->clk_div = div;
    h->sample_rate = clk/(div *2 + 4);
    
    return 0;
}

static void* rx_thread_func(void* ctx)
{
    sfe *h = (sfe*) ctx;

    do {
       struct timeval timeout = {0 ,5000};
       int err = libusb_handle_events_timeout(NULL, &timeout);
       if (err ==  LIBUSB_ERROR_INTERRUPTED)
            err = libusb_handle_events_timeout(NULL, &timeout);

    }while(!h->rx_exit_request);
    
    return NULL;
}

static void* tx_thread_func(void* ctx)
{
    sfe *h = (sfe*) ctx;

    do {
       struct timeval timeout = {0 ,5000};
       int err = libusb_handle_events_timeout(NULL, &timeout);
       if (err ==  LIBUSB_ERROR_INTERRUPTED)
            err = libusb_handle_events_timeout(NULL, &timeout);

    }while(!h->tx_exit_request);
    
    return NULL;
}

int sfe_tx_start(sfe *h,
                sfe_callback* tx_cb,
                void* cbdata
                )
{

    int tx_i=0,  tx_q=0;
    unsigned clk, rate;


    h->tx_pkts = 0;
    h->dac_check_pkts = 0;
    h->tx_bytes_frame_remain = 0;
    
    if (ensure_stable_clock_reading(h, &clk)){
        fprintf(stderr, "board is not boot yet\n");
        return -1;
    }
    rate =  clk/(h->clk_div*2 + 4);
    h->actual_rate = rate;
    h->tx_bytes_per_sec = rate * h->num_tx_channels * 10/8;
    if (h->tx_bytes_per_sec > (h->usb->max_out_packet_size * 8 *1000)){
        fprintf(stderr, "Sample rate too high, rate=%d, bytes=%d\n",rate, h->tx_bytes_per_sec);
        return -1;
    }

    //start thread
    h->rc_hold_count = 0;
    h->tx_callback = tx_cb;
    h->tx_ctx = cbdata;
    h->tx_exit_request = 0;

    submit_tx_transfers(h);
    
    if(pthread_create(&h->tx_thread, NULL, tx_thread_func, h)){
        fprintf(stderr, "thread creation failed\n");
        return -1;
    }

    return 0;
}

void sfe_stop_tx(sfe *h)
{
    void *ret;
    int rx_i, rx_q, tx_i, tx_q, sys_en;
    unsigned char cfg[2];

    h->tx_exit_request = 1;
    pthread_join(h->tx_thread, &ret);

    //if nothing is there, stop fpga
    get_fpga_status(h->usb, NULL, &tx_i, &tx_q, &rx_i, &rx_q, &sys_en);
    if (!sys_en){
        return;
    }


    h->xfer_ctrl = (rx_q << 2) | (rx_i << 1);

    if (!h->xfer_ctrl){
        stop_fpga(h);
    }else{
        
        cfg[0] = (1<<7); //reg 0
        cfg[1] =  h->xfer_ctrl;
        
        set_gpio(h->usb, FPGA_CS, 0);
        usb_xfer_spi(h->usb, &cfg[0], 2);
        set_gpio(h->usb, FPGA_CS, 1);

#ifdef _MSC_VER
		_sleep(1);
#else            
		usleep(1000);
#endif
        
        cfg[0] = (1<<7); //reg 0
        cfg[1] =  h->xfer_ctrl | 0x01;
    
        set_gpio(h->usb, FPGA_CS, 0);
        usb_xfer_spi(h->usb, &cfg[0], 2);
        set_gpio(h->usb, FPGA_CS, 1);
    }
}

int  sfe_rx_start(sfe *h,
                  sfe_callback* rx_cb,
                  void* cbdata
                  )
{

    int rx_i=0,  rx_q=0;
    const unsigned th = 1024 *num_pkts_per_sec;
    
    h->rx_pkts = 0;
    h->rx_data_valid = 0;

    /*
    if (h->sample_rate * h->num_rx_channels <= th){
        set_isopkts(h->usb, 1);
    }else{
        set_isopkts(h->usb, 2);
    }
    */
    
    //start thread
    h->rx_callback = rx_cb;
    h->rx_ctx = cbdata;
    h->rx_exit_request = 0;

    submit_rx_transfers(h);

    if(pthread_create(&h->rx_thread, NULL, rx_thread_func, h)){
        fprintf(stderr, "thread creation failed\n");
        return -1;
    }

    return 0;
}

void sfe_stop_rx(sfe *h)
{
    void *ret;
    int rx_i, rx_q, tx_i, tx_q, sys_en;
    unsigned char cfg[2];

    h->rx_exit_request = 1;
    pthread_join(h->rx_thread, &ret);

    //if nothing is there, stop fpga
    get_fpga_status(h->usb, NULL, &tx_i, &tx_q, &rx_i, &rx_q, &sys_en);
    if (!sys_en){
        return;
    }

    h->xfer_ctrl = (tx_q << 4) | (tx_i << 3);

    if (!h->xfer_ctrl){
        stop_fpga(h);
    }else{
        //dont have to stop fpga
        cfg[0] = (1<<7); //reg 0
        cfg[1] =  h->xfer_ctrl | 0x01;
    
        set_gpio(h->usb, FPGA_CS, 0);
        usb_xfer_spi(h->usb, &cfg[0], 2);
        set_gpio(h->usb, FPGA_CS, 1);
    }
}

    
sfe* sfe_init()
{
    unsigned char cfg[2];

    sfe *h = calloc(sizeof(sfe), 1);
    h->usb = usb_init();
    if (h->usb == NULL){
        free(h);
        fprintf(stderr, "cannot find known device to load firmware or run program\n");
        return NULL;
    }

    if (get_cdone(h->usb) == -1){
        free(h);
        fprintf(stderr, "firmware has not been loaded, load firmware first\n");
        return NULL;
    }        

    h->packets_per_xfer = NUM_PKTS_PER_XFER;
    h->num_xfers = NUM_TRANSFERS;

    h->pp_xfers = calloc(sizeof(struct libusb_transfer*), h->num_xfers);

    // 1. enable MAX6863 ADC/DAC
    cfg[0] = 0x04;
    set_gpio(h->usb, MAX5863_CS, 0);
    usb_xfer_spi(h->usb, &cfg[0], 1);
    set_gpio(h->usb, MAX5863_CS, 1);

    sfe_reset_board(h);
    
    return h;
}

unsigned sfe_get_num_data_per_transfer(sfe *h)
{
    return (unsigned)((h->sample_rate * 1.0 ) / num_pkts_per_sec * h->packets_per_xfer);
}


void sfe_close(sfe* h)
{
    usb_close(h->usb);
    free(h->pp_xfers);
    free(h);
}

void sfe_query_sample_rates(unsigned *pr)
{
    const unsigned clk = FPGA_CLK; //30M
    unsigned div;

    for (div = 0; div<128; div++){
        *pr++ = clk / (div*2 + 4);
    }
}


void sfe_reset_board(sfe* h)
{
    // Reset FPGA
    set_gpio(h->usb, FPGA_RST, 0);
    set_gpio(h->usb, FPGA_RST, 1);
}


unsigned get_real_sample_rate(sfe* h)
{
    return h->actual_rate;
}


void sfe_external_gpio_set(sfe* h, int gpio, int val)
{
    external_gpio_set(h->usb, gpio, val);
}

int sfe_spi_transfer(sfe *h, unsigned char *data, unsigned len)
{
    usb_xfer_spi(h->usb, data, len);
}


void sfe_auxdac_set(sfe* h, int ch, unsigned val)
{
    unsigned char oct0 = (val & 0xF0)>>4;
    unsigned char oct1 = val & 0x0F;
    unsigned char dac[2];
    
    ch = ch & 0x03;
    
    dac[0] = (ch<<6) | (0x01<<4) | oct0;
    dac[1] = oct1 << 4;
    set_gpio(h->usb, AUXDAC_CS, 0);
    usb_xfer_spi(h->usb, &dac[0], 2);
    set_gpio(h->usb, AUXDAC_CS, 1);

}

void sfe_i2c_write(sfe* h, unsigned char addr,  char *data, unsigned len)
{
    usb_write_i2c(h->usb, data, addr, len);
}

void sfe_i2c_read(sfe* h, unsigned char addr, char *data, unsigned len)
{
    usb_read_i2c(h->usb, data, addr, len);
}
