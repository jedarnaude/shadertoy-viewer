#ifndef shadertoy_test_ImguiPlatform_h
#define shadertoy_test_ImguiPlatform_h

#include "imgui.h"

#ifdef __OBJC__
#include <AppKit/AppKit.h>
NSEvent* ImGuiEventMonitor(NSEvent *event);
#endif

void ImGuiRenderDrawLists(ImDrawList** const cmd_lists, int cmd_lists_count);
void ImGuiNewFramePlatform(ImGuiIO &io);

#endif
