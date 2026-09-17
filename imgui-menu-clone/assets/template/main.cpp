// main.cpp — host shell + menu layout.
//
// Everything visual lives in ui/. This file only: creates the blurred window,
// boots D3D11, loads fonts, and describes the layout.
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"

#include "ui/theme.h"
#include "ui/anim.h"
#include "ui/widgets.h"
#include "ui/shell_win32.h"
#include "ui/themes/vision.h"

#if __has_include("ui/IconsFontAwesome6.h")
  #include "ui/IconsFontAwesome6.h"
  #define UI_HAS_ICONS 1
#else
  // Bootstrap did not fetch the icon font: fall back to no glyphs.
  #define ICON_FA_CROSSHAIRS ""
  #define ICON_FA_SLIDERS    ""
  #define ICON_FA_GEAR       ""
  #define ICON_FA_FILE       ""
  #define UI_HAS_ICONS 0
#endif

#include <d3d11.h>
#include <windows.h>

#pragma comment(lib, "d3d11.lib")

static ID3D11Device*           g_device       = nullptr;
static ID3D11DeviceContext*    g_context      = nullptr;
static IDXGISwapChain*         g_swapchain    = nullptr;
static ID3D11RenderTargetView* g_rtv          = nullptr;
static HWND                    g_hwnd         = nullptr;

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND, UINT, WPARAM, LPARAM);

// ─── state driven by the menu ───────────────────────────────────────────────
namespace vars {
    bool  toggle       = false;
    int   key          = ImGuiKey_None;
    int   slider_i     = 1;
    float slider_f     = 1.f;
    char  input[64]    = "";
    float color[4]     = { 230 / 255.f, 99 / 255.f, 240 / 255.f, 1.f };
    int   combo        = 0;
    bool  multi[5]     = { true, true, true, true, true };
    const char* const items[3]      = { "One", "Two", "Three" };
    const char* const multi_items[5]= { "One", "Two", "Three", "Four", "Five" };
}

static void CreateRenderTarget()
{
    ID3D11Texture2D* back = nullptr;
    g_swapchain->GetBuffer(0, IID_PPV_ARGS(&back));
    if (!back) return;
    g_device->CreateRenderTargetView(back, nullptr, &g_rtv);
    back->Release();
}
static void CleanupRenderTarget()
{
    if (g_rtv) { g_rtv->Release(); g_rtv = nullptr; }
}

static bool CreateDeviceD3D(HWND hwnd)
{
    DXGI_SWAP_CHAIN_DESC sd{};
    sd.BufferCount        = 2;
    sd.BufferDesc.Format  = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator   = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags              = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage        = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow       = hwnd;
    sd.SampleDesc.Count   = 1;
    sd.Windowed           = TRUE;
    sd.SwapEffect         = DXGI_SWAP_EFFECT_DISCARD;

    const D3D_FEATURE_LEVEL levels[] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0 };
    D3D_FEATURE_LEVEL got;
    if (D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0,
                                      levels, 2, D3D11_SDK_VERSION, &sd,
                                      &g_swapchain, &g_device, &got, &g_context) != S_OK)
        return false;
    CreateRenderTarget();
    return true;
}

static LRESULT WINAPI WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wp, lp))
        return true;
    switch (msg)
    {
    case WM_SIZE:
        if (g_device && wp != SIZE_MINIMIZED)
        {
            CleanupRenderTarget();
            g_swapchain->ResizeBuffers(0, LOWORD(lp), HIWORD(lp), DXGI_FORMAT_UNKNOWN, 0);
            CreateRenderTarget();
        }
        return 0;
    case WM_SYSCOMMAND:
        if ((wp & 0xfff0) == SC_KEYMENU) return 0;
        break;
    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;
    }
    return ::DefWindowProcW(hwnd, msg, wp, lp);
}

