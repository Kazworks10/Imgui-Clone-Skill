// ui/themes/vision.h — worked example: a theme table measured off a real
// screenshot with scripts/extract_palette.py and scripts/crop_zoom.py.
// Every value below is a measurement, not a guess. A cloned design is exactly
// this file plus a layout function.
#pragma once
#include "../theme.h"

namespace ui::themes {

inline Theme Vision()
{
    Theme t;
    t.name = "vision";

    t.accent     = ImColor(230,  99, 240, 255);   // probed #E663F0
    t.accent_dim = ImColor(230,  99, 240, 130);
    t.accent_off = ImColor(230,  99, 240,   0);

    t.window.size        = ImVec2(800, 405);
    t.window.bg          = ImColor(20, 20, 20, 255);   // #141414
    t.window.bg_sidebar  = ImColor(20, 20, 20, 170);
    t.window.border      = ImColor(33, 32, 35, 255);   // #212023
    t.window.rounding    = 5.f;
    t.window.sidebar_w   = 155.f;
    t.window.header_h    = 60.f;                       // divider probed at y=60

    t.tab.text_active    = ImColor(255, 255, 255, 255);
    t.tab.text_inactive  = ImColor(140, 140, 140, 255);
    t.tab.height         = 40.f;

    // Panel top probed at y=70, accent rule at y=101 -> header 31px.
    // Body samples #151515 over a #141414 window: the panel colour at alpha 40.
    t.child.bg_header    = ImColor(26, 25, 28, 255);   // #1A191C
    t.child.bg_body      = ImColor(26, 25, 28,  40);
    t.child.border       = ImColor(33, 32, 35, 255);
    t.child.header_h     = 31.f;
    t.child.header_line_accent = true;                 // probed #7C3D82 = 50% accent
    t.child.header_line_alpha  = 0.5f;

    t.text.active        = ImColor(255, 255, 255, 255);
    t.text.hover         = ImColor(170, 170, 170, 255);
    t.text.idle          = ImColor(120, 120, 120, 255);

    // Toggle: track x 404..462 (58x15), knob 38x21 ringed 1px, travel 20.
    t.toggle.track       = ImColor(26, 25, 28, 255);
    t.toggle.knob_fill   = ImColor(33, 32, 35, 255);   // #212023
    t.toggle.knob_ring_off = ImColor(52, 52, 52, 255);
    t.toggle.track_w     = 58.f;
    t.toggle.track_h     = 15.f;
    t.toggle.knob_w      = 38.f;
    t.toggle.knob_h      = 21.f;
    t.toggle.travel      = 20.f;

    // Slider: track y 240..249 (10px), knob 32x19 ringed, accent fill to knob.
    t.slider.track       = ImColor(26, 25, 28, 255);
    t.slider.knob_fill   = ImColor(33, 32, 35, 255);
    t.slider.track_h     = 10.f;
    t.slider.knob_w      = 32.f;
    t.slider.knob_h      = 19.f;
    t.slider.fill_track  = true;

    t.frame.bg           = ImColor(26, 25, 28, 255);
    t.frame.bg_hover     = ImColor(32, 31, 35, 255);
    t.frame.border       = ImColor(33, 32, 35, 255);
    t.frame.height       = 35.f;                       // input probed y 339..373

    // Preview shows 3 of 5 ticked items -> the design caps by count, and the
    // marker is three ASCII dots rather than a U+2026 glyph.
    t.combo.max_preview_items = 3;
    t.combo.ellipsis          = "...";

    t.caret.width        = 16.f;                       // bars probed at y=196/200/204
    t.caret.gap          = 4.f;
    t.caret.thickness    = 1.5f;

    t.swatch.w           = 26.f;                       // probed x 750..776
    t.swatch.h           = 26.f;                       //        y 117..143
    t.swatch.rounding    = 5.f;

    t.popup.bg           = ImColor(21, 20, 23, 255);
    t.separator.line     = ImColor(30, 29, 32, 255);   // probed #1E1D20

    t.font.text          = 20.f;                       // cap height measured
    t.font.title         = 25.f;
    t.font.icon          = 17.f;

    return t;
}

} // namespace ui::themes
