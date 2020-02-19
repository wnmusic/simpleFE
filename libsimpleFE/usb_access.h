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


#ifndef USB_ACCESS_H_
#define USB_ACCESS_H_

#include "libusb.h"
typedef struct
{
    libusb_device_handle *dev;
    unsigned char ep_spi_out;
    unsigned char ep_spi_in;
    unsigned char ep_data_out;
    unsigned char ep_data_in;

    /* ISO handling */
    unsigned  max_out_packet_size;
    unsigned  max_in_packet_size;
    
}sfe_usb;


sfe_usb *usb_init();
void usb_close(sfe_usb *h);
int  usb_xfer_spi(sfe_usb *h, uint8_t *data, int n);
void set_cs_creset(sfe_usb *h, int cs_b, int creset_b);
int  get_cdone(sfe_usb *h);
void set_gpio(sfe_usb *h, int gpio, unsigned val);
int get_gpio(sfe_usb *h, int gpio);
unsigned get_clkrate(sfe_usb* h);
void get_fifo_status(sfe_usb* h, unsigned *adc_level, unsigned *dac_level);
void get_fpga_status(sfe_usb* h,
                     unsigned *cdiv,
                     int* tx_i,
                     int* tx_q,
                     int* rx_i,
                     int* rx_q,
                     int* sys_en
                     );
void set_isopkts(sfe_usb *h, unsigned n);
void external_gpio_set(sfe_usb* h, int gpio, int value);
int usb_read_i2c(sfe_usb *h, uint8_t *data, uint8_t addr, int n);
int usb_write_i2c(sfe_usb *h, uint8_t *data, uint8_t addr, int n);


#define GPIO_LED    (2)
#define FPGA_RST    (5)


#define VR_GPIO 0xaa
#define VR_RATE 0xab
#define VR_I2C  0xac

#define MAX_VR_BYTES    4
#define VR_GPIO_BYTES   1
#define VR_RATE_CLOCK_BYTES      4
#define VR_RATE_STATUS_BYTES     2
#endif
