#ifndef shadertoy_test_ImguiPlatform_h
#define shadertoy_test_ImguiPlatform_h

#include "imgui.h"
#include <AppKit/AppKit.h>

void ImGuiRenderDrawLists(ImDrawList** const cmd_lists, int cmd_lists_count);
void ImGuiNewFramePlatform(ImGuiIO &io);
NSEvent* ImGuiEventMonitor(NSEvent *event);

#endif
