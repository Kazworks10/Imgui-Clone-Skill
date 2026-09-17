// ui/theme.h — the single source of truth for every colour and metric.
//
// HARD RULE: widget code never contains a colour literal. Cloning a new
// design = writing a new Theme in ui/themes/, never touching ui/widgets.cpp.
#pragma once
#include "imgui.h"

namespace ui {

struct Theme
{
    const char* name = "unnamed";

    // Accent — usually re-patched every frame from the in-menu colour picker.
    ImVec4 accent     = ImColor(230, 99, 240, 255);
    ImVec4 accent_dim = ImColor(230, 99, 240, 130);
    ImVec4 accent_off = ImColor(230, 99, 240,   0);

    struct Window {
        ImVec2 size        = ImVec2(800, 405);
        ImVec4 bg          = ImColor(20, 20, 20, 255);
        ImVec4 bg_sidebar  = ImColor(20, 20, 20, 170);
        ImVec4 border      = ImColor(33, 32, 35, 255);
        ImVec4 title       = ImColor(255, 255, 255, 255);
        float  rounding    = 5.f;
        float  border_size = 1.2f;
        float  sidebar_w   = 155.f;
        float  header_h    = 60.f;
        float  content_pad = 10.f;
    } window;

    struct Tab {
        ImVec4 text_active   = ImColor(255, 255, 255, 255);
        ImVec4 text_inactive = ImColor(140, 140, 140, 255);
        float  height        = 40.f;
        float  bar_w         = 2.f;   // accent bar on the selected edge
        float  icon_x        = 15.f;
        float  label_x       = 45.f;
    } tab;

    // A panel is a solid header strip over a near-transparent body: the body
    // fill is the panel colour at very low alpha, so the window (and the blur
    // behind it) still reads through. Filling the body opaque is the single
    // most common way to lose a design's depth.
    struct Child {
        ImVec4 bg_header    = ImColor(26, 25, 28, 255);
        ImVec4 bg_body      = ImColor(26, 25, 28,  40);
        ImVec4 border       = ImColor(33, 32, 35, 255);
        ImVec4 header_text  = ImColor(255, 255, 255, 255);
        float  rounding     = 5.f;
        float  border_size  = 1.2f;
        float  header_h     = 31.f;
        float  pad          = 12.f;
        // The rule under the header title is frequently accent-tinted rather
        // than a neutral border. Look before assuming grey.
        bool   header_line_accent = true;
        float  header_line_alpha  = 0.5f;
    } child;

    struct Text {
        ImVec4 active = ImColor(255, 255, 255, 255);
        ImVec4 hover  = ImColor(170, 170, 170, 255);
        ImVec4 idle   = ImColor(120, 120, 120, 255);
        ImVec4 faint  = ImColor(255, 255, 255, 100);
    } text;

    // Ring-style knob: the knob is a 1px accent outline over a dark fill, and
    // it is TALLER than its track, overhanging it. Do not assume the iOS
    // pattern (accent-filled track + solid white knob) — check the image.
    struct Toggle {
        ImVec4 track        = ImColor(26, 25, 28, 255);
        ImVec4 track_border = ImColor(33, 32, 35, 255);
        ImVec4 knob_fill    = ImColor(33, 32, 35, 255);
        ImVec4 knob_ring_off= ImColor(52, 52, 52, 255);
        float  track_w      = 58.f;
        float  track_h      = 15.f;
        float  knob_w       = 38.f;
        float  knob_h       = 21.f;
        float  travel       = 20.f;   // knob slide distance
        float  ring         = 1.f;
        float  rounding     = 15.f;
    } toggle;

    struct Slider {
        ImVec4 track        = ImColor(26, 25, 28, 255);
        ImVec4 track_border = ImColor(33, 32, 35, 255);
        ImVec4 knob_fill    = ImColor(33, 32, 35, 255);
        float  track_h      = 10.f;
        float  knob_w       = 32.f;
        float  knob_h       = 19.f;
        float  ring         = 1.f;
        float  rounding     = 25.f;
        float  label_gap    = 3.f;
        bool   fill_track   = true;   // accent fill from track start to knob
    } slider;

    struct Frame {   // shared by button / input / combo / keybind boxes
        ImVec4 bg        = ImColor(26, 25, 28, 255);
        ImVec4 bg_hover  = ImColor(32, 31, 35, 255);
        ImVec4 border    = ImColor(33, 32, 35, 255);
        float  rounding  = 5.f;
        float  border_sz = 1.2f;
        float  height    = 35.f;
        float  label_gap = 8.f;   // gap between a label and the box under it
        float  text_pad  = 12.f;
    } frame;

    // How a multi-select summarises its selection. Read this off the image:
    // a preview showing three items while five are ticked means the design
    // truncates by item count, not by available width.
    struct Combo {
        int         max_preview_items = 0;                 // 0 = truncate on width
        const char* ellipsis          = "\xE2\x80\xA6";    // or "..."
    } combo;

    struct Caret {   // the three-bar affordance on combo boxes
        float width     = 16.f;
        float gap       = 4.f;
        float thickness = 1.5f;
        float inset     = 12.f;   // from the right edge of the frame
    } caret;

    struct Swatch {  // colour-picker preview
        float w        = 26.f;
        float h        = 26.f;
        float rounding = 5.f;
    } swatch;

    struct Popup {
        ImVec4 bg       = ImColor(21, 20, 23, 255);
        ImVec4 border   = ImColor(33, 32, 35, 255);
        ImVec4 hovered  = ImColor(32, 31, 35, 255);
        float  rounding = 5.f;
        float  item_h   = 28.f;
    } popup;

    struct Scrollbar {
        ImVec4 bg       = ImColor(0, 0, 0, 0);
        ImVec4 grab     = ImColor(33, 32, 35, 255);
        float  size     = 7.f;
        float  rounding = 3.f;
        bool   visible  = true;
    } scrollbar;

    struct Separator {
        ImVec4 line   = ImColor(30, 29, 32, 255);
        float  height = 14.f;   // vertical space consumed
    } separator;

    struct Shadow {
        ImVec4 color     = ImColor(0, 0, 0, 90);
        float  thickness = 18.f;
        int    steps     = 6;
    } shadow;

    struct Font {
        float text  = 20.f;   // measure this from the image; do not guess
        float title = 25.f;
        float icon  = 17.f;
        // ImGui's default stb rasteriser renders noticeably lighter and softer
        // than FreeType, which most real menus use. Nudging this recovers the
        // perceived weight without adding a dependency; build with
        // -DUI_USE_FREETYPE=ON for an exact match.
        float rasterizer_multiply = 1.15f;
    } font;

    struct Motion {
        float speed        = 9.f;   // generic ImLerp speed (colour + position)
        float speed_slow   = 6.f;   // text fades
        float speed_fast   = 16.f;  // knob / indicator travel
        float tab_fade     = 15.f;  // cross-tab alpha fade
    } motion;
};

// The live theme. Generated designs assign to this at startup.
inline Theme theme;

// Fonts owned by the shell; widgets read them, never load them.
namespace fonts {
    inline ImFont* text  = nullptr;
    inline ImFont* title = nullptr;
    inline ImFont* icon  = nullptr;
    inline ImFont* icon_sm = nullptr;
}

} // namespace ui
