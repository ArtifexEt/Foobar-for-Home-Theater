cmake_minimum_required(VERSION 3.24)

project(AtmosForHomeTheaterExperiment LANGUAGES CXX)

if(NOT WIN32)
    message(FATAL_ERROR "This experiment uses Windows Spatial Audio APIs and must be built on Windows.")
endif()

add_executable(AtmosHomeTheaterExperiment
    src/main.cpp
)

target_compile_features(AtmosHomeTheaterExperiment PRIVATE cxx_std_20)
target_compile_definitions(AtmosHomeTheaterExperiment PRIVATE
    WIN32_LEAN_AND_MEAN
    NOMINMAX
    UNICODE
    _UNICODE
    _CRT_SECURE_NO_WARNINGS
)

target_link_libraries(AtmosHomeTheaterExperiment PRIVATE
    ole32
    propsys
    uuid
)

if(MSVC)
    target_compile_options(AtmosHomeTheaterExperiment PRIVATE /W4 /permissive-)
endif()
