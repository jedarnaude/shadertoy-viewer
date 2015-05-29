#pragma once

// This is included per platform only available due to project include paths
#include "ImguiPlatform.h"

bool ImGuiInit(void *window);
void ImGuiShutdown();
void ImGuiNewFrame();

void ImGuiResetDevice();
bool ImGuiCreateDevice();
