#define NOMINMAX
#include "OverviewPanel.h"
#include "../config/OverviewConfig.h"
#include "../external/Scintilla.h"
#include <algorithm>
#include <windowsx.h>
#include <commctrl.h>
#pragma comment(lib, "Comctl32.lib")

// Unique subclass ID for SetWindowSubclass
static constexpr UINT_PTR kSubclassId = 0xA0B1C2D3u;

// ---------------------------------------------------------------------------
//  Helpers
// ---------------------------------------------------------------------------

// Returns the panel strip in SCREEN coordinates.
// With our NCCALCSIZE trick (subtract before default proc), the panel occupies
// the rightmost OVERVIEW_PANEL_WIDTH px of the Scintilla window rect — to the
// RIGHT of the scrollbar.
RECT OverviewPanel::GetPanelRectScreen(HWND hwnd) const
{
    RECT rcWin = {};
    ::GetWindowRect(hwnd, &rcWin);
    return {
        rcWin.right - OVERVIEW_PANEL_WIDTH,
        rcWin.top,
        rcWin.right,
        rcWin.bottom
    };
}

void OverviewPanel::InvalidateFrame() const
{
    if (m_hSci)
        ::RedrawWindow(m_hSci, nullptr, nullptr,
                       RDW_FRAME | RDW_INVALIDATE | RDW_NOERASE | RDW_NOCHILDREN);
}

// ---------------------------------------------------------------------------
//  Init — subclass Scintilla
// ---------------------------------------------------------------------------
void OverviewPanel::Init(HWND hNpp, HWND hSci, HINSTANCE /*hInst*/)
{
    if (m_hSci) return;
    m_hNpp = hNpp;
    m_hSci = hSci;

    ::SetWindowSubclass(hSci, SubclassProc, kSubclassId,
                        reinterpret_cast<DWORD_PTR>(this));

    // Trigger WM_NCCALCSIZE so Scintilla shrinks its client area immediately
    ::SetWindowPos(hSci, nullptr, 0, 0, 0, 0,
                   SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER |
                   SWP_NOACTIVATE | SWP_FRAMECHANGED);
}

// ---------------------------------------------------------------------------
//  Update
// ---------------------------------------------------------------------------
void OverviewPanel::Update(HWND hSci, const std::vector<PanelMark>& marks)
{
    m_hSci  = hSci;
    m_marks = marks;

    if (hSci)
    {
        m_totalLines = static_cast<int>(
            ::SendMessage(hSci, SCI_GETLINECOUNT, 0, 0));
        if (m_totalLines < 1) m_totalLines = 1;
    }

    InvalidateFrame();
}

// ---------------------------------------------------------------------------
//  UpdateViewport
// ---------------------------------------------------------------------------
void OverviewPanel::UpdateViewport()
{
    InvalidateFrame();
}

// ---------------------------------------------------------------------------
//  Destroy — remove subclass, restore full client width
// ---------------------------------------------------------------------------
void OverviewPanel::Destroy()
{
    if (!m_hSci) return;

    ::RemoveWindowSubclass(m_hSci, SubclassProc, kSubclassId);

    // Trigger WM_NCCALCSIZE without our hook — restores original client area
    ::SetWindowPos(m_hSci, nullptr, 0, 0, 0, 0,
                   SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER |
                   SWP_NOACTIVATE | SWP_FRAMECHANGED);

    m_hSci = nullptr;
}

