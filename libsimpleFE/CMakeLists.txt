cmake_minimum_required(VERSION 3.0)   
project(libsimpleFE)


set ( CMAKE_MODULE_PATH
  ${PROJECT_SOURCE_DIR}/cmake
)

find_package(LibUSB REQUIRED)
find_package(Threads)

message(STATUS "libusb inc: " ${LIBUSB_INCLUDE_DIR})
message(STATUS "libusb lib: " ${LIBUSB_LIBRARY})
add_library(simpleFE usb_access.c simpleFE.c)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")

target_include_directories(simpleFE  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(simpleFE  PUBLIC ${LIBUSB_INCLUDE_DIR})

if (WIN32)
  target_include_directories(simpleFE  PRIVATE ${PROJECT_SOURCE_DIR}/../contrib/pthread-win32/)
  target_link_libraries(simpleFE PUBLIC ${PROJECT_SOURCE_DIR}/../contrib/pthread-win32/bin/x64_MSVC2015.Release/pthread_lib.lib)
  add_compile_definitions(HAVE_STRUCT_TIMESPEC=1 PTW32_STATIC_LIB=1)
elseif(APPLE)
  target_link_libraries(simpleFE PUBLIC ${LIBUSB_LIBRARY} ${CMAKE_THREAD_LIBS_INIT})
else()
find_package(Udev)
target_link_libraries(simpleFE PUBLIC ${LIBUSB_LIBRARY} ${CMAKE_THREAD_LIBS_INIT})
target_link_libraries(simpleFE PUBLIC ${UDEV_LIBRARIES})

endif()


add_subdirectory(example)
