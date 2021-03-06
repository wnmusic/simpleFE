/* -*- c++ -*- */
/* 
 * Copyright 2019 gr-simplefe author.
 * 
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 * 
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this software; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gnuradio/io_signature.h>
#include "sink_f_impl.h"
#include "simpleFE.h"
#include <stdio.h>


namespace gr {
  namespace simplefe {
      sink_f::sptr
      sink_f::make(unsigned sample_rate, int channel)
      {
          return gnuradio::get_initial_sptr
              (new sink_f_impl(sample_rate, channel));
      }
      
      /*
       * The private constructor
       */
      sink_f_impl::sink_f_impl(unsigned sample_rate, int channel)
          : gr::sync_block("sink_f",
                           gr::io_signature::make(1, 1, sizeof(float)),
                           gr::io_signature::make(0, 0, 0))
      {
          unsigned rates[SIMPLE_FE_NUM_SAMPLE_RATES];
          unsigned r = 0;
          int data_per_xfer = 0;
          int min_rate_diff = 30000000;
          int tx_i = (channel == 0);
          int tx_q = (channel == 1); 
        
          sfe_query_sample_rates(rates);
          for (int i=0; i<SIMPLE_FE_NUM_SAMPLE_RATES; i++){
              if (rates[i] >= sample_rate && (rates[i] - sample_rate < min_rate_diff)){
                  r = rates[i];
                  min_rate_diff = r - sample_rate;
              }
          }
          if (r == 0){
              throw std::out_of_range("sample rate is out of range\n");
          }
          //printf("sample rate: %d\n", r);
        
          m_sfe = sfe_device::get_device()->dev();
          if (!m_sfe){
              throw std::runtime_error("Cannot open simpleFE device\n");
          }

          if (sfe_set_sample_rate(m_sfe, r)){
              throw std::runtime_error("set sampling rate error, device running ? \n");
          }
          
          /* allocate ring buffer to hold 4 transfers */
          data_per_xfer = sfe_get_num_data_per_transfer(m_sfe);
          m_ringbuf.alloc_buffer(data_per_xfer*4);

          /* start tx thread */
          std::cout << "start tx: I" << tx_i << "Q" << tx_q <<  std::endl;
          sfe_tx_enable(m_sfe, tx_i, tx_q);
          if (sfe_tx_start(m_sfe, sink_f_impl::tx_callback, this)) {
              throw std::runtime_error("start tx failed\n");
          }
      }
      
      int sink_f_impl::tx_callback(unsigned char* buffer, int length, void* data)
      {
          sink_f_impl *obj = (sink_f_impl*)data;
          return obj->data_request(buffer, length);
      }


      int sink_f_impl::data_request(unsigned char* buffer, int length)
      {
          boost::mutex::scoped_lock lock(m_buf_mutex);
          if (m_ringbuf.get_count() < calc_read_len(length)) {
              memset(buffer, 0, length);
              std::cerr << "U" << std::flush;
          }
          else {
              //copy and convert data
              m_ringbuf.read(buffer, length, sink_f_impl::fill_tx_buffer, sink_f_impl::calc_read_len);
              m_buf_cond.notify_one();
          }
          return 0;
      }

        
      int sink_f_impl::calc_read_len(int dst_len)
      {
          /* notice that dst_len should be a multiple of 5,  5 bytes is 4 10bits data */
          return dst_len / 5 * 4;
      }

      int sink_f_impl::fill_tx_buffer(void* dst, void* src, int src_len)
      {
          /* dst should have enought memory */
          /* src_len is a multiple of 4, it represents number of data */          
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

      
      void sink_f_impl::reset_simplefe(void)
      {
          sfe_reset_board(m_sfe);
      }
      
      
      /*
       * Our virtual destructor.
       */
      sink_f_impl::~sink_f_impl()
      {
      }
      
      int
      sink_f_impl::work(int noutput_items,
                        gr_vector_const_void_star &input_items,
                        gr_vector_void_star &output_items)
      {
          const float *in = (const float *) input_items[0];
          {
              boost::mutex::scoped_lock lock(m_buf_mutex );
              while ( m_ringbuf.get_space() < noutput_items){
                  //printf("space: %d, %d\n", m_ringbuf.get_space(), m_ringbuf.get_count());
                  m_buf_cond.wait( lock );
              }
              m_ringbuf.write(in, noutput_items);
          }
          consume_each(noutput_items);
          return 0;
      }

  } /* namespace simplefe */
} /* namespace gr */

