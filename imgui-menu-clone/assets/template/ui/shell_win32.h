// ui/shell_win32.h — borderless, blurred, layered Win32 host window.
//
// The frosted-glass look behind a menu is NOT drawn by ImGui. It is a DWM
// composition attribute applied to the host window. Attempting to fake it with
// the draw list is the single most common way to fail at cloning these menus.
#pragma once
#include <windows.h>
#include "imgui.h"

namespace ui {

enum class BlurMode
{
    None       = 0,  // ACCENT_DISABLED
    Transparent= 2,  // ACCENT_ENABLE_TRANSPARENTGRADIENT
    Blur       = 3,  // ACCENT_ENABLE_BLURBEHIND      <- classic menu look
    Acrylic    = 4,  // ACCENT_ENABLE_ACRYLICBLURBEHIND (needs a tint colour)
};

// Applies blur behind `hwnd`. `tint` is ABGR and only used by Acrylic.
void ApplyWindowBlur(HWND hwnd, BlurMode mode, unsigned int tint = 0x01000000);

// Creates the borderless popup window (WS_POPUP + WS_EX_LAYERED + DWM frame
// extension) that these menus live in.
HWND CreateMenuWindow(const wchar_t* class_name, const wchar_t* title,
                      int width, int height, WNDPROC wndproc);

// Drag-by-window-body: keeps the OS window glued to ImGui's window position.
void DragWindowWithImGui(HWND hwnd, const ImVec2& size);

} // namespace ui
