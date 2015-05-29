#pragma once

#include <Windows.h>
#include "imgui.h"

void ImGuiRenderDrawLists(ImDrawList** const cmd_lists, int cmd_lists_count);
void ImguiNewFramePlatform(ImGuiIO &io);

LRESULT ImGuiWndProcHandler(HWND, UINT msg, WPARAM wParam, LPARAM lParam);
