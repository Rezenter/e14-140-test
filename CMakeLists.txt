cmake_minimum_required(VERSION 3.5)
project(ExampleModule)

set(CMAKE_CXX_STANDARD 11)

include_directories( /usr/include/labbot
        /usr/include/jsoncpp)

set(SOURCE_FILES ExampleModule.cpp ExampleModule.h)
add_library(ExampleModule SHARED ${SOURCE_FILES})

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/libExampleModule.so
        DESTINATION /usr/share/labbot/modules/)

#install(DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/html/
#        DESTINATION /usr/share/labbot/nginx/html)