// ---------------------------------------------------------------------------
//  SubclassProc — SetWindowSubclass callback
// ---------------------------------------------------------------------------
LRESULT CALLBACK OverviewPanel::SubclassProc(
    HWND hwnd, UINT msg, WPARAM wp, LPARAM lp,
    UINT_PTR /*uIdSubclass*/, DWORD_PTR dwRefData)
{
    auto* self = reinterpret_cast<OverviewPanel*>(dwRefData);

    switch (msg)
    {
    case WM_NCCALCSIZE:
        return self->OnNCCalcSize(hwnd, wp, lp);

    case WM_NCPAINT:
        return self->OnNCPaint(hwnd, wp);

    case WM_NCHITTEST:
        return self->OnNCHitTest(hwnd,
                                  GET_X_LPARAM(lp),
                                  GET_Y_LPARAM(lp));

    case WM_NCLBUTTONDOWN:
        if (wp == HTBORDER)
            return self->OnNCLButtonDblClk(hwnd, lp);
        break;

    case WM_NCLBUTTONUP:
    case WM_NCLBUTTONDBLCLK:
        if (wp == HTBORDER)
            return 0; // suppress default HTBORDER behaviour

    // Deferred navigation: fired by SetTimer so it runs after all click
    // messages finish, with no risk of conflicting with Scintilla messages.
    case WM_TIMER:
        if (wp == 0xAB42)
        {
            ::KillTimer(hwnd, 0xAB42);
            if (self) self->DoNavigation();
            return 0;
        }
        break;

    case WM_SETCURSOR:
        return self->OnSetCursor(hwnd, lp);

    case WM_NCDESTROY:
        // Scintilla is being destroyed — clean up without calling SetWindowPos
        ::RemoveWindowSubclass(hwnd, SubclassProc, kSubclassId);
        self->m_hSci = nullptr;
        break;

    default:
        break;
    }

    return ::DefSubclassProc(hwnd, msg, wp, lp);
}

// ---------------------------------------------------------------------------
//  WM_NCCALCSIZE — reserve the rightmost OVERVIEW_PANEL_WIDTH px for our panel
// ---------------------------------------------------------------------------
LRESULT OverviewPanel::OnNCCalcSize(HWND hwnd, WPARAM wp, LPARAM lp)
{
    if (wp)
    {
        // Subtract from the PROPOSED WINDOW RECT *before* calling the original
        // proc.  The original proc then lays out borders + scrollbar within the
        // (14px-narrower) space, so the scrollbar lands 14px left of the window
        // edge.  The rightmost 14px strip belongs to our panel — no overlap with
        // the scrollbar, no duplicate bars.
        auto* ncp = reinterpret_cast<NCCALCSIZE_PARAMS*>(lp);
        ncp->rgrc[0].right -= OVERVIEW_PANEL_WIDTH;
    }
    return ::DefSubclassProc(hwnd, WM_NCCALCSIZE, wp, lp);
}

// ---------------------------------------------------------------------------
//  WM_NCPAINT — draw into the stolen strip
// ---------------------------------------------------------------------------
LRESULT OverviewPanel::OnNCPaint(HWND hwnd, WPARAM wp)
{
    // Let Scintilla draw its own non-client area (border, scrollbars) first
    LRESULT result = ::DefSubclassProc(hwnd, WM_NCPAINT, wp, 0);

    // Panel position in screen coords
    RECT rcPanel = GetPanelRectScreen(hwnd);
    int  panelH  = rcPanel.bottom - rcPanel.top;
    if (panelH <= 0) return result;

    // GetWindowDC gives a DC with origin at window top-left (not client top-left)
    RECT  rcWin = {};
    ::GetWindowRect(hwnd, &rcWin);
    HDC hdc = ::GetWindowDC(hwnd);

    // Convert panel rect to window-relative coords for drawing
    RECT rcDraw = {
        rcPanel.left   - rcWin.left,
        rcPanel.top    - rcWin.top,
        rcPanel.right  - rcWin.left,
        rcPanel.bottom - rcWin.top
    };

    // Double-buffer to avoid flicker
    int  W       = rcDraw.right  - rcDraw.left;
    int  H       = rcDraw.bottom - rcDraw.top;
    HDC  hdcMem  = ::CreateCompatibleDC(hdc);
    HBITMAP hBmp    = ::CreateCompatibleBitmap(hdc, W, H);
    HBITMAP hOldBmp = static_cast<HBITMAP>(::SelectObject(hdcMem, hBmp));

    DrawPanel(hdcMem, { 0, 0, W, H }, H);

    ::BitBlt(hdc, rcDraw.left, rcDraw.top, W, H, hdcMem, 0, 0, SRCCOPY);
    ::SelectObject(hdcMem, hOldBmp);
    ::DeleteObject(hBmp);
    ::DeleteDC(hdcMem);
    ::ReleaseDC(hwnd, hdc);

    return result;
}