static void LoadFonts()
{
    ImGuiIO& io = ImGui::GetIO();
    ImFontConfig cfg;
    cfg.OversampleH = 2;
    cfg.OversampleV = 2;
    cfg.RasterizerMultiply = ui::theme.font.rasterizer_multiply;

    // Sizes come from the theme because they are measured, not chosen: body
    // text one or two pixels off is the difference a viewer registers as
    // "close but not it", without being able to name why.
    // Latin + U+2026. The ellipsis is outside ImGui's default range, and a
    // glyph the atlas does not carry renders as '?' — an easy detail to ship
    // broken because it only shows up on truncated text.
    static const ImWchar text_range[] = { 0x0020, 0x00FF, 0x2026, 0x2026, 0 };
    ui::fonts::text = io.Fonts->AddFontFromFileTTF("assets/fonts/Poppins-Medium.ttf",
                                                   ui::theme.font.text, &cfg, text_range);
    if (!ui::fonts::text)
        ui::fonts::text = io.Fonts->AddFontDefault();

    // Merge the icon glyphs into the text font so ICON_FA_* works inline.
#if UI_HAS_ICONS
    static const ImWchar icon_range[] = { ICON_MIN_FA, ICON_MAX_16_FA, 0 };
    ImFontConfig icfg;
    icfg.MergeMode  = true;
    icfg.PixelSnapH = true;
    icfg.GlyphMinAdvanceX = 16.f;
    io.Fonts->AddFontFromFileTTF("assets/fonts/" FONT_ICON_FILE_NAME_FAS,
                                 ui::theme.font.icon, &icfg, icon_range);
#endif

    ui::fonts::title = io.Fonts->AddFontFromFileTTF("assets/fonts/Bungee-Regular.ttf",
                                                    ui::theme.font.title, &cfg);
    if (!ui::fonts::title) ui::fonts::title = ui::fonts::text;

    ui::fonts::icon_sm = ui::fonts::text;   // icons are merged into the text font
    ui::fonts::icon    = ui::fonts::text;
}

// ─────────────────────────────────────────────────────────────────────────────
// layout — this is the part a cloned design replaces
// ─────────────────────────────────────────────────────────────────────────────
static void DrawMenu()
{
    ui::Theme& th = ui::theme;
    ImDrawList* bg = ImGui::GetBackgroundDrawList();
    const ImVec2 sz = th.window.size;

    // Panels: opaque right side, translucent sidebar (the DWM blur shows through).
    bg->AddRectFilled(ImVec2(th.window.sidebar_w, 0), sz,
                      ImGui::GetColorU32(th.window.bg), th.window.rounding,
                      ImDrawFlags_RoundCornersRight);
    bg->AddRectFilled(ImVec2(0, 0), ImVec2(th.window.sidebar_w, sz.y),
                      ImGui::GetColorU32(th.window.bg_sidebar), th.window.rounding,
                      ImDrawFlags_RoundCornersLeft);

    bg->AddLine(ImVec2(0, th.window.header_h), ImVec2(sz.x, th.window.header_h),
                ImGui::GetColorU32(th.window.border), th.window.border_size);
    bg->AddLine(ImVec2(th.window.sidebar_w, 0), ImVec2(th.window.sidebar_w, sz.y),
                ImGui::GetColorU32(th.window.border), th.window.border_size);

    // Logo.
    ImGui::PushFont(ui::fonts::title);
    const ImVec2 ts = ImGui::CalcTextSize("VISION");
    ImGui::GetWindowDrawList()->AddText(
        ImVec2((th.window.sidebar_w - ts.x) * 0.5f, (th.window.header_h - ts.y) * 0.5f),
        ImGui::GetColorU32(th.window.title), "VISION");
    ImGui::PopFont();

    // Sidebar.
    static int tab = 0;
    ImGui::SetCursorPos(ImVec2(0.f, th.window.header_h + 10.f));
    ImGui::BeginGroup();
    {
        const ImVec2 tsz(th.window.sidebar_w, th.tab.height);
        if (ui::Tab(ICON_FA_CROSSHAIRS, "Recoil",   tab == 0, tsz)) tab = 0;
        if (ui::Tab(ICON_FA_SLIDERS,    "Misc",     tab == 1, tsz)) tab = 1;
        if (ui::Tab(ICON_FA_GEAR,       "Settings", tab == 2, tsz)) tab = 2;
        if (ui::Tab(ICON_FA_FILE,       "Configs",  tab == 3, tsz)) tab = 3;
    }
    ImGui::EndGroup();

    // Cross-tab fade: alpha falls to 0, the page swaps, alpha climbs back.
    static float tab_alpha = 1.f;
    static int   active    = 0;
    ui::Approach(tab_alpha, (tab == active) ? 1.f : 0.f, th.motion.tab_fade);
    if (tab_alpha < 0.02f) active = tab;

    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, tab_alpha * ImGui::GetStyle().Alpha);

    const float pad  = th.window.content_pad;
    const float left = th.window.sidebar_w + pad;
    const float boxw = (sz.x - left - pad * 3.f) * 0.5f;
    const float boxh = sz.y - th.window.header_h - pad * 2.f;

    ImGui::SetCursorPos(ImVec2(left, th.window.header_h + pad));
    if (active == 0)
    {
        ImGui::BeginGroup();
        if (ui::BeginGroupBox("Recoil", ImVec2(boxw, boxh)))
        {
            ui::Toggle("Checkbox", &vars::toggle);
            ui::Keybind("KeyBind", &vars::key);
            ui::Separator();
            ui::SliderInt("Slider Integer", &vars::slider_i, 1, 100, "%d%%");
            ui::SliderFloat("Slider Float", &vars::slider_f, 1.f, 100.f, "%.1ff");
            ui::Separator();
            ui::InputText("##input", "InputText", vars::input, IM_ARRAYSIZE(vars::input));
        }
        ui::EndGroupBox();
        ImGui::EndGroup();

        ImGui::SameLine(0.f, pad);

        ImGui::BeginGroup();
        if (ui::BeginGroupBox("Settings", ImVec2(boxw, boxh)))
        {
            ui::ColorPicker("Color Picker", vars::color);
            ui::MultiCombo("MultiCombo", vars::multi, vars::multi_items, 5);
            ui::Combo("Combo", &vars::combo, vars::items, 3);
            ui::Separator();
            ui::Button("Button");
        }
        ui::EndGroupBox();
        ImGui::EndGroup();
    }
    else
    {
        ImGui::BeginGroup();
        if (ui::BeginGroupBox("Empty", ImVec2(boxw, boxh)))
            ui::Text("Nothing here yet.");
        ui::EndGroupBox();
        ImGui::EndGroup();
    }

    ImGui::PopStyleVar();
}

