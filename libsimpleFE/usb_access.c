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
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include "usb_access.h"
#include "chip_select.h"
#include "libusb.h"

struct simplefe_usb_device{
    unsigned vid;
    unsigned pid;
    const char* name;
};

static struct simplefe_usb_device known_devices[] =
    {
     {0x1209, 0xA119, "simpleFE"}
    };
        

sfe_usb *usb_init()
{
    sfe_usb *h = calloc(sizeof(sfe_usb), 1);
    int status;
    libusb_device *dev, **devs;
    struct libusb_device_descriptor desc;
    int bfound = 0, i, j;
    
    status = libusb_init(NULL);
    if (status < 0) {
        fprintf(stderr, "libusb_init() failed: %s\n", libusb_error_name(status));
        free(h);
        return NULL;
    }

    if (libusb_get_device_list(NULL, &devs) < 0) {
        fprintf(stderr, "libusb_get_device_list() failed: %s\n", libusb_error_name(status));
        free(h);
        return NULL;
    }
    
    for (i=0; !bfound && (dev=devs[i]) != NULL; i++) {
        status = libusb_get_device_descriptor(dev, &desc);
        if (status >= 0) {
            for (j=0; j<sizeof(known_devices)/sizeof(struct simplefe_usb_device); j++){
                if (desc.idVendor == known_devices[j].vid && desc.idProduct == known_devices[j].pid){
                    //printf("found device = 0x%x, vid=%x, pid=%x\n", dev, desc.idVendor, desc.idProduct);
                    bfound = 1;
                    break;
                }
            }
        }
    }

    if (dev == NULL) {
        libusb_free_device_list(devs, 1);
        libusb_exit(NULL);
        fprintf(stderr, "could not find a known device \n");
        return NULL;
    }
    
    h->dev = NULL;
    status = libusb_open(dev, &h->dev);
    libusb_free_device_list(devs, 1);
    if (status < 0) {
        fprintf(stderr, "libusb_open() failed: %s\n", libusb_error_name(status));
        free(h);
        return NULL;
    }
    
    libusb_set_auto_detach_kernel_driver(h->dev, 1);
    status = libusb_claim_interface(h->dev, 0);

    if (status != LIBUSB_SUCCESS) {
        libusb_close(h->dev);
        fprintf(stderr, "libusb_claim_interface failed: %s\n", libusb_error_name(status));
        free(h);
        return NULL;
    }

    //libusb_set_debug(NULL, LIBUSB_LOG_LEVEL_DEBUG);

    h->ep_spi_out = 0x01;
    h->ep_spi_in  = 0x81;
    h->ep_data_out = 0x02;
    h->ep_data_in = 0x86;

    h->max_out_packet_size = libusb_get_max_iso_packet_size(dev, h->ep_data_out);
    h->max_in_packet_size = libusb_get_max_iso_packet_size(dev, h->ep_data_in);

    //printf("iso max packet size: %d : %d \n", h->max_out_packet_size, h->max_in_packet_size);
    return h;

}

void usb_close(sfe_usb *h)
{
    libusb_release_interface(h->dev, 0);
    libusb_close(h->dev);
    libusb_exit(NULL);
    free(h);
}


void set_gpio(sfe_usb *h, int gpio, unsigned val)
{
    int status;
    uint16_t addr = (gpio << 8 ) | val;
    status = libusb_control_transfer(h->dev,
                                     LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE,
                                     VR_GPIO, addr, 0x00,
                                     NULL, 0, 1000);
    if (status != 0){
        fprintf(stderr, "libusb_control_transfer failed: %s\n", libusb_error_name(status));
    }
}

unsigned get_clkrate(sfe_usb* h)
{
    int status;    
    uint8_t data[VR_RATE_CLOCK_BYTES];
    uint16_t addr = 0x0001;
    status = libusb_control_transfer(h->dev,
                                     LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE,
                                     VR_RATE, addr, 0x00,
                                     &data[0], VR_RATE_CLOCK_BYTES, 1000);
    if (status != VR_RATE_CLOCK_BYTES){
        fprintf(stderr, "libusb_control_transfer failed: %s\n", libusb_error_name(status));
    }
    
    return data[0]<<24 | data[1]<<16 | data[2]<<8 | data[3];
}

void get_fifo_status(sfe_usb* h, unsigned *adc_level, unsigned *dac_level)
{
    int status;    
    uint8_t data[VR_RATE_STATUS_BYTES];
    uint16_t addr = 0x0000;
    status = libusb_control_transfer(h->dev,
                                     LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE,
                                     VR_RATE, addr, 0x00,
                                     &data[0], VR_RATE_STATUS_BYTES, 1000);
    if (status != VR_RATE_STATUS_BYTES){
       fprintf(stderr, "libusb_control_transfer failed: %s\n", libusb_error_name(status));
    }
    if (adc_level){
        *adc_level = data[0] & 0x3F;
    }
    if (dac_level){
        *dac_level = data[1] & 0x3F;
    }
}

