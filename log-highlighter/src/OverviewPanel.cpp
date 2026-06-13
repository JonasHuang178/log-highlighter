#include "OverviewPanel.h"
#include "../config/OverviewConfig.h"
#include "../external/PluginInterface.h"
#include "../external/Scintilla.h"
#include <tchar.h>
#include <algorithm>

// ---------------------------------------------------------------------------
//  Window class name (must be unique per DLL)
// ---------------------------------------------------------------------------
static const TCHAR* kWndClassName = TEXT("LogHighlighter_OverviewPanel");

// ---------------------------------------------------------------------------
//  Init — register class, create window, dock with Notepad++
// ---------------------------------------------------------------------------
void OverviewPanel::Init(HWND hNpp, HINSTANCE hInst)
{
    if (m_hwnd) return; // already initialized
    m_hNpp = hNpp;

    // --- Register window class ---
    WNDCLASSEX wc   = {};
    wc.cbSize        = sizeof(WNDCLASSEX);
    wc.style         = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
    wc.lpfnWndProc   = OverviewPanel::WndProc;
    wc.hInstance     = hInst;
    wc.hCursor       = ::LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    wc.lpszClassName = kWndClassName;
    ::RegisterClassEx(&wc); // harmless if already registered

    // --- Create panel window (child of NPP, initially hidden — NPP will show it) ---
    m_hwnd = ::CreateWindowEx(
        0,
        kWndClassName,
        TEXT("Overview"),
        WS_CHILD,
        0, 0, OVERVIEW_PANEL_WIDTH, 100,
        hNpp,
        nullptr,
        hInst,
        this); // pass 'this' so WndProc can retrieve it via WM_NCCREATE

    if (!m_hwnd) return;

    // --- Register as a docked panel (right side) ---
    tTbData tbData      = {};
    tbData.hClient       = m_hwnd;
    tbData.pszName       = TEXT("Log Overview");
    tbData.dlgID         = 0;
    tbData.uMask         = DWS_DF_CONT_RIGHT;
    tbData.hIconTab      = nullptr;
    tbData.pszAddInfo    = nullptr;
    tbData.rcFloat       = {};
    tbData.iPrevCont     = -1;
    tbData.pszModuleName = TEXT("log-highlighter.dll");

    ::SendMessage(hNpp,
                  static_cast<UINT>(NPPM_DMMREGASDCKDLG),
                  0,
                  reinterpret_cast<LPARAM>(&tbData));
}

// ---------------------------------------------------------------------------
//  Update — store marks and trigger repaint
// ---------------------------------------------------------------------------
void OverviewPanel::Update(HWND hSci, const std::vector<PanelMark>& marks)
{
    m_hSci   = hSci;
    m_marks  = marks;

    // Cache total line count (avoid repeated SendMessage on every paint)
    if (hSci)
    {
        m_totalLines = static_cast<int>(
            ::SendMessage(hSci, SCI_GETLINECOUNT, 0, 0));
        if (m_totalLines < 1) m_totalLines = 1;
    }

    if (m_hwnd)
        ::InvalidateRect(m_hwnd, nullptr, FALSE);
}

// ---------------------------------------------------------------------------
//  UpdateViewport — redraw to refresh the viewport indicator box
// ---------------------------------------------------------------------------
void OverviewPanel::UpdateViewport()
{
    if (m_hwnd)
        ::InvalidateRect(m_hwnd, nullptr, FALSE);
}

// ---------------------------------------------------------------------------
//  Destroy
// ---------------------------------------------------------------------------
void OverviewPanel::Destroy()
{
    if (m_hwnd)
    {
        ::DestroyWindow(m_hwnd);
        m_hwnd = nullptr;
    }
}

// ---------------------------------------------------------------------------
//  WndProc — static trampoline
// ---------------------------------------------------------------------------
LRESULT CALLBACK OverviewPanel::WndProc(HWND hwnd, UINT msg,
                                         WPARAM wp, LPARAM lp)
{
    OverviewPanel* self = nullptr;

    if (msg == WM_NCCREATE)
    {
        // Store 'this' pointer passed via CreateWindowEx lpParam
        auto* cs = reinterpret_cast<CREATESTRUCT*>(lp);
        self = reinterpret_cast<OverviewPanel*>(cs->lpCreateParams);
        ::SetWindowLongPtr(hwnd, GWLP_USERDATA,
                           reinterpret_cast<LONG_PTR>(self));
    }
    else
    {
        self = reinterpret_cast<OverviewPanel*>(
            ::GetWindowLongPtr(hwnd, GWLP_USERDATA));
    }

    if (self)
        return self->HandleMsg(msg, wp, lp);

    return ::DefWindowProc(hwnd, msg, wp, lp);
}

// ---------------------------------------------------------------------------
//  HandleMsg — instance-level message dispatcher
// ---------------------------------------------------------------------------
LRESULT OverviewPanel::HandleMsg(UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg)
    {
    case WM_ERASEBKGND:
        // Suppress default erase; OnPaint fills the entire area
        return 1;

    case WM_PAINT:
        OnPaint();
        return 0;

    case WM_SIZE:
        ::InvalidateRect(m_hwnd, nullptr, FALSE);
        return 0;

    case WM_LBUTTONDBLCLK:
    {
        int y = static_cast<int>(HIWORD(lp));
        OnDblClick(y);
        return 0;
    }

    default:
        break;
    }

    return ::DefWindowProc(m_hwnd, msg, wp, lp);
}

