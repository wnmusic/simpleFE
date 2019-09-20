/* -*- c++ -*- */
/* 
 * Copyright 2019 GPL.
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

#ifndef INCLUDED_SIMPLEFE_SINK_C_IMPL_H
#define INCLUDED_SIMPLEFE_SINK_C_IMPL_H

#include <simplefe/sink_c.h>
#include "simpleFE.h"
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition_variable.hpp>
#include "ringbuf.h"
#include "sfe_device.h"

namespace gr {
  namespace simplefe {
      class sink_c_impl : public sink_c
      {
      private:
          sfe* m_sfe;
          boost::mutex m_buf_mutex;
          boost::condition_variable m_buf_cond;
          static int tx_callback(unsigned char* buffer, int length, void* data);
          static int fill_tx_buffer(void* dst, void* src, int src_len);
          static int calc_read_len(int dst_len);
          ring_buffer<std::complex<float>> m_ringbuf;

      public:
          sink_c_impl(unsigned sample_rate);
          ~sink_c_impl();
          int data_request(unsigned char* buffer, int length);
          void reset_simplefe(void);
          // Where all the action really happens
          int work(int noutput_items,
                   gr_vector_const_void_star &input_items,
                   gr_vector_void_star &output_items);
      };



  } // namespace simplefe
} // namespace gr

#endif /* INCLUDED_SIMPLEFE_SINK_C_IMPL_H */

