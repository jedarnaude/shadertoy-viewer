#ifndef shadertoy_test_DataPath_h
#define shadertoy_test_DataPath_h

#include <Windows.h>

const char* GetDataPath() {
    return "..\\..\\data";
}

const char* GetPathSeparator() {
    return "\\";
}

#endif
