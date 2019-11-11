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

## Load the firmware before you go
Right now, the firmware of simplefe is loaded via USB as well. The firmware leaves in firmware directory.  (There is no source for firmware, mainly because it is built based on the example code from Cypress, anyone interested in re-write it using sdcc, please contact me!)

You also have to build the fw\_load (which is done when built libsimplefe), it is in libsimplefe/build/example/fw\_load

```
#cd firmware
#../libsimplfe/build/example/fw\_load simpleFE.hex
```
D6 on simpleFE will be lit if firmware is sucessfuly loaded. 

## Make firmware autoload (Linux)
On linux, the firmware loading can be done by udev. 
```
#mkdir -p /usr/local/share/simplefe
```
Copy fw\_load and simpleFE.hex to /usr/local/share/simplefe. 

Then change 
```
GROUP=`groups | cut -f 1 -d ' '`
``` 
to your own group id by running the command in the quote in udev/99-udev-simplefe.rules. 

After that 
```
#cp udev/99-udev-simplefe.rules /etc/udev/rules.d
```
Next time when you plug in simpleFE D6 will be lit automatically. 


