# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

# Sets the minimum version of CMake required to build the native library.

cmake_minimum_required(VERSION 3.4.1)

set(LIB_DIR ${CMAKE_SOURCE_DIR}/../ffmpeg/lib/${ANDROID_ABI})

# Sets the so output directory
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/../jniLibs/${ANDROID_ABI})

include_directories(
        ./../ffmpeg/include
        ./include)

# Creates and names a library, sets it as either STATIC
# or SHARED, and provides the relative paths to its source code.
# You can define multiple libraries, and CMake builds them for you.
# Gradle automatically packages shared libraries with your APK.

add_library( # Sets the name of the library.
        ffmpeg-decode

        # Sets the library as a shared library.
        SHARED

        # Provides a relative path to your source file(s).
        sw_audio_decoder.c
        FFDecoder.cpp
        adecoder.c
        vdecoder.c)

# Searches for a specified prebuilt library and stores the path as a
# variable. Because CMake includes system libraries in the search path by
# default, you only need to specify the name of the public NDK library
# you want to add. CMake verifies that the library exists before
# completing its build.

find_library( # Sets the name of the path variable.
        log-lib

        # Specifies the name of the NDK library that
        # you want CMake to locate.
        log)

# Specifies libraries CMake should link to your target library. You
# can link multiple libraries, such as libraries you define in this
# build script, prebuilt third-party libraries, or system libraries.

target_link_libraries( # Specifies the target library.
        ffmpeg-decode

        # Links the target library to the log library
        # included in the NDK.
        # 注意依赖的库的顺序，否则会编译失败
        ${LIB_DIR}/libavformat.a
        ${LIB_DIR}/libavcodec.a
        ${LIB_DIR}/libavutil.a
        ${LIB_DIR}/libswresample.a
        ${LIB_DIR}/libswscale.a
        ${log-lib}
        -landroid
        -llog -lm -lz) # 'uncompress' error