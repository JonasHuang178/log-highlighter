#pragma once
#ifndef OVERVIEW_PANEL_H
#define OVERVIEW_PANEL_H

#include <windows.h>
#include <vector>

// ---------------------------------------------------------------------------
//  OverviewPanel — Non-Client Area (NCA) approach
//
//  No separate HWND. The panel is drawn directly into the Scintilla window's
//  non-client area by subclassing the Scintilla HWND.
//
//  Key Win32 messages intercepted:
//    WM_NCCALCSIZE     — steal OVERVIEW_PANEL_WIDTH px from the right edge of
//                        Scintilla's client area so text doesn't paint over us
//    WM_NCPAINT        — draw colored marks + viewport box into the stolen strip
//    WM_NCHITTEST      — return HTBORDER when the cursor is in our strip so
//                        double-click generates WM_NCLBUTTONDBLCLK
//    WM_NCLBUTTONDBLCLK — navigate editor to the clicked line
//    WM_SETCURSOR      — show arrow cursor (not the resize cursor) in our strip
//
//  Lifecycle:
//    Init()           — subclass Scintilla; call after first valid hSci
//    Update()         — store marks, redraw NCA frame
//    UpdateViewport() — redraw NCA frame (moves viewport box)
//    Destroy()        — remove subclass, restore Scintilla's full client width
// ---------------------------------------------------------------------------

struct PanelMark {
    int      line;   // 0-based line number in document
    COLORREF color;  // RGB color for the mark (standard Windows RGB)
};

class OverviewPanel
{
public:
    OverviewPanel()  = default;
    ~OverviewPanel() = default;

    void Init(HWND hNpp, HWND hSci, HINSTANCE hInst);
    void Update(HWND hSci, const std::vector<PanelMark>& marks);
    void UpdateViewport();
    void Destroy();

    bool IsInitialized() const { return m_hSci != nullptr; }

private:
    // SetWindowSubclass callback
    static LRESULT CALLBACK SubclassProc(HWND hwnd, UINT msg,
        WPARAM wp, LPARAM lp,
        UINT_PTR uIdSubclass, DWORD_PTR dwRefData);

    // Per-message handlers
    LRESULT OnNCCalcSize    (HWND hwnd, WPARAM wp, LPARAM lp);
    LRESULT OnNCPaint       (HWND hwnd, WPARAM wp);
    LRESULT OnNCHitTest     (HWND hwnd, int xScreen, int yScreen);
    LRESULT OnNCLButtonDblClk(HWND hwnd, LPARAM lp);
    LRESULT OnSetCursor     (HWND hwnd, LPARAM lp);

    // Compute the panel rect in SCREEN coordinates from current window/client rects
    RECT GetPanelRectScreen(HWND hwnd) const;

    // Draw all panel content into hdc (in window-relative coords, rcDraw is the strip)
    void DrawPanel(HDC hdc, const RECT& rcDraw, int panelH);

    // Invalidate the NCA frame to trigger WM_NCPAINT
    void InvalidateFrame() const;

    HWND m_hNpp = nullptr;
    HWND m_hSci = nullptr;   // the subclassed Scintilla HWND

    void DoNavigation();  // deferred navigation called via WM_TIMER

    std::vector<PanelMark> m_marks;
    int m_totalLines    = 1;
    int m_visibleLines  = 30;  // cached from DrawPanel
    int m_pendingNavLine = -1; // set before PostMessage, consumed in DoNavigation
};

#endif // OVERVIEW_PANEL_H