// ---------------------------------------------------------------------------
//  OnPaint — double-buffered paint
// ---------------------------------------------------------------------------
void OverviewPanel::OnPaint()
{
    PAINTSTRUCT ps;
    HDC hdcScreen = ::BeginPaint(m_hwnd, &ps);

    RECT rc;
    ::GetClientRect(m_hwnd, &rc);
    int panelW = rc.right  - rc.left;
    int panelH = rc.bottom - rc.top;

    // Create off-screen DC + bitmap
    HDC     hdcMem  = ::CreateCompatibleDC(hdcScreen);
    HBITMAP hBmp    = ::CreateCompatibleBitmap(hdcScreen, panelW, panelH);
    HBITMAP hOldBmp = static_cast<HBITMAP>(::SelectObject(hdcMem, hBmp));

    DrawContent(hdcMem, panelW, panelH);

    // Blit to screen
    ::BitBlt(hdcScreen, 0, 0, panelW, panelH, hdcMem, 0, 0, SRCCOPY);

    // Clean up
    ::SelectObject(hdcMem, hOldBmp);
    ::DeleteObject(hBmp);
    ::DeleteDC(hdcMem);

    ::EndPaint(m_hwnd, &ps);
}

// ---------------------------------------------------------------------------
//  DrawContent — all drawing logic (called with off-screen DC)
// ---------------------------------------------------------------------------
void OverviewPanel::DrawContent(HDC hdc, int panelW, int panelH)
{
    // --- Background ---
    HBRUSH hBgBrush = ::CreateSolidBrush(RGB(30, 30, 30));
    RECT rcAll = { 0, 0, panelW, panelH };
    ::FillRect(hdc, &rcAll, hBgBrush);
    ::DeleteObject(hBgBrush);

    if (panelH <= 0 || m_totalLines <= 0)
        return;

    // --- Mark height formula ---
    //   h = max(OVERVIEW_MARK_MIN_H, panelH / totalLines)
    int markH = std::max(OVERVIEW_MARK_MIN_H,
                         panelH / m_totalLines);

    // --- Draw marks with merge logic ---
    //   Consecutive marks of the same color within 2px of each other
    //   are extended rather than drawn separately.

    // Working state for merge
    bool    inMark      = false;
    int     mergeTop    = 0;
    int     mergeBot    = 0;
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
        // Clamp line to valid range
        int line = std::max(0, std::min(mark.line, m_totalLines - 1));

        int y = static_cast<int>(
            static_cast<double>(line) / m_totalLines * panelH);
        int yBottom = y + markH;
        yBottom = std::min(yBottom, panelH);

        if (inMark && mark.color == mergeColor && y <= mergeBot + 2)
        {
            // Extend existing mark
            mergeBot = std::max(mergeBot, yBottom);
        }
        else
        {
            FlushMark();
            inMark     = true;
            mergeTop   = y;
            mergeBot   = yBottom;
            mergeColor = mark.color;
        }
    }
    FlushMark();

    // --- Viewport indicator box ---
    if (m_hSci)
    {
        int firstVisible = static_cast<int>(
            ::SendMessage(m_hSci, SCI_GETFIRSTVISIBLELINE, 0, 0));
        int visibleLines = static_cast<int>(
            ::SendMessage(m_hSci, SCI_LINESONSCREEN, 0, 0));

        if (visibleLines < 1) visibleLines = 1;

        int boxTop = static_cast<int>(
            static_cast<double>(firstVisible) / m_totalLines * panelH);
        int boxBot = static_cast<int>(
            static_cast<double>(firstVisible + visibleLines) / m_totalLines * panelH);
        boxTop = std::max(0, boxTop);
        boxBot = std::min(panelH, boxBot);

        if (boxBot > boxTop)
        {
            // Semi-transparent overlay using AlphaBlend
            // For simplicity, draw a framed rect instead (no GDI+ dependency)
            HPEN hPen  = ::CreatePen(PS_SOLID, 1,
                                     OVERVIEW_VIEWPORT_COLOR);
            HPEN hOld  = static_cast<HPEN>(::SelectObject(hdc, hPen));
            HBRUSH hNullBrush = static_cast<HBRUSH>(
                ::GetStockObject(NULL_BRUSH));
            HBRUSH hOldBr = static_cast<HBRUSH>(
                ::SelectObject(hdc, hNullBrush));

            ::Rectangle(hdc, 0, boxTop, panelW - 1, boxBot);

            ::SelectObject(hdc, hOld);
            ::SelectObject(hdc, hOldBr);
            ::DeleteObject(hPen);
        }
    }
}

// ---------------------------------------------------------------------------
//  OnDblClick — navigate editor to line at pixel y
// ---------------------------------------------------------------------------
void OverviewPanel::OnDblClick(int y)
{
    if (!m_hSci || m_totalLines <= 0) return;

    RECT rc;
    ::GetClientRect(m_hwnd, &rc);
    int panelH = rc.bottom - rc.top;
    if (panelH <= 0) return;

    int targetLine = static_cast<int>(
        static_cast<double>(y) / panelH * m_totalLines);
    targetLine = std::max(0, std::min(targetLine, m_totalLines - 1));

    // Jump to line and center it in the visible area
    ::SendMessage(m_hSci, SCI_GOTOLINE,
                  static_cast<WPARAM>(targetLine), 0);
    ::SendMessage(m_hSci, SCI_SCROLLCARET, 0, 0);
}
