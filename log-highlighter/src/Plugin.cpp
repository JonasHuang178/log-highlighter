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
#include <thread>
#include <atomic>
#include <chrono>

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

// Lazy indicator state: indicators have been applied up to (but not including)
// this byte offset. Extended by SCN_UPDATEUI as the user scrolls.
static intptr_t g_appliedByteEnd = -1;

// How many lines ahead of the visible viewport to pre-apply indicators.
static constexpr int APPLY_LOOKAHEAD_LINES = 1000;

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
// Helpers for lazy indicator application
// ---------------------------------------------------------------------------

// Returns the byte position at the end of (firstVisibleLine + linesOnScreen
// + APPLY_LOOKAHEAD_LINES), clamped to the last line in the document.
static intptr_t GetLookaheadByteEnd(HWND hSci)
{
    const int firstLine    = static_cast<int>(::SendMessage(hSci, SCI_GETFIRSTVISIBLELINE, 0, 0));
    const int linesOnScr   = static_cast<int>(::SendMessage(hSci, SCI_LINESONSCREEN, 0, 0));
    const int totalLines   = static_cast<int>(::SendMessage(hSci, SCI_GETLINECOUNT, 0, 0));
    const int endLine      = min(firstLine + linesOnScr + APPLY_LOOKAHEAD_LINES, totalLines - 1);
    return static_cast<intptr_t>(::SendMessage(hSci, SCI_GETLINEENDPOSITION, endLine, 0));
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

    const int totalLines = static_cast<int>(
        ::SendMessage(hSci, SCI_GETLINECOUNT, 0, 0));

    // Only show a progress dialog for large files. For small files the parse
    // completes in microseconds — showing and immediately destroying a dialog
    // just produces an annoying flash with no useful information.
    static constexpr int PROGRESS_MIN_LINES = 5000;
    const bool showProgress = (totalLines >= PROGRESS_MIN_LINES);

    HWND hDlg = nullptr;
    bool cancelled = false;

    if (showProgress)
    {
        // Snapshot the Scintilla buffer on the UI thread before creating any
        // other windows, so all three Scintilla calls are atomic (nothing can
        // be dispatched between them).
        const intptr_t docLen = static_cast<intptr_t>(
            ::SendMessage(hSci, SCI_GETLENGTH, 0, 0));
        const char* rawPtr = reinterpret_cast<const char*>(
            ::SendMessage(hSci, SCI_GETCHARACTERPOINTER, 0, 0));
        std::vector<char> docBuf(rawPtr, rawPtr + docLen);

        hDlg = CreateProgressDialog(g_nppData._nppHandle, g_hInstance);
        SetProgressLine(hDlg, 0, totalLines);
        ::UpdateWindow(hDlg);
        ::EnableWindow(g_nppData._nppHandle, FALSE);
        ::SetForegroundWindow(hDlg);

        // Parse on a worker thread so the UI thread stays in its message loop
        // and the progress dialog remains alive and responsive throughout.
        // The worker only reads docBuf — it never calls SendMessage to Scintilla.
        std::vector<Match> workerMatches;
        std::atomic<bool>  workerCancelled { false };
        HANDLE hDone = ::CreateEventW(nullptr, TRUE, FALSE, nullptr);

        std::thread worker([&]() {
            // Parse the pre-snapshotted buffer — never touches Scintilla.
            workerMatches = ParseDocument(docBuf, totalLines,
                [&](int cur, int total) -> bool {
                    ::PostMessage(hDlg, WM_APP, static_cast<WPARAM>(cur),
                                  static_cast<LPARAM>(total));
                    return !workerCancelled.load();
                });
            ::SetEvent(hDone);
        });

        // UI thread message loop — keeps dialog painted and Cancel responsive.
        MSG msg;
        while (::MsgWaitForMultipleObjects(1, &hDone, FALSE, INFINITE, QS_ALLINPUT)
               != WAIT_OBJECT_0)
        {
            while (::PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
            {
                if (msg.hwnd == hDlg && msg.message == WM_APP)
                    SetProgressLine(hDlg,
                                    static_cast<int>(msg.wParam),
                                    static_cast<int>(msg.lParam));
                else
                {
                    ::TranslateMessage(&msg);
                    ::DispatchMessage(&msg);
                }
                if (IsProgressCancelled(hDlg))
                    workerCancelled.store(true);
            }
        }
        while (::PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
        }

        worker.join();
        ::CloseHandle(hDone);

        cancelled = IsProgressCancelled(hDlg) || workerCancelled.load();
        g_matches = std::move(workerMatches);

        ::EnableWindow(g_nppData._nppHandle, TRUE);
        ::SetForegroundWindow(g_nppData._nppHandle);
        ::DestroyWindow(hDlg);
    }
    else
    {
        g_matches = ParseDocument(hSci);
    }

    g_parseInProgress = false;

    if (cancelled)
    {
        // Leave the document in its original state.
        ClearAllHighlights(hSci);
        g_appliedByteEnd = -1;
        if (g_overviewPanel.IsInitialized())
            g_overviewPanel.Update(hSci, {});
        g_highlightActive = false;
        return;
    }

    ClearAllHighlights(hSci);
    g_appliedByteEnd = -1;

    // Apply only the visible viewport + lookahead. The rest is deferred to
    // SCN_UPDATEUI so Ctrl+Alt+Q returns instantly regardless of file size.
    const intptr_t applyEnd = GetLookaheadByteEnd(hSci);
    ApplyHighlightsInRange(hSci, g_matches, 0, applyEnd + 1);
    g_appliedByteEnd  = applyEnd;
    g_highlightActive = true;

    if (!g_overviewPanel.IsInitialized())
        g_overviewPanel.Init(g_nppData._nppHandle, hSci, g_hInstance);
    g_overviewPanel.Update(hSci, BuildPanelMarks(hSci, g_matches));

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
        g_appliedByteEnd = static_cast<intptr_t>(::SendMessage(hSci, SCI_GETLENGTH, 0, 0));

        // Update overview panel marks
        g_overviewPanel.Update(hSci, BuildPanelMarks(hSci, g_matches));
        break;
    }

    case SCN_UPDATEUI:
        // Triggered on scroll, selection change, etc. — refresh viewport indicator box.
        g_overviewPanel.UpdateViewport();

        // Extend lazy indicator coverage to keep the viewport highlighted as user scrolls.
        if (g_highlightActive && !g_matches.empty())
        {
            HWND hSci2 = reinterpret_cast<HWND>(notification->nmhdr.hwndFrom);
            if (hSci2)
            {
                const intptr_t needed = GetLookaheadByteEnd(hSci2);
                if (needed > g_appliedByteEnd)
                {
                    ApplyHighlightsInRange(hSci2, g_matches,
                                           g_appliedByteEnd + 1, needed + 1,
                                           /*repaintAfter=*/false);
                    g_appliedByteEnd = needed;
                }
            }
        }
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
