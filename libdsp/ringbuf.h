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

#ifndef MY_RINGBUF_H_
#define MY_RINGBUF_H_

#include <stdlib.h>
#include <string.h>

template <class T>
class ring_buffer
{
public:
    ring_buffer()
    {
        m_buf = NULL;
        m_bufsize = 0;
        m_count = 0;
        m_rd_pos = m_wr_pos = 0;
    }
          
    ring_buffer(int capacity) : m_bufsize(capacity)
    {
        m_buf = new T[m_bufsize];
        m_count = 0;
        m_rd_pos = m_wr_pos = 0;
    }
    ~ring_buffer()
    {
        delete[] m_buf;
    }

    void alloc_buffer(int capacity)
    {
        m_bufsize = capacity;
        if (m_buf){
            delete [] m_buf;
        }
        m_buf = new T[m_bufsize];
        m_count = 0;
        m_rd_pos = m_wr_pos = 0;
    }

    int get_space()
    {
        return m_bufsize - m_count;
    }
    int get_count()
    {
        return m_count;
    }
          
    int write(const T* data, int len)
    {
        int sz1;
        if (len > get_space()){
            return 0;
        }
              
        sz1 = m_bufsize - m_wr_pos;
        if (sz1 < len){
            memcpy(&m_buf[m_wr_pos], data, sz1*sizeof(T));
            memcpy(&m_buf[0], &data[sz1], (len-sz1)*sizeof(T));
                  
            m_wr_pos = len-sz1;
        }
        else{
            memcpy(&m_buf[m_wr_pos], data, len*sizeof(T));
            m_wr_pos += len;
        }
              
        m_count += len;
        return len;
    }

    /* conv function consumes src_len data with type T and returns 
       number of bytes put into dst, dst should have enough space 
    */
    int read(void* dst, unsigned dst_len,
             int (*conv)(void* dst, void *src, int src_len),
             int (*calc_src_len)(int dst_len)
             )
    {
        if (!conv || !calc_src_len){
            return 0;
        }
                      
        int src_len = calc_src_len(dst_len);

        if (src_len > get_count()){
            return 0;
        }
              
        int sz1 = m_bufsize - m_rd_pos;
        if (sz1 < src_len){
            int dst_sz1 = conv(dst, &m_buf[m_rd_pos], sz1);
            conv((char*)dst+dst_sz1, &m_buf[0], src_len - sz1);
            m_rd_pos = src_len - sz1;
        }
        else{
            conv(dst, &m_buf[m_rd_pos], src_len);
            m_rd_pos += src_len;
        }
              
        m_count -= src_len;
        return src_len;
    }
          
private:
    T* m_buf;
    int m_bufsize;
    int m_count;
    int m_rd_pos;
    int m_wr_pos;
};

      





#endif
