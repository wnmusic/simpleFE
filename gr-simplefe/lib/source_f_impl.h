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

#ifndef INCLUDED_SIMPLEFE_SOURCE_F_IMPL_H
#define INCLUDED_SIMPLEFE_SOURCE_F_IMPL_H

#include <simplefe/source_f.h>
#include "simpleFE.h"
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition_variable.hpp>
#include "ringbuf.h"
#include "sfe_device.h"

namespace gr {
    namespace simplefe {

        class source_f_impl : public source_f
        {
        private:
            // Nothing to declare in this block.
            sfe *m_sfe;
            boost::mutex m_buf_mutex;
            boost::condition_variable m_buf_cond;
            static int rx_callback(unsigned char* buffer, int length, void* data);
            static int calc_src_len(int dst_len);
            static int fill_rx_buffer(void* dst, void* src, int src_len);
            ring_buffer<unsigned char> m_ringbuf;

        public:
            source_f_impl(unsigned sample_rate, int channel);
            ~source_f_impl();
            int write_data(unsigned char* buffer, int length);          
            // Where all the action really happens
            int work(int noutput_items,
                     gr_vector_const_void_star &input_items,
                     gr_vector_void_star &output_items);
        };

    } // namespace simplefe
} // namespace gr

#endif /* INCLUDED_SIMPLEFE_SOURCE_F_IMPL_H */

