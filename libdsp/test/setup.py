#! /usr/bin/env python
from __future__ import division, print_function

# System imports
from distutils.core import *
from distutils      import sysconfig

# Third-party modules - we depend on numpy for everything
import numpy

# Obtain the numpy include directory.
numpy_include = numpy.get_include()

# Array extension module
_resample = Extension("_resample",
                      ['resample.i', '../resample.cxx'],
                      swig_opts=['-I..', '-c++'],
                      include_dirs = [numpy_include, ".."]
                   )

# NumyTypemapTests setup
setup(name        = "dsplib",
      description = "Python wrapping of hte dsplib ",
      author      = "Ning Wang",
      py_modules  = ["resample"],
      ext_modules = [_resample]
      )
