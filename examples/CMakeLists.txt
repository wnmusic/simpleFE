cmake_minimum_required(VERSION 3.0)   
project(simpleFE_Examples)

set(SIMPLEFE_INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/../libsimpleFE)
set(LIBDSP_INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/../libdsp)


if (WIN32)
set(SIMPLEFE_LIB ${CMAKE_CURRENT_SOURCE_DIR}/../libsimpleFE/build/Debug/simpleFE.lib)
set(FFTWF_LIB ${CMAKE_CURRENT_SOURCE_DIR}/../contrib/fftw-3.3.5-dll64/libfftw3f-3.lib)
set(DSP_LIB ${CMAKE_CURRENT_SOURCE_DIR}/../libsimpleFE/build/Debug/Libdsp.lib)
else()
find_package(FFTW REQUIRED)
find_package(Threads)
find_package(Udev)
endif()

add_subdirectory(bpsk)
