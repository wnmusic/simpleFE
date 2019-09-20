/* -*- c++ -*- */
/* 
 * Copyright 2019 Ning Wang.
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
#include "source_f_impl.h"
#include "ringbuf.h"
#include "simpleFE.h"

namespace gr {
  namespace simplefe {

    source_f::sptr
    source_f::make(unsigned sample_rate, int channel)
    {
      return gnuradio::get_initial_sptr
        (new source_f_impl(sample_rate, channel));
    }

    /*
     * The private constructor
     */
    source_f_impl::source_f_impl(unsigned sample_rate, int channel)
      : gr::sync_block("source_f",
              gr::io_signature::make(0, 0, 0),
              gr::io_signature::make(1, 1, sizeof(float)))
    {
          unsigned rates[SIMPLE_FE_NUM_SAMPLE_RATES];
          unsigned r = 0;
          int data_per_xfer = 0;
          int min_rate_diff = 30000000;
          int rx_i = (channel == 0);
          int rx_q = (channel == 1);
        
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
        
          m_sfe = sfe_device::get_device()->dev();
          if (!m_sfe){
              throw std::runtime_error("Cannot open simpleFE device\n");
          }

          if (sfe_set_sample_rate(m_sfe, r)){
              throw std::runtime_error("set sampling rate error, device running ? \n");
          }

          /* allocate ring buffer to hold 4 transfers , IQ two data */
          data_per_xfer = sfe_get_num_data_per_transfer(m_sfe);
          m_ringbuf.alloc_buffer(data_per_xfer*2*4);
          /* start rx thread */
		  //std::cout << "start rx" << std::endl;
          sfe_rx_enable(m_sfe, rx_i, rx_q);
          if (sfe_rx_start(m_sfe, source_f_impl::rx_callback, this)) {
              throw std::runtime_error("start rx thread ofailed\n");
          }
    }
      
      int source_f_impl::rx_callback(unsigned char* buffer, int length, void* data)
      {
          source_f_impl *obj = (source_f_impl*)data;
          return obj->write_data(buffer, length);
      }

      int source_f_impl::write_data(unsigned char* buffer, int length)
      {
          if (length > 0){
              
              boost::mutex::scoped_lock lock(m_buf_mutex);
              if (!m_ringbuf.write(buffer, length)){
                  std::cerr << "O" << std::flush;
              }
              else {
                  m_buf_cond.notify_one();
              }
              
          }
          return 0;
      }

      int source_f_impl::calc_src_len(int dst_len)
      {
          return dst_len; // 1 float is 8 bits
      }
      /*
       * Our virtual destructor.
       */
      source_f_impl::~source_f_impl()
      {
          sfe_stop_rx(m_sfe);
      }

      int source_f_impl::fill_rx_buffer(void* dst, void* src, int src_len)
      {
          float *out = (float*)dst;
          unsigned char *in = (unsigned char*)src;
          const float qinv = (1.0f/127.0f);
          for(int i=0; i<src_len; i++){
              out[i] = (in[i]-128)*qinv;
          }
          return src_len*sizeof(float);
      }

      int
      source_f_impl::work(int noutput_items,
                          gr_vector_const_void_star &input_items,
                          gr_vector_void_star &output_items)
      {
          float *out = (float *) output_items[0];

          {
              boost::mutex::scoped_lock lock(m_buf_mutex );
              while ( m_ringbuf.get_count() < calc_src_len(noutput_items)){
                  
                  m_buf_cond.wait( lock );
              }
              
              m_ringbuf.read(out, noutput_items, fill_rx_buffer, calc_src_len);
          }
          
          // Tell runtime system how many output items we produced.
          return noutput_items;

    }

  } /* namespace simplefe */
} /* namespace gr */