int APIENTRY wWinMain(HINSTANCE, HINSTANCE, LPWSTR, int)
{
    ui::theme = ui::themes::Vision();

    g_hwnd = ui::CreateMenuWindow(L"ui_menu_window", L"menu",
                                  static_cast<int>(ui::theme.window.size.x),
                                  static_cast<int>(ui::theme.window.size.y),
                                  WndProc);
    if (!g_hwnd)
        return 1;

    ui::ApplyWindowBlur(g_hwnd, ui::BlurMode::Blur);

    if (!CreateDeviceD3D(g_hwnd))
        return 1;

    ::ShowWindow(g_hwnd, SW_SHOWDEFAULT);
    ::UpdateWindow(g_hwnd);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    // No imgui.ini. The menu window is dragged by moving the OS window, so a
    // persisted ImGui position is re-applied as an offset on every frame and
    // the window walks across the screen until it leaves the display.
    ImGui::GetIO().IniFilename = nullptr;

    ImGui::StyleColorsDark();
    LoadFonts();

    ImGui_ImplWin32_Init(g_hwnd);
    ImGui_ImplDX11_Init(g_device, g_context);

    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowPadding    = ImVec2(0.f, 0.f);
    style.WindowBorderSize = 0.f;
    style.ItemSpacing      = ImVec2(0.f, 10.f);
    style.ScrollbarSize      = ui::theme.scrollbar.size;
    style.ScrollbarRounding  = ui::theme.scrollbar.rounding;
    style.Colors[ImGuiCol_ScrollbarBg]          = ui::theme.scrollbar.bg;
    style.Colors[ImGuiCol_ScrollbarGrab]        = ui::theme.scrollbar.grab;
    style.Colors[ImGuiCol_ScrollbarGrabHovered] = ui::theme.scrollbar.grab;
    style.Colors[ImGuiCol_ScrollbarGrabActive]  = ui::theme.scrollbar.grab;

    const ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration |
                                   ImGuiWindowFlags_NoResize |
                                   ImGuiWindowFlags_NoBackground |
                                   ImGuiWindowFlags_NoBringToFrontOnFocus;

    bool done = false;
    while (!done)
    {
        MSG msg;
        while (::PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            ::TranslateMessage(&msg);
            ::DispatchMessageW(&msg);
            if (msg.message == WM_QUIT) done = true;
        }
        if (done) break;

        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
        ui::NewFrame();

        // Live accent recolour from the in-menu picker.
        ui::theme.accent     = ImVec4(vars::color[0], vars::color[1], vars::color[2], 1.f);
        ui::theme.accent_dim = ImVec4(vars::color[0], vars::color[1], vars::color[2], 0.51f);
        ui::theme.accent_off = ImVec4(vars::color[0], vars::color[1], vars::color[2], 0.f);

        ImGui::SetNextWindowSize(ui::theme.window.size);
        ImGui::Begin("##menu", nullptr, flags);
        ui::DragWindowWithImGui(g_hwnd, ui::theme.window.size);
        DrawMenu();
        ImGui::End();

        ImGui::Render();
        const float clear[4] = { 0.f, 0.f, 0.f, 0.f };   // alpha 0 -> DWM blur shows
        g_context->OMSetRenderTargets(1, &g_rtv, nullptr);
        g_context->ClearRenderTargetView(g_rtv, clear);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
        g_swapchain->Present(1, 0);
    }

    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    CleanupRenderTarget();
    if (g_swapchain) g_swapchain->Release();
    if (g_context)   g_context->Release();
    if (g_device)    g_device->Release();
    ::DestroyWindow(g_hwnd);
    return 0;
}
