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
#include "source_c_impl.h"
#include "ringbuf.h"
#include "simpleFE.h"

namespace gr {
  namespace simplefe {

    source_c::sptr
    source_c::make(unsigned sample_rate)
    {
      return gnuradio::get_initial_sptr
        (new source_c_impl(sample_rate));
    }

    /*
     * The private constructor
     */
      source_c_impl::source_c_impl(unsigned sample_rate)
          : gr::sync_block("source_c",
                           gr::io_signature::make(0, 0, 0),
                           gr::io_signature::make(1, 1, sizeof(gr_complex)))
      {
          unsigned rates[SIMPLE_FE_NUM_SAMPLE_RATES];
          unsigned r = 0;
          int data_per_xfer = 0;
          int min_rate_diff = 30000000;
        
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
		  sfe_rx_enable(m_sfe, 1, 1);
          if (sfe_rx_start(m_sfe, source_c_impl::rx_callback, this)) {
              throw std::runtime_error("start rx thread ofailed\n");
          }
      }
      
      int source_c_impl::rx_callback(unsigned char* buffer, int length, void* data)
      {
          source_c_impl *obj = (source_c_impl*)data;
          return obj->write_data(buffer, length);
      }

      int source_c_impl::write_data(unsigned char* buffer, int length)
      {
          if (length > 0){
			  if (length & 0x01) {
				  printf("odd number!!!, packet corruption, discard\n");
				  return 0;
			  }
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

      int source_c_impl::calc_src_len(int dst_len)
      {
          return dst_len * 2; //1 complex has 2 bytes 
      }
      /*
       * Our virtual destructor.
       */
      source_c_impl::~source_c_impl()
      {
          sfe_stop_rx(m_sfe);
      }

      int source_c_impl::fill_rx_buffer(void* dst, void* src, int src_len)
      {
          gr_complex *out = (gr_complex*)dst;
          unsigned char *in = (unsigned char*)src;
          int j = 0;
          const float qinv = (1.0f/127.0f);
          for(int i=0; i<src_len; i+=2,j++){
              out[j] = gr_complex( (in[i]-128)*qinv, (in[i+1]-128)*qinv );
          }

          return j*sizeof(gr_complex);
      }
      
      int
      source_c_impl::work(int noutput_items,
                          gr_vector_const_void_star &input_items,
                          gr_vector_void_star &output_items)
      {
          gr_complex *out = (gr_complex *) output_items[0];

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

