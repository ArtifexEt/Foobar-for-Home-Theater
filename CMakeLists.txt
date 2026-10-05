cmake_minimum_required(VERSION 3.24)

project(FoobarForHomeTheater LANGUAGES CXX)

include(CTest)
if(BUILD_TESTING)
    add_executable(SpatialDspTests tests/dsp_tests.cpp)
    target_compile_features(SpatialDspTests PRIVATE cxx_std_20)
    target_compile_definitions(SpatialDspTests PRIVATE SPATIAL_AUDIO_PORTABLE_TEST)
    add_executable(SpatialOutputConfigTests tests/output_config_tests.cpp)
    target_compile_features(SpatialOutputConfigTests PRIVATE cxx_std_20)
    target_compile_definitions(SpatialOutputConfigTests PRIVATE SPATIAL_AUDIO_PORTABLE_TEST)
    add_test(NAME output_config_compatibility COMMAND SpatialOutputConfigTests)
    foreach(case IN ITEMS channel_transport output_routing config_compatibility spatial_synthesis spatial_preservation spatial_controls height_compatibility height_preservation height_synthesis)
        add_test(NAME ${case} COMMAND SpatialDspTests ${case})
    endforeach()
endif()

if(NOT WIN32)
    if(NOT BUILD_TESTING)
        message(FATAL_ERROR "The playback tools require Windows. Enable BUILD_TESTING for portable DSP tests.")
    endif()
    message(STATUS "Building portable DSP tests only; playback and Windows rendering require Windows.")
    return()
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
