%module resample
%{
#define SWIG_FILE_WITH_INIT
#include "resample.h"
%}


%include "numpy.i"

%init %{
import_array();
%}


%apply (float *IN_ARRAY1, int DIM1) {(float *taps, int n_taps)};
%apply (float *IN_ARRAY1, int DIM1) {(float *in, int n_in)};
%apply (float *ARGOUT_ARRAY1, int DIM1) {(float *out, int out_len)};


%include "resample.h"
