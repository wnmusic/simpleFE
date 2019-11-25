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

#ifndef DECIMATE_H_
#define DECIMATE_H_

class decimate
{
public:
  /* taps and n_taps defines the interpolation filter
   * upsample defines the interpolation ratio */
  decimate(float *taps, int n_taps, int upsample, int blksize);
  ~decimate();

  /* given a rate of "how many" input samples (step size, like 10.52) to 
   * produce an output, typically this should be sufficiently large (>= 8 )
   * to use this class instead of resample. 
   * number of output samples generated is returned 
   */
  int process(float* in, int n_in, float *out, int out_len, float rate);
 private:
  float          *m_taps;
  int             m_n_taps;
  float          *m_history;
  int             m_up_ratio;
  float          *m_in;

  int             m_blksize;
  int             m_len;
  int             m_pos;
  float           m_mu;
  float           m_last_remain;
  bool            m_is_leftover;

  
  float get_sample(int phase, int n);
};


#endif
