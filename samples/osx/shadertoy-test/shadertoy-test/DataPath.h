#ifndef shadertoy_test_DataPath_h
#define shadertoy_test_DataPath_h

#include <CoreFoundation/CoreFoundation.h>

const char* GetDataPath() {
    // Cocoa "shortcuts"
    CFBundleRef main_bundle = CFBundleGetMainBundle();
    CFURLRef data_url = CFBundleCopyResourceURL(main_bundle, CFSTR("data"), NULL, NULL);
    CFStringRef data_path = CFURLCopyFileSystemPath(data_url, kCFURLPOSIXPathStyle);
    CFStringEncoding encoding = CFStringGetSystemEncoding();
    return CFStringGetCStringPtr(data_path, encoding);
}

const char* GetPathSeparator() {
    return "/";
}

#endif
