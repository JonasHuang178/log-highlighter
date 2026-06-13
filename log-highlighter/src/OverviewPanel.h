#pragma once
#ifndef OVERVIEW_PANEL_H
#define OVERVIEW_PANEL_H

#include <windows.h>
#include <vector>

// ---------------------------------------------------------------------------
//  OverviewPanel
//
//  A narrow docked panel on the right side of Notepad++ that shows colored
//  marks for log matches where showInPanel == true, plus a semi-transparent
//  viewport indicator box showing the currently visible region of the editor.
//
//  Lifecycle:
//    Init()           — call once from NPPN_READY handler
//    Update()         — call after every parse (ParseLog / SCN_MODIFIED)
//    UpdateViewport() — call from SCN_UPDATEUI handler
//    Destroy()        — call from DLL_PROCESS_DETACH (optional, for cleanup)
// ---------------------------------------------------------------------------

struct PanelMark {
    int      line;      // 0-based line number in the document
    COLORREF color;     // RGB color for the mark
};

class OverviewPanel
{
public:
    OverviewPanel() = default;
    ~OverviewPanel() = default;

    // Register window class, create HWND, dock via NPPM_DMMREGASDCKDLG
    void Init(HWND hNpp, HINSTANCE hInst);

    // Store new match marks and trigger a repaint
    void Update(HWND hSci, const std::vector<PanelMark>& marks);

    // Trigger a repaint to refresh the viewport indicator box position
    void UpdateViewport();

    // Clean up resources
    void Destroy();

    // Returns true after Init() succeeds
    bool IsInitialized() const { return m_hwnd != nullptr; }

private:
    // Window procedure (static trampoline → dispatches to instance method)
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);
    LRESULT HandleMsg(UINT msg, WPARAM wp, LPARAM lp);

    // Paint all content into hdc (used by double-buffered WM_PAINT)
    void OnPaint();
    void DrawContent(HDC hdc, int panelW, int panelH);

    // Double-click: navigate editor to the line at pixel y
    void OnDblClick(int y);

    // Panel HWND and parent
    HWND      m_hwnd   = nullptr;
    HWND      m_hNpp   = nullptr;
    HWND      m_hSci   = nullptr;  // current Scintilla handle (for navigation)

    // Mark data (filtered: showInPanel == true only)
    std::vector<PanelMark> m_marks;

    // Cached document total line count (updated in Update())
    int m_totalLines = 1;
};

#endif // OVERVIEW_PANEL_H
