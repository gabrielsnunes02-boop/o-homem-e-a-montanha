cmake_minimum_required(VERSION 3.18)
project(piano_host_test C CXX)
set(CMAKE_CXX_STANDARD 17)
set(ENGINE ${CMAKE_CURRENT_SOURCE_DIR}/../app/src/main/cpp)
add_executable(host_test
    host_test.cpp
    ${ENGINE}/engine/PianoEngine.cpp
    ${ENGINE}/engine/SampleBank.cpp
    ${ENGINE}/third_party/stb_vorbis.c)
target_compile_options(host_test PRIVATE -O2 -Wall -Wextra $<$<COMPILE_LANGUAGE:C>:-w>)
find_package(Threads REQUIRED)
target_link_libraries(host_test Threads::Threads m)
