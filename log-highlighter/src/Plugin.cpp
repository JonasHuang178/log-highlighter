#include "Plugin.h"
#include "Parser.h"
#include "log-highlighter.h"
#include "OverviewPanel.h"
#include "ProgressDialog.h"
#include "../config/AboutInfo.h"
#include "../config/LogPatterns.h"
#include "../external/Scintilla.h"
#include <tchar.h>
#include <vector>
#include <unordered_map>
#include <chrono>

// ---------------------------------------------------------------------------
// Globals
// ---------------------------------------------------------------------------
NppData g_nppData = {};

static FuncItem    g_funcItems[2];   // 0 = Parse Log, 1 = About
static ShortcutKey g_parseLogKey;

// The overview panel (right-side docked minimap)
static OverviewPanel g_overviewPanel;

// Per-buffer parse state. Key = NPP buffer ID (NPPM_GETCURRENTBUFFERID).
// Scintilla indicators are stored per-buffer by NPP already; this tracks
// the in-memory match list and lazy-rendering cursor for each open buffer.
struct BufferState
{
    std::vector<Match> matches;
    intptr_t           appliedByteEnd  = -1;
    bool               highlightActive = false;
};
static std::unordered_map<LRESULT, BufferState> g_bufferStates;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------
HWND GetCurrentScintilla()
{
    int currentView = 0;
    ::SendMessage(g_nppData._nppHandle,
                  NPPM_GETCURRENTSCINTILLA,
                  0,
                  reinterpret_cast<LPARAM>(&currentView));
    return (currentView == 0) ? g_nppData._scintillaMainHandle
                               : g_nppData._scintillaSecondHandle;
}

// Returns the BufferState for the currently active NPP buffer.
// Default-constructs a new entry if this buffer has never been parsed.
static BufferState& CurrentBuffer()
{
    LRESULT id = ::SendMessage(g_nppData._nppHandle,
                               NPPM_GETCURRENTBUFFERID, 0, 0);
    return g_bufferStates[id];
}

// Build PanelMark list from g_matches (only showInPanel == true rules)
static std::vector<PanelMark> BuildPanelMarks(HWND hSci,
                                               const std::vector<Match>& matches)
{
    std::vector<PanelMark> out;
    out.reserve(matches.size());

    for (const auto& m : matches)
    {
        bool   show  = false;
        COLORREF col = RGB(255, 255, 255);

        if (m.type == MatchType::LOG_TYPE)
        {
            const auto& rule = LOG_TYPE_RULES[m.ruleIndex];
            show = rule.showInPanel;
            // MAKE_BGR(r,g,b) == RGB(r,g,b) — already a standard COLORREF, use directly.
            col = rule.textColor;
        }
        else // STEP_TYPE
        {
            const auto& rule = STEP_TYPE_RULES[m.ruleIndex];
            show = rule.showInPanel;
            col = rule.bgColor;
        }

        if (!show) continue;

        int line = static_cast<int>(
            ::SendMessage(hSci, SCI_LINEFROMPOSITION,
                          static_cast<WPARAM>(m.byteOffset), 0));
        out.push_back({ line, col });
    }

    return out;
}

// ---------------------------------------------------------------------------
// Command: Parse Log  (Ctrl+Alt+Q)
// ---------------------------------------------------------------------------
static bool g_parseInProgress = false;  // re-entrancy guard

static void ParseLog()
{
    if (g_parseInProgress) return;
    g_parseInProgress = true;

    HWND hSci = GetCurrentScintilla();
    if (!hSci) { g_parseInProgress = false; return; }

    InitStyles(hSci);

    const auto t0 = std::chrono::steady_clock::now();

    // Show progress dialog. Disable the NPP window so menus / shortcuts
    // (including Ctrl+Alt+Q itself) cannot trigger re-entrant calls while
    // PeekMessage is running inside the parse loop.
    HWND hDlg = CreateProgressDialog(g_nppData._nppHandle, g_hInstance);
    ::EnableWindow(g_nppData._nppHandle, FALSE);

    BufferState& buf = CurrentBuffer();

    buf.matches = ParseDocument(hSci, [&](int cur, int total) -> bool
    {
        SetProgressLine(hDlg, cur, total);
        MSG msg;
        while (::PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
        }
        return !IsProgressCancelled(hDlg);
    });

    const bool cancelled = IsProgressCancelled(hDlg);

    if (cancelled)
    {
        ::EnableWindow(g_nppData._nppHandle, TRUE);
        ::SetForegroundWindow(g_nppData._nppHandle);
        ::DestroyWindow(hDlg);
        g_parseInProgress = false;

        ClearAllHighlights(hSci);
        if (g_overviewPanel.IsInitialized())
            g_overviewPanel.Update(hSci, {});
        buf.highlightActive = false;
        buf.appliedByteEnd  = -1;
        return;
    }

    // Phase 2: applying highlights. Keep dialog open so the user sees progress.
    g_parseInProgress = false;
    SetProgressApplying(hDlg);

    ClearAllHighlights(hSci);
    ApplyHighlights(hSci, buf.matches); // repaintAfter = true (default)
    buf.highlightActive = true;
    buf.appliedByteEnd  = -1;

    // Init AFTER ApplyHighlights so SWP_FRAMECHANGED doesn't queue a WM_SIZE
    // that fires before the indicator fill reaches the screen.
    if (!g_overviewPanel.IsInitialized())
        g_overviewPanel.Init(g_nppData._nppHandle, hSci, g_hInstance);

    g_overviewPanel.Update(hSci, BuildPanelMarks(hSci, buf.matches));

    // Show elapsed time in NPP status bar (bottom-left).
    {
        const auto t1 = std::chrono::steady_clock::now();
        const double elapsed = std::chrono::duration<double>(t1 - t0).count();
        wchar_t statusBuf[64];
        const int total_s = static_cast<int>(elapsed);
        const int hh = total_s / 3600;
        const int mm = (total_s % 3600) / 60;
        const int ss = total_s % 60;
        const int ms = static_cast<int>((elapsed - total_s) * 1000);
        if (hh > 0)
            ::swprintf_s(statusBuf, L"log-highlighter: parsed in %02d:%02d:%02d.%03d", hh, mm, ss, ms);
        else if (mm > 0)
            ::swprintf_s(statusBuf, L"log-highlighter: parsed in %02d:%02d.%03d", mm, ss, ms);
        else
            ::swprintf_s(statusBuf, L"log-highlighter: parsed in %.3f s", elapsed);
        ::SendMessage(g_nppData._nppHandle, NPPM_SETSTATUSBAR,
                      STATUSBAR_DOC_TYPE,
                      reinterpret_cast<LPARAM>(statusBuf));
    }

    // Dismiss dialog only after all work is complete.
    ::EnableWindow(g_nppData._nppHandle, TRUE);
    ::SetForegroundWindow(g_nppData._nppHandle);
    ::DestroyWindow(hDlg);
}

