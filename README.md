# simpleFE
A very low cost mixed front end (ADC, DAC, SDR)

## Pre-requisite
simpleFE uses libusb, so a working libusb should be ready. Under linux, libusb also requires udev as the driver interface.  

If you would like to use gr-simplefe, you can install the latest gnuradio, gr-simplefe work as an out of tree module. Make it work on Windows is pretty involved. (please hack the CMakefiles to locate BOOST and CPPUnit)

For libdsp which is continously evolving, fftw3 is a must. 

## Install
```
# cd libsimplefe
# mkdir build
# cd build
# cmake ..
# make
# make install
```

for gr-smiplefe

```
# cd libsimplefe
# mkdir build
# cd build
# cmake ..
# make
# make install
```



