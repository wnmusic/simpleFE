# FindLibUSB.cmake - Try to find the Hiredis library
# Once done this will define
#
#  LIBUSB_FOUND - System has libusb
#  LIBUSB_INCLUDE_DIR - The libusb include directory
#  LIBUSB_LIBRARY - The libraries needed to use libusb
#  LIBUSB_DEFINITIONS - Compiler switches required for using libusb
#
#  Original from https://github.com/texane/stlink/blob/master/cmake/modules/FindLibUSB.cmake


#if(WIN32)
#if (EXISTS ${CMAKE_BINARY_DIR}/libusb-1.0.22)
#else()
#  file(DOWNLOAD
#        https://sourceforge.net/projects/libusb/files/libusb-1.0/libusb-1.0.22/libusb-1.0.22.7z/download
#        ${CMAKE_BINARY_DIR}/libusb-1.0.22.7z
#        SHOW_PROGRESS
#	)
#  
#  execute_process(COMMAND "/mnt/c/Program Files/7-Zip/7z.exe" x -y libusb-1.0.22.7z -olibusb-1.0.22
#	  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
#  )
#endif()
#endif()


FIND_PATH(LIBUSB_INCLUDE_DIR NAMES libusb.h
   HINTS
   /usr
   /usr/local
   /opt
   /mingw64/include
   ${CMAKE_SOURCE_DIR}/../contrib/libusb/libusb/
   PATH_SUFFIXES libusb-1.0
   )

if (APPLE)
  set(LIBUSB_NAME libusb-1.0.a)
elseif(WIN32)
  set(LIBUSB_NAME libusb-1.0.lib)
else()
  set(LIBUSB_NAME usb-1.0)
endif()


FIND_LIBRARY(LIBUSB_LIBRARY NAMES libusb-1.0.a libusb-1.0.lib
  HINTS
  /usr
  /usr/local
  /opt
  /mingw64/lib
  ${CMAKE_SOURCE_DIR}/../contrib/libusb/x64/Release/lib/
)

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(Libusb DEFAULT_MSG LIBUSB_LIBRARY LIBUSB_INCLUDE_DIR)

MARK_AS_ADVANCED(LIBUSB_INCLUDE_DIR LIBUSB_LIBRARY)