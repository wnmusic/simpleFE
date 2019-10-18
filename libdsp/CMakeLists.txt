cmake_minimum_required(VERSION 3.0)   
project(Libdsp)

set ( CMAKE_MODULE_PATH
  ${PROJECT_SOURCE_DIR}/cmake
)

if (WIN32)
set(FFTW_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../contrib/fftw-3.3.5-dll64)
set(FFTWF_LIB ${CMAKE_CURRENT_SOURCE_DIR}/../contrib/fftw-3.3.5-dll64/libfftw3f-3.lib)
else()
find_package(FFTW REQUIRED)
endif()

add_library(Libdsp blkconv.cxx)

target_include_directories(Libdsp  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(Libdsp  PUBLIC ${FFTW_INCLUDE_DIR})

target_link_libraries(Libdsp PUBLIC ${FFTWF_LIB})

add_subdirectory(test)
