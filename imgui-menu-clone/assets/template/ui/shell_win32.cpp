#include "shell_win32.h"
#include <dwmapi.h>
#pragma comment(lib, "dwmapi.lib")

namespace ui {

namespace {
    struct ACCENTPOLICY { int nAccentState; int nFlags; int nColor; int nAnimationId; };
    struct WINCOMPATTRDATA { int nAttribute; PVOID pData; ULONG ulDataSize; };
    constexpr int WCA_ACCENT_POLICY = 19;
}

void ApplyWindowBlur(HWND hwnd, BlurMode mode, unsigned int tint)
{
    const HMODULE user32 = ::GetModuleHandleW(L"user32.dll");
    if (!user32)
        return;

    using pSetWindowCompositionAttribute = BOOL(WINAPI*)(HWND, WINCOMPATTRDATA*);
    const auto SetWindowCompositionAttribute =
        reinterpret_cast<pSetWindowCompositionAttribute>(
            ::GetProcAddress(user32, "SetWindowCompositionAttribute"));
    if (!SetWindowCompositionAttribute)
        return;

    // nFlags = 2 makes the accent apply to the whole window. Acrylic needs a
    // tint in nColor (ABGR); plain blur ignores it.
    ACCENTPOLICY policy{ static_cast<int>(mode), 2,
                         (mode == BlurMode::Acrylic) ? static_cast<int>(tint) : 0, 0 };
    WINCOMPATTRDATA data{ WCA_ACCENT_POLICY, &policy, sizeof(policy) };
    SetWindowCompositionAttribute(hwnd, &data);
}

HWND CreateMenuWindow(const wchar_t* class_name, const wchar_t* title,
                      int width, int height, WNDPROC wndproc)
{
    WNDCLASSEXW wc{};
    wc.cbSize        = sizeof(wc);
    wc.style         = CS_CLASSDC;
    wc.lpfnWndProc   = wndproc;
    wc.hInstance     = ::GetModuleHandleW(nullptr);
    wc.hCursor       = ::LoadCursorW(nullptr, IDC_ARROW);
    wc.lpszClassName = class_name;
    ::RegisterClassExW(&wc);

    const int x = (::GetSystemMetrics(SM_CXSCREEN) - width) / 2;
    const int y = (::GetSystemMetrics(SM_CYSCREEN) - height) / 2;

    HWND hwnd = ::CreateWindowExW(0, class_name, title, WS_POPUP,
                                  x, y, width, height,
                                  nullptr, nullptr, wc.hInstance, nullptr);
    if (!hwnd)
        return nullptr;

    // Layered + extended frame: required for per-pixel alpha at the rounded
    // corners. Without this the corners render as opaque black squares.
    ::SetWindowLongPtrW(hwnd, GWL_EXSTYLE,
                        ::GetWindowLongPtrW(hwnd, GWL_EXSTYLE) | WS_EX_LAYERED);
    ::SetLayeredWindowAttributes(hwnd, RGB(0, 0, 0), 255, LWA_ALPHA);

    MARGINS margins{ -1, -1, -1, -1 };
    ::DwmExtendFrameIntoClientArea(hwnd, &margins);

    return hwnd;
}

void DragWindowWithImGui(HWND hwnd, const ImVec2& size)
{
    const ImVec2 pos = ImGui::GetWindowPos();
    if (pos.x == 0.f && pos.y == 0.f)
        return;

    RECT rc{};
    ::GetWindowRect(hwnd, &rc);
    ::MoveWindow(hwnd,
                 rc.left + static_cast<int>(pos.x),
                 rc.top  + static_cast<int>(pos.y),
                 static_cast<int>(size.x), static_cast<int>(size.y), TRUE);
    ImGui::SetWindowPos(ImVec2(0.f, 0.f));
}

} // namespace ui