// ---------------------------------------------------------------------------
// Command: About
// ---------------------------------------------------------------------------
static void ShowAbout()
{
    ::MessageBoxW(g_nppData._nppHandle,
                  ABOUT_CONTENT,
                  ABOUT_TITLE,
                  MB_OK | MB_ICONINFORMATION);
}

// ---------------------------------------------------------------------------
// Notepad++ Plugin API exports
// ---------------------------------------------------------------------------
extern "C" {

__declspec(dllexport) bool isUnicode()
{
    return true;
}

__declspec(dllexport) const TCHAR* getName()
{
    return TEXT("log-highlighter");
}

__declspec(dllexport) FuncItem* getFuncsArray(int* nbF)
{
    *nbF = 2;

    // --- [0] Parse Log ---
    _tcscpy_s(g_funcItems[0]._itemName, TEXT("Parse Log"));
    g_funcItems[0]._pFunc      = ParseLog;
    g_funcItems[0]._cmdID      = 0;
    g_funcItems[0]._init2Check = false;

    // Ctrl + Alt + Q
    g_parseLogKey._isCtrl  = true;
    g_parseLogKey._isAlt   = true;
    g_parseLogKey._isShift = false;
    g_parseLogKey._key     = 'Q';
    g_funcItems[0]._pShKey = &g_parseLogKey;

    // --- [1] About ---
    _tcscpy_s(g_funcItems[1]._itemName, TEXT("About"));
    g_funcItems[1]._pFunc      = ShowAbout;
    g_funcItems[1]._cmdID      = 0;
    g_funcItems[1]._init2Check = false;
    g_funcItems[1]._pShKey     = nullptr;  // no shortcut key

    return g_funcItems;
}

__declspec(dllexport) void setInfo(NppData notepadPlusData)
{
    g_nppData = notepadPlusData;
}

__declspec(dllexport) void beNotified(SCNotification* notification)
{
    if (!notification) return;

    const UINT code = notification->nmhdr.code;

    switch (code)
    {
    case SCN_UPDATEUI:
        // Triggered on scroll, selection change, etc. — refresh viewport indicator box.
        g_overviewPanel.UpdateViewport();
        break;

    case NPPN_BUFFERACTIVATED:
    {
        // User switched to a different tab. Restore the Overview Panel to reflect
        // whichever buffer is now active. No re-parse — Scintilla already holds
        // the correct indicator state for each buffer.
        LRESULT id = static_cast<LRESULT>(notification->nmhdr.idFrom);
        HWND hSci  = GetCurrentScintilla();
        if (!hSci) break;

        auto it = g_bufferStates.find(id);
        if (it != g_bufferStates.end() && it->second.highlightActive)
            g_overviewPanel.Update(hSci, BuildPanelMarks(hSci, it->second.matches));
        else if (g_overviewPanel.IsInitialized())
            g_overviewPanel.Update(hSci, {});
        break;
    }

    case NPPN_FILEBEFORECLOSE:
    {
        // Buffer is about to be closed — free its match list.
        LRESULT id = static_cast<LRESULT>(notification->nmhdr.idFrom);
        g_bufferStates.erase(id);
        break;
    }

    default:
        break;
    }
}

__declspec(dllexport) LRESULT messageProc(UINT    /*Message*/,
                                           WPARAM  /*wParam*/,
                                           LPARAM  /*lParam*/)
{
    return TRUE;
}

} // extern "C"
