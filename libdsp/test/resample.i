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
%apply (float *INPLACE_ARRAY1, int DIM1) {(float *inout, int n_in)};


%include "resample.h"
