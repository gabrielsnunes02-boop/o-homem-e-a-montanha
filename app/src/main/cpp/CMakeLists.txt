cmake_minimum_required(VERSION 3.22.1)
project(pianomidi C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(oboe REQUIRED CONFIG)

add_library(pianomidi SHARED
    android/NativeBridge.cpp
    engine/PianoEngine.cpp
    engine/SampleBank.cpp
    third_party/stb_vorbis.c)

# O codigo de audio e sempre otimizado, inclusive em builds de debug.
target_compile_options(pianomidi PRIVATE -O3 -fno-finite-math-only -Wall $<$<COMPILE_LANGUAGE:C>:-w>)

target_link_libraries(pianomidi oboe::oboe amidi android log)
