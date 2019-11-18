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

#include "blkconv.h"
#include <stdlib.h>
#include <string.h>

blkconv::blkconv(float *taps, int n_taps, int fft_len)
{
    int n_ffto = fft_len/2 + 1;
    int i;
    float *in;
    fftwf_complex *out;
    
    m_fft_taps = (fftwf_complex*)fftwf_malloc(n_ffto*sizeof(fftwf_complex));

    // 2 is used for hold DC data
    m_data_buf = fftwf_malloc(n_ffto*2 * sizeof(float));

    m_fft_len = fft_len;
    m_blk_size = fft_len + 1 - n_taps;
    m_overlap_size = n_taps - 1;

    m_scaling = 1.0f/m_fft_len;

    m_overlap = (float*)malloc(m_overlap_size * sizeof(float));
    for (i=0; i<m_overlap_size; i++){
        m_overlap[i] = 0.0f;
    }
    
    //fft the taps
    in    = (float*)m_data_buf;
    out   = (fftwf_complex*)m_data_buf;
    m_plan = fftwf_plan_dft_r2c_1d(m_fft_len, in, m_fft_taps, FFTW_ESTIMATE);

    for (i=0; i<n_taps; i++){
        in[i] = taps[i];
    }
    for (; i<fft_len; i++){
        in[i] = 0.0f;
    }
    fftwf_execute(m_plan);
    fftwf_destroy_plan(m_plan);

    //should be inplace 
    m_plan      = fftwf_plan_dft_r2c_1d(fft_len, in, out, FFTW_ESTIMATE);
    m_inv_plan = fftwf_plan_dft_c2r_1d(fft_len, out, in, FFTW_ESTIMATE);

}

void blkconv::process()
{

    int i;
    float *in = (float*)m_data_buf;
    fftwf_complex *out = (fftwf_complex*)m_data_buf;

    //zero padding to fft_len    
    for(i=m_blk_size; i<m_fft_len; i++){
        in[i] = 0.0f;
    }

    fftwf_execute(m_plan);

    // multiplication
    for (i=0; i<(m_fft_len/2 + 1); i++)
    {
        float re = out[i][0];
        float im = out[i][1];
        float cr = m_fft_taps[i][0];
        float ci = m_fft_taps[i][1];

        out[i][0] = m_scaling *(re * cr - im * ci);
        out[i][1] = m_scaling *(re * ci + im * cr);
    }

    fftwf_execute(m_inv_plan);
    //overlap add
    for (i=0; i<m_overlap_size; i++)
    {
        in[i]        = in[i] + m_overlap[i];
        m_overlap[i] = in[m_blk_size + i];
    }
}


blkconv::~blkconv()
{
    free(m_overlap);
    fftwf_free(m_data_buf);
    fftwf_free(m_fft_taps);

    fftwf_destroy_plan(m_plan);
    fftwf_destroy_plan(m_inv_plan);

}