// ---------------------------------------------------------------------------
//  WM_NCHITTEST — return HTBORDER when cursor is inside our panel strip
// ---------------------------------------------------------------------------
LRESULT OverviewPanel::OnNCHitTest(HWND hwnd, int xScreen, int yScreen)
{
    RECT rcPanel = GetPanelRectScreen(hwnd);
    POINT pt = { xScreen, yScreen };
    if (::PtInRect(&rcPanel, pt))
        return HTBORDER;  // causes WM_NCLBUTTONDBLCLK + WM_SETCURSOR

    return ::DefSubclassProc(hwnd, WM_NCHITTEST, 0,
                              MAKELPARAM(xScreen, yScreen));
}

// ---------------------------------------------------------------------------
//  WM_NCLBUTTONDOWN in panel — store target line, post deferred navigation
// ---------------------------------------------------------------------------
LRESULT OverviewPanel::OnNCLButtonDblClk(HWND hwnd, LPARAM lp)
{
    if (!m_hSci || m_totalLines <= 0) return 0;

    RECT rcPanel = GetPanelRectScreen(hwnd);
    int  panelH  = rcPanel.bottom - rcPanel.top;
    if (panelH <= 0) return 0;

    int y    = GET_Y_LPARAM(lp) - rcPanel.top;
    int line = static_cast<int>(static_cast<double>(y) / panelH * m_totalLines);
    line = std::max(0, std::min(line, m_totalLines - 1));

    // Use SetTimer (10ms) so DoNavigation() fires after all click-message
    // processing is done. PostMessage to Scintilla is unsafe — Scintilla uses
    // WM_APP range internally and would misinterpret our message.
    m_pendingNavLine = line;
    ::SetTimer(m_hSci, 0xAB42, 10, nullptr);
    return 0;
}

// ---------------------------------------------------------------------------
//  DoNavigation — deferred, called via WM_APP+42
// ---------------------------------------------------------------------------
void OverviewPanel::DoNavigation()
{
    if (m_pendingNavLine < 0 || !m_hSci) return;
    int line = m_pendingNavLine;
    m_pendingNavLine = -1;

    // Use the cached visible-line count from DrawPanel.
    // Calling SCI_LINESONSCREEN from a WM_TIMER callback can return 1 because
    // Scintilla's layout is not fully settled at that point.  DrawPanel caches
    // the value during WM_NCPAINT (when layout IS stable) into m_visibleLines.
    int visible = (m_visibleLines > 1) ? m_visibleLines : 30;

    int targetFirst = line - visible / 2;
    int maxFirst    = std::max(0, m_totalLines - visible);
    targetFirst     = std::max(0, std::min(targetFirst, maxFirst));

    // 1. Centre the view on the target line.
    ::SendMessage(m_hSci, SCI_SETFIRSTVISIBLELINE,
                  static_cast<WPARAM>(targetFirst), 0);

    // 2. Move caret to the start of the target line, clearing any existing selection.
    //    SCI_SETEMPTYSELECTION sets anchor=caret=pos atomically (no scroll, no selection).
    WPARAM startPos = static_cast<WPARAM>(
        ::SendMessage(m_hSci, SCI_POSITIONFROMLINE, static_cast<WPARAM>(line), 0));
    ::SendMessage(m_hSci, SCI_SETEMPTYSELECTION, startPos, 0);

    // 3. Re-apply scroll position in case caret move caused any drift.
    ::SendMessage(m_hSci, SCI_SETFIRSTVISIBLELINE,
                  static_cast<WPARAM>(targetFirst), 0);
}

// ---------------------------------------------------------------------------
//  WM_SETCURSOR — show arrow cursor (not resize) when hovering our strip
// ---------------------------------------------------------------------------
LRESULT OverviewPanel::OnSetCursor(HWND hwnd, LPARAM lp)
{
    if (LOWORD(lp) == HTBORDER)
    {
        POINT pt;
        ::GetCursorPos(&pt);
        RECT rcPanel = GetPanelRectScreen(hwnd);
        if (::PtInRect(&rcPanel, pt))
        {
            ::SetCursor(::LoadCursor(nullptr, IDC_ARROW));
            return TRUE;
        }
    }
    return ::DefSubclassProc(hwnd, WM_SETCURSOR, 0, lp);
}

