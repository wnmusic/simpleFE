/* -*- c++ -*- */

#define SIMPLEFE_API

%include "gnuradio.i"			// the common stuff

//load generated python docstrings
%include "simplefe_swig_doc.i"

%{
#include "simplefe/sink_c.h"
#include "simplefe/source_c.h"
#include "simplefe/sink_f.h"
#include "simplefe/source_f.h"
%}


%include "simplefe/sink_c.h"
GR_SWIG_BLOCK_MAGIC2(simplefe, sink_c);
%include "simplefe/source_c.h"
GR_SWIG_BLOCK_MAGIC2(simplefe, source_c);
%include "simplefe/sink_f.h"
GR_SWIG_BLOCK_MAGIC2(simplefe, sink_f);
%include "simplefe/source_f.h"
GR_SWIG_BLOCK_MAGIC2(simplefe, source_f);
