cmake_minimum_required(VERSION 3.24)

project(SpatialAudioForHomeTheaterExperiment LANGUAGES CXX)

if(NOT WIN32)
    message(FATAL_ERROR "This experiment uses Windows Spatial Audio APIs and must be built on Windows.")
endif()

add_executable(SpatialAudioHomeTheaterExperiment
    src/main.cpp
)

add_executable(StereoSpatialPlayer
    src/stereo_spatial_player.cpp
)

target_compile_features(SpatialAudioHomeTheaterExperiment PRIVATE cxx_std_20)
target_compile_features(StereoSpatialPlayer PRIVATE cxx_std_20)

target_compile_definitions(SpatialAudioHomeTheaterExperiment PRIVATE
    WIN32_LEAN_AND_MEAN
    NOMINMAX
    UNICODE
    _UNICODE
    _CRT_SECURE_NO_WARNINGS
)
target_compile_definitions(StereoSpatialPlayer PRIVATE
    WIN32_LEAN_AND_MEAN
    NOMINMAX
    UNICODE
    _UNICODE
    _CRT_SECURE_NO_WARNINGS
)

target_link_libraries(SpatialAudioHomeTheaterExperiment PRIVATE
    ole32
    propsys
    uuid
)
target_link_libraries(StereoSpatialPlayer PRIVATE
    ole32
    propsys
    uuid
)

if(MSVC)
    target_compile_options(SpatialAudioHomeTheaterExperiment PRIVATE /W4 /permissive-)
    target_compile_options(StereoSpatialPlayer PRIVATE /W4 /permissive-)
endif()
