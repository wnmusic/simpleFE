
add_executable(ledblink ledblink.c)
target_link_libraries(ledblink LINK_PUBLIC simpleFE)

add_executable(ice40prog ice40prog.c)
if (WIN32)
   target_sources(ice40prog PRIVATE ${PROJECT_SOURCE_DIR}/../contrib/getopt/getopt.c ${PROJECT_SOURCE_DIR}/../contrib/getopt/getopt1.c)
   target_include_directories(ice40prog PRIVATE ${PROJECT_SOURCE_DIR}/../contrib/getopt/)
   add_compile_definitions(HAVE_STRING_H=1)
endif()

target_link_libraries(ice40prog LINK_PUBLIC simpleFE)


add_executable(wavegen wavegen.c)
target_link_libraries(wavegen LINK_PUBLIC simpleFE)

add_executable(loopback loopback.c)
target_link_libraries(loopback LINK_PUBLIC simpleFE)


add_executable(fw_load fw_load.c)
target_link_libraries(fw_load LINK_PUBLIC simpleFE)

add_executable(external_spi external_spi.c)
target_link_libraries(external_spi LINK_PUBLIC simpleFE)


add_executable(setfreq setfreq.c)
target_link_libraries(setfreq LINK_PUBLIC simpleFE m)