// ---------------------------------------------------------------------------
//  DrawPanel — all drawing logic (off-screen DC, local coords 0,0-W,H)
// ---------------------------------------------------------------------------
void OverviewPanel::DrawPanel(HDC hdc, const RECT& rcDraw, int panelH)
{
    // Background — COLOR_BTNFACE is the scrollbar track background on Windows 10/11
    HBRUSH hBg = ::CreateSolidBrush(::GetSysColor(COLOR_BTNFACE));
    ::FillRect(hdc, &rcDraw, hBg);
    ::DeleteObject(hBg);

    if (panelH <= 0 || m_totalLines <= 0) return;

    int panelW = rcDraw.right - rcDraw.left;

    // Adaptive mark height: scale down when marks are dense so total coverage
    // stays below ~50% of panel height. Never smaller than 1px.
    int markH = std::max(OVERVIEW_MARK_MIN_H, panelH / m_totalLines);
    if (!m_marks.empty())
    {
        int adaptive = static_cast<int>(panelH * 0.5 / static_cast<double>(m_marks.size()));
        markH = std::max(1, std::min(markH, adaptive));
    }

    // Draw marks with merge logic (only truly overlapping same-color marks merge)
    bool     inMark     = false;
    int      mergeTop   = 0;
    int      mergeBot   = 0;
    COLORREF mergeColor = 0;

    auto FlushMark = [&]() {
        if (!inMark) return;
        HBRUSH hBr = ::CreateSolidBrush(mergeColor);
        RECT r = { 0, mergeTop, panelW, mergeBot };
        ::FillRect(hdc, &r, hBr);
        ::DeleteObject(hBr);
        inMark = false;
    };

    for (const auto& mark : m_marks)
    {
        int line = std::max(0, std::min(mark.line, m_totalLines - 1));
        int y    = static_cast<int>(static_cast<double>(line) / m_totalLines * panelH);
        int yBot = std::min(y + markH, panelH);

        if (inMark && mark.color == mergeColor && y < mergeBot)
            mergeBot = std::max(mergeBot, yBot);
        else
        {
            FlushMark();
            inMark     = true;
            mergeTop   = y;
            mergeBot   = yBot;
            mergeColor = mark.color;
        }
    }
    FlushMark();

    // Viewport indicator box
    if (m_hSci)
    {
        int first   = static_cast<int>(::SendMessage(m_hSci, SCI_GETFIRSTVISIBLELINE, 0, 0));
        int visible = static_cast<int>(::SendMessage(m_hSci, SCI_LINESONSCREEN,       0, 0));
        if (visible < 1) visible = 1;
        m_visibleLines = visible;  // cache for use in OnDblClick

        int boxTop = static_cast<int>(static_cast<double>(first)           / m_totalLines * panelH);
        int boxBot = static_cast<int>(static_cast<double>(first + visible) / m_totalLines * panelH);
        boxTop = std::max(0, boxTop);
        boxBot = std::min(panelH, std::max(boxTop + 2, boxBot));

        // Fill viewport box background
        COLORREF vpBgColor = (OVERVIEW_VIEWPORT_BG_COLOR == CLR_NONE)
            ? ::GetSysColor(COLOR_SCROLLBAR)
            : static_cast<COLORREF>(OVERVIEW_VIEWPORT_BG_COLOR);
        HBRUSH hFillBr = ::CreateSolidBrush(vpBgColor);
        RECT   rcVp    = { 0, boxTop, panelW, boxBot };
        ::FillRect(hdc, &rcVp, hFillBr);
        ::DeleteObject(hFillBr);

        // Draw viewport box border
        HPEN   hPen    = ::CreatePen(PS_SOLID, 1, OVERVIEW_VIEWPORT_COLOR);
        HPEN   hOldPen = static_cast<HPEN>(::SelectObject(hdc, hPen));
        HBRUSH hNullBr = static_cast<HBRUSH>(::GetStockObject(NULL_BRUSH));
        HBRUSH hOldBr  = static_cast<HBRUSH>(::SelectObject(hdc, hNullBr));

        ::Rectangle(hdc, 0, boxTop, panelW - 1, boxBot);

        ::SelectObject(hdc, hOldPen);
        ::SelectObject(hdc, hOldBr);
        ::DeleteObject(hPen);
    }
}
