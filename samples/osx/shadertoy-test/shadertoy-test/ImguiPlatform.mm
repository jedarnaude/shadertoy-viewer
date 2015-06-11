#include "ImguiPlatform.h"

#include <sys/time.h>

static double last_time = 0.0;

static double
GetTimeNow() {
	timeval tv;
	gettimeofday(&tv, NULL);
	return tv.tv_sec * 1000.0 + tv.tv_usec / 1000.0;
}

NSEvent* ImGuiEventMonitor(NSEvent *event) {
    ImGuiIO& io = ImGui::GetIO();
    
    switch ([event type]) {
    case NSLeftMouseDown:
        io.MouseDown[0] = true;
        break;
    case NSLeftMouseUp:
        io.MouseDown[0] = false;
        break;
    case NSRightMouseDown:
        io.MouseDown[1] = true;
        break;
    case NSRightMouseUp:
        io.MouseDown[1] = false;
        break;
    case NSScrollWheel:
        io.MouseWheel += [event deltaY] > 0 ? +1.0f : -1.0f;
        break;
    case NSLeftMouseDragged:
    case NSMouseMoved: {
        NSRect rect = [[[event window] contentView] frame];
        NSPoint point = [event locationInWindow];
        io.MousePos.x = (signed short)(point.x);
        io.MousePos.y = (signed short)(rect.size.height - point.y);
        break;
    }
    default:
        break;
    }
    return event;
}

bool ImGuiInit(void *window) {
    last_time = GetTimeNow();

    ImGuiIO& io = ImGui::GetIO();
    io.ImeWindowHandle = window;

    io.RenderDrawListsFn = ImGuiRenderDrawLists;
    return true;
}

// We use this function to gather platform specific data
void ImGuiNewFramePlatform(ImGuiIO &io) {
    NSWindow *window = (__bridge NSWindow *)io.ImeWindowHandle;
    NSSize size = [[window contentView] frame].size;
    CGFloat scale = [[window contentView] backingScaleFactor];
    io.DisplaySize = ImVec2(size.width * scale, size.height * scale);

    // Setup time step
    double current_time = GetTimeNow();
    io.DeltaTime = (float)(current_time - last_time) / 1000;
    last_time = current_time;
}