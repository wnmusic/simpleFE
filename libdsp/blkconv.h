#ifndef BLK_CONV_H_
#define BLK_CONV_H_

#include <fftw3.h>

class blkconv
{
public: 
    blkconv(float *taps, int n_taps, int fft_len);
    ~blkconv();
    int get_blksize()
    {
        return m_blk_size;
    }
    float* get_process_buf()
    {
        return (float*)m_data_buf;
    }
    void process();
private:

    fftwf_plan      m_plan;
    fftwf_plan      m_inv_plan;
    
    fftwf_complex  *m_fft_taps;
    void           *m_data_buf;
    float          *m_overlap;
    float           m_scaling;
    
    int             m_fft_len;
    int             m_blk_size;
    int             m_overlap_size;
};


#endif
