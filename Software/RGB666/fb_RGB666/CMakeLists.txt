cmake_minimum_required(VERSION 2.8)

SET(COMPILE_DEFINITIONS -Werror)

include_directories(/usr/include)
include_directories(/opt/vc/include)
include_directories(/opt/vc/include/interface/vcos/pthreads)
include_directories(/opt/vc/include/interface/vmcs_host)
include_directories(/opt/vc/include/interface/vmcs_host/linux)

link_directories(/opt/vc/lib)
add_executable(fb_RGB666_wPi fb_RGB666_wiringPi.c)
add_executable(fb_RGB666_bcm2835 fb_RGB666_bcm2835.c)

target_link_libraries(fb_RGB666_wPi bcm_host wiringPi)
target_link_libraries(fb_RGB666_bcm2835 bcm_host bcm2835)
