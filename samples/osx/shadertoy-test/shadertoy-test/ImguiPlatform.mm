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
        NSRect converted = [[event window] convertRectToScreen:rect];
        NSPoint point = [NSEvent mouseLocation];
        io.MousePos.x = (signed short)(point.x - converted.origin.x);
        io.MousePos.y = (signed short)(converted.size.height - (point.y - converted.origin.y));
        break;
    }
    case NSKeyDown:
//        if (wParam < 256)
//            io.KeysDown[wParam] = 1;
        break;
    case NSKeyUp:
//        if (wParam < 256)
//            io.KeysDown[wParam] = 0;
        break;
//    case WM_CHAR:
//        // You can also use ToAscii()+GetKeyboardState() to retrieve characters.
//        if (wParam > 0 && wParam < 0x10000)
//            io.AddInputCharacter((unsigned short)wParam);
//        return true;
    case NSFlagsChanged:
        break;
    default:
        break;
    }
    return event;
}

bool ImGuiInit(void *window) {
    last_time = GetTimeNow();

    ImGuiIO& io = ImGui::GetIO();
//    io.KeyMap[ImGuiKey_Tab] = VK_TAB;
    io.KeyMap[ImGuiKey_LeftArrow] = NSLeftArrowFunctionKey;
    io.KeyMap[ImGuiKey_RightArrow] = NSRightArrowFunctionKey;
    io.KeyMap[ImGuiKey_UpArrow] = NSUpArrowFunctionKey;
    io.KeyMap[ImGuiKey_DownArrow] = NSDownArrowFunctionKey;
    io.KeyMap[ImGuiKey_Home] = NSHomeFunctionKey;
    io.KeyMap[ImGuiKey_End] = NSEndFunctionKey;
    io.KeyMap[ImGuiKey_Delete] = NSDeleteFunctionKey;
//    io.KeyMap[ImGuiKey_Backspace] = VK_BACK;
//    io.KeyMap[ImGuiKey_Enter] = VK_RETURN;
//    io.KeyMap[ImGuiKey_Escape] = VK_ESCAPE;
    io.KeyMap[ImGuiKey_A] = 'A';
    io.KeyMap[ImGuiKey_C] = 'C';
    io.KeyMap[ImGuiKey_V] = 'V';
    io.KeyMap[ImGuiKey_X] = 'X';
    io.KeyMap[ImGuiKey_Y] = 'Y';
    io.KeyMap[ImGuiKey_Z] = 'Z';
    io.ImeWindowHandle = window;

    io.RenderDrawListsFn = ImGuiRenderDrawLists;
    //io.SetClipboardTextFn = ImGui_ImplGlfwGL3_SetClipboardText;
    //io.GetClipboardTextFn = ImGui_ImplGlfwGL3_GetClipboardText;
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

    // Read keyboard modifiers inputs
//    io.KeyCtrl = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
//    io.KeyShift = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
//    io.KeyAlt = (GetKeyState(VK_MENU) & 0x8000) != 0;
    // io.KeysDown : filled by WM_KEYDOWN/WM_KEYUP events
    // io.MousePos : filled by WM_MOUSEMOVE events
    // io.MouseDown : filled by WM_*BUTTON* events
    // io.MouseWheel : filled by WM_MOUSEWHEEL events
}