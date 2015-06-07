#pragma once

/**
 * This header file is meant to be extended with each platform we add.
 * @see http://stackoverflow.com/questions/5919996/how-to-detect-reliably-mac-os-x-ios-linux-windows-in-c-preprocessor
 */
#ifdef _WIN32
    // TODO(jose): windows remove glew completely. Do it manually
    #include "glew.h"
    #ifdef _WIN64
        // Nothing
    #endif
#elif __APPLE__
    #include "TargetConditionals.h"
    #if TARGET_IPHONE_SIMULATOR
         // iOS Simulator
    #elif TARGET_OS_IPHONE
        // iOS device
    #elif TARGET_OS_MAC
        #include <OpenGL/gl3.h>
    #else
    #error Unsupported platform
    #endif
#elif __linux
    // linux
#elif __unix // all unices not caught above
    // Unix
#elif __posix
    // POSIX
#else
#error Unknown platform
#endif