void get_fpga_status(sfe_usb* h,
                     unsigned *cdiv,
                     int* tx_i,
                     int* tx_q,
                     int* rx_i,
                     int* rx_q,
                     int* sys_en
                     )
{
    uint8_t data[3];
    data[0] = 0x03 << 5;
    set_gpio(h, FPGA_CS, 0);
    usb_xfer_spi(h, data, 3);
    set_gpio(h, FPGA_CS, 1);
    
    if (sys_en){
        *sys_en = data[2] & 0x01;
    }
    if (tx_q){
        *tx_q = (data[2] & 0x10)>>4;
    }
    if (tx_i){
        *tx_i = (data[2] & 0x08)>>3;
    }
    if (rx_q){
        *rx_q = (data[2] & 0x04)>>2;
    }

    if (rx_i){
        *rx_i = (data[2] & 0x02)>>1;
    }

    if (cdiv){
        *cdiv = data[1] & 0x7F;
    }
}

void external_gpio_set(sfe_usb* h, int gpio, int value)
{
    uint8_t data[3];
    
    data[0] = (gpio>7 ? 0x06 : 0x07) << 5; //WR op
    data[1] = !!value;
    data[2] = 0;
    
    set_gpio(h, FPGA_CS, 0);
    usb_xfer_spi(h, data, 3);
    set_gpio(h, FPGA_CS, 1);
}


void set_isopkts(sfe_usb *h, unsigned n)
{

    int status;    
    uint16_t addr = 0x0002 | (n<<8);
    assert(n <= 2);
    status = libusb_control_transfer(h->dev,
                                     LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE,
                                     VR_RATE, addr, 0x00,
                                     NULL, 0, 1000);
    if (status != 0){
       fprintf(stderr, "libusb_control_transfer failed: %s\n", libusb_error_name(status));
    }
}

void set_cs_creset(sfe_usb *h, int cs_b, int creset_b)
{
    set_gpio(h, 4, cs_b);
    set_gpio(h, 7, creset_b);
}


int get_gpio(sfe_usb *h, int gpio)
{
    int status;
    uint8_t data;
    uint16_t addr = (gpio << 8 ) | 0;
    status = libusb_control_transfer(h->dev,
                                     LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE,
                                     VR_GPIO, addr, 0x00,
                                     &data, 1, 1000);
    if (status != 1){
        fprintf(stderr, "libusb_control_transfe_usbr failed: %s\n", libusb_error_name(status));
        return -1;
    }
    return data;
}
    

int get_cdone(sfe_usb *h)
{
    return get_gpio(h, 6);
}

int usb_xfer_spi(sfe_usb *h, uint8_t *data, int n)
{
    int s = 0;
    int status;
    
    while (s < n){
        int bytes_sent = 0;
        int bytes_recv = 0;
        int len = n - s;
        
        len = len > 64 ? 64 : len;
        status = libusb_bulk_transfer(h->dev,
                                      h->ep_spi_out,
                                      &data[s],
                                      len,
                                      &bytes_sent,
                                      1000
                                      );
        
        if (status != 0){
            fprintf(stderr, "libusb_bulk_transfe_usbr failed: %s\n", libusb_error_name(status));
            break;
        }

        status = libusb_bulk_transfer(h->dev,
                                      h->ep_spi_in,
                                      &data[s],
                                      len,
                                      &bytes_recv,
                                      1000
                                      );
        if (status != 0){
            fprintf(stderr, "libusb_bulk_transfe_usbr failed: %s\n", libusb_error_name(status));
            break;
        }

        if (bytes_recv && bytes_sent != bytes_recv){
            fprintf(stderr, "Error: sent and recv bytes are different %d: %d\n", bytes_sent, bytes_recv);
            break;
        }
        s += bytes_sent;
    }
    return s;
}

int usb_write_i2c(sfe_usb *h, uint8_t *data, uint8_t addr, int n)
{
    int status;
    uint16_t addr16 = (addr << 8 ) | 0;
    if (n > 8) return -1;
    
    status = libusb_control_transfer(h->dev,
                                     LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE,
                                     VR_I2C, addr16, 0x00,
                                     data, n, 1000);
    if (status != 1){
        fprintf(stderr, "libusb_control_transfe_usbr failed: %s\n", libusb_error_name(status));
        return -1;
    }
    return 0;
}

int usb_read_i2c(sfe_usb *h, uint8_t *data, uint8_t addr, int n)
{
    int status;
    uint16_t addr16 = (addr << 8 ) | 0;
    if (n > 8) return -1;
    status = libusb_control_transfer(h->dev,
                                     LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE,
                                     VR_I2C, addr16, 0x00,
                                     data, n, 1000);
    if (status != 1){
        fprintf(stderr, "libusb_control_transfe_usbr failed: %s\n", libusb_error_name(status));
        return -1;
    }
    return 0;
}
