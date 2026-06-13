#include "Plugin.h"
#include "Parser.h"
#include "Highlighter.h"
#include "OverviewPanel.h"
#include "../config/AboutInfo.h"
#include "../config/LogPatterns.h"
#include <tchar.h>
#include <vector>

// ---------------------------------------------------------------------------
// Globals
// ---------------------------------------------------------------------------
NppData g_nppData = {};

static FuncItem    g_funcItems[2];   // 0 = Parse Log, 1 = About
static ShortcutKey g_parseLogKey;
static std::vector<Match> g_matches;

// The overview panel (right-side docked minimap)
static OverviewPanel g_overviewPanel;

// Auto-highlight via SCN_MODIFIED is inactive until the user presses Ctrl+Alt+Q
// at least once.  This prevents highlights from appearing on file open / load.
static bool g_highlightActive = false;

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
            // Convert Scintilla BGR to GDI RGB
            COLORREF bgr = rule.textColor;
            col = RGB(GetBValue(bgr), GetGValue(bgr), GetRValue(bgr));
        }
        else // STEP_TYPE
        {
            const auto& rule = STEP_TYPE_RULES[m.ruleIndex];
            show = rule.showInPanel;
            COLORREF bgr = rule.bgColor;
            col = RGB(GetBValue(bgr), GetGValue(bgr), GetRValue(bgr));
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
static void ParseLog()
{
    HWND hSci = GetCurrentScintilla();
    if (!hSci) return;

    InitStyles(hSci);
    g_matches = ParseDocument(hSci);
    ClearAllHighlights(hSci);
    ApplyHighlights(hSci, g_matches); // repaintAfter = true (default)
    g_highlightActive = true;         // enable real-time highlighting from now on

    // Update overview panel marks
    g_overviewPanel.Update(hSci, BuildPanelMarks(hSci, g_matches));
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
    return TEXT("LogHighlighter");
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

    // --- Notepad++ notification: fully initialized ---
    if (code == NPPN_READY)
    {
        g_overviewPanel.Init(g_nppData._nppHandle, g_hInstance);
        return;
    }

    switch (code)
    {
    case SCN_MODIFIED:
    {
        // SC_MOD_INSERTTEXT (0x1) and SC_MOD_DELETETEXT (0x2) are real text edits.
        // SC_MOD_CHANGEINDICATOR (0x4000) fires when indicators are applied - not a text edit.
        // Filtering by (INSERT|DELETE) prevents an infinite highlight->notify->highlight loop.
        // Do nothing until the user has run Parse Log (Ctrl+Alt+Q) at least once.
        if (!g_highlightActive)
            break;

        const int TEXT_CHANGE = SC_MOD_INSERTTEXT | SC_MOD_DELETETEXT;
        if (!(notification->modificationType & TEXT_CHANGE))
            break;

        HWND hSci = reinterpret_cast<HWND>(notification->nmhdr.hwndFrom);
        if (!hSci) break;

        InitStyles(hSci);                // ensure indicator styles are configured
        g_matches = ParseDocument(hSci); // full re-parse via direct buffer pointer (fast)
        ClearAllHighlights(hSci);
        ApplyHighlights(hSci, g_matches, /*repaintAfter=*/false); // Scintilla repaints itself

        // Update overview panel marks
        g_overviewPanel.Update(hSci, BuildPanelMarks(hSci, g_matches));
        break;
    }

    case SCN_UPDATEUI:
        // Triggered on scroll, selection change, etc. — refresh viewport indicator box.
        g_overviewPanel.UpdateViewport();
        break;

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
