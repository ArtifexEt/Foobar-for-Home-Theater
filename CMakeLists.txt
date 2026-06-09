cmake_minimum_required(VERSION 3.24)

project(FoobarForHomeTheater LANGUAGES CXX)

if(NOT WIN32)
    message(FATAL_ERROR "This project uses Windows Spatial Audio APIs and must be built on Windows.")
endif()

add_executable(SpatialAudioDiagnostics
    src/main.cpp
)

add_executable(StereoSpatialPlayer
    src/stereo_spatial_player.cpp
)

target_compile_features(SpatialAudioDiagnostics PRIVATE cxx_std_20)
target_compile_features(StereoSpatialPlayer PRIVATE cxx_std_20)

target_compile_definitions(SpatialAudioDiagnostics PRIVATE
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

target_link_libraries(SpatialAudioDiagnostics PRIVATE
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
    target_compile_options(SpatialAudioDiagnostics PRIVATE /W4 /permissive-)
    target_compile_options(StereoSpatialPlayer PRIVATE /W4 /permissive-)
endif()
