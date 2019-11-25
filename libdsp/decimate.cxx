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
#include <string.h>
#include "decimate.h"
#include <math.h>
#include <assert.h>

decimate::decimate(float *taps, int n_taps, int upsample, int blksize)
  : m_up_ratio(upsample), m_n_taps(n_taps), m_blksize(blksize)
  , m_pos(0), m_in(NULL), m_mu(0.0f), m_last_remain(0.0f), m_is_leftover(false)
{

  if (m_n_taps%2 == 0){
    m_n_taps++;
  }
  m_taps = new float[m_n_taps];
  for (int i=0; i<n_taps; i++){
    m_taps[i] = taps[i];
  }
  if (n_taps < m_n_taps){
    m_taps[n_taps] = 0.0f;
  }

  m_len = m_n_taps + m_blksize;
  m_history = new float[m_len];
  for (int i=0; i<m_len; i++){
    m_history[i] = 0.0f;
  }

}


decimate::~decimate()
{
  delete[] m_taps;
  delete[] m_history;
}


int decimate::process(float* in, int n_in, float* out, int out_len, float rate)
{
    //fitler for each polyphase and store them;
    int n_out = 0;
    float t = m_pos + m_mu; //current time

    if (rate < 1.0){
        printf("rate should be larger than 1.0\n");
        return 0;
    }
    if (n_in > m_blksize){
        printf("number of samples should be less than blksize\n");
        return 0;
    }
	
    if (out_len < floorf(n_in*1.0f/rate)){
        printf("output buffer is not large enough");
        return 0;
    }            

    memcpy(&m_history[0], &m_history[n_in], sizeof(float)*(m_len - n_in));
    memcpy(&m_history[m_len - n_in], in, sizeof(float)*n_in);
    m_in = &m_history[m_len - n_in];
    
    //get the output
    // beacuse rate > 1/upsample, there can only be a sample use the old
    // remaining
    if (m_is_leftover){
      out[n_out++] = m_last_remain * (1.0f-m_mu) + m_mu*get_sample(0, 0);
      m_is_leftover = false;
      t += rate * m_up_ratio;
    }
        
    while(1)
    {
        int phase0, phase1, n0, n1, pos1;
        
        m_pos = (int)floorf(t);
        m_mu = t - m_pos;
        pos1 = m_pos + 1;

        phase0 = m_pos % m_up_ratio;
        phase1 = pos1 % m_up_ratio;
        n0 = m_pos / m_up_ratio;
        n1 = pos1 / m_up_ratio;

        if (n0 >= n_in || n_out >= out_len){
            break;
        }
        else if (n1 >= n_in){
            m_is_leftover = true;
            m_last_remain = get_sample(phase0,n0);
            break;
        }
        out[n_out++] = get_sample(phase0,n0) * (1.0f-m_mu) + m_mu*get_sample(phase1,n1);
        t += rate * m_up_ratio;
    }

    m_pos -= n_in * m_up_ratio;
    return n_out;
}
        
        
float decimate::get_sample(int phase, int n)
{
  float accu = 0.0f;
  for(int m=phase, j=0; m<m_n_taps; m += m_up_ratio, j++){
    accu += m_taps[m] * m_in[n - j];
  }

  return accu;
}
        
