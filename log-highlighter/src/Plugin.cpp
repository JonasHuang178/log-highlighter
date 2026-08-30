#include "Plugin.h"
#include "Parser.h"
#include "Report.h"
#include "DebugConsole.h"
#include "log-highlighter.h"
#include "OverviewPanel.h"
#include "ProgressDialog.h"
#include "../config/AboutInfo.h"
#include "../config/LogPatterns.h"
#include "../external/Scintilla.h"
#include <tchar.h>
#include <vector>
#include <unordered_map>
#include <utility>
#include <chrono>

// ---------------------------------------------------------------------------
// Globals
// ---------------------------------------------------------------------------
NppData g_nppData = {};

// Menu layout: Parse Log, Next Bookmark, one entry per registered report, About.
// Sized at runtime because the report table lives in config/CustomReports.h,
// which only Report.cpp is allowed to include.
static std::vector<FuncItem>    g_funcItems;
static std::vector<ShortcutKey> g_shortcutKeys;

// The overview panel (right-side docked minimap)
static OverviewPanel g_overviewPanel;

// Per-buffer scan caches. Key = NPP buffer ID (NPPM_GETCURRENTBUFFERID).
// Scintilla indicators are stored per-buffer by NPP already; this tracks the
// in-memory results for each open buffer so the Overview Panel can be restored
// on tab switch, and so bookmark navigation and reports do not rescan on every
// keypress. BufferState itself is declared in Plugin.h — Report.cpp shares it.
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
BufferState& CurrentBuffer()
{
    LRESULT id = ::SendMessage(g_nppData._nppHandle,
                               NPPM_GETCURRENTBUFFERID, 0, 0);
    return g_bufferStates[id];
}

// Drops the caches whose correctness depends on byte offsets staying valid.
//
// `matches` and `highlightActive` are left alone on purpose: Parse Log always
// rescans, and keeping them means the Overview Panel still restores marks on
// tab switch rather than going blank after an unrelated keystroke. The marks
// may be slightly out of date, which is the same trade the editor's own
// indicators make.
void InvalidateIfStale(BufferState& buf)
{
    if (!buf.stale) return;

    buf.bookmarkLines.clear();
    buf.bookmarksCached = false;

    buf.reportText.clear();
    buf.reportIndex = -1;

    buf.stale = false;
}

// Marks every tracked buffer stale and returns how many actually transitioned
// from not-stale to stale.
//
// SCN_MODIFIED carries no buffer id, so the handler cannot tell which document
// changed — it can only guess the active one or cover everything. Guessing is
// wrong precisely when it matters (a modification that did not go through the
// active buffer, as with Replace All in All Opened Documents) and wrong
// silently, leaving that buffer serving a cache built from older content.
// Covering everything only costs a rescan on next use.
//
// Buffers with no entry are deliberately not created here: no entry means no
// cache to invalidate, and they scan fresh on first use anyway.
static int MarkAllBuffersStale()
{
    int transitioned = 0;

    for (auto& entry : g_bufferStates)
    {
        if (!entry.second.stale)
        {
            entry.second.stale = true;
            ++transitioned;
        }
    }

    return transitioned;
}

// Collects the 0-based lines carrying a BOOKMARK match, in document order.
static std::vector<int> BookmarkLinesFrom(HWND hSci,
                                           const std::vector<Match>& matches)
{
    std::vector<int> lines;

    for (const auto& m : matches)
    {
        if (m.type != MatchType::BOOKMARK) continue;

        int line = static_cast<int>(
            ::SendMessage(hSci, SCI_LINEFROMPOSITION,
                          static_cast<WPARAM>(m.byteOffset), 0));
        if (lines.empty() || lines.back() != line)
            lines.push_back(line);
    }

    return lines;
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
            col = rule.textColor;
        }
        else if (m.type == MatchType::BOOKMARK)
        {
            const auto& rule = BOOKMARK_RULES[m.ruleIndex];
            show = rule.showInPanel;
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
    InvalidateIfStale(buf);

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
        return;
    }

    // Phase 2: applying highlights. Keep dialog open so the user sees progress.
    g_parseInProgress = false;
    SetProgressApplying(hDlg);

    ClearAllHighlights(hSci);
    ApplyHighlights(hSci, buf.matches);
    buf.highlightActive = true;

    // Free side effect: this scan already visited every BOOKMARK hit, so fill
    // the navigation cache too. This creates no dependency in either direction —
    // Next Bookmark scans for itself whenever the cache is empty.
    buf.bookmarkLines   = BookmarkLinesFrom(hSci, buf.matches);
    buf.bookmarksCached = true;

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
// Command: Next Bookmark  (Ctrl+Alt+W)
// ---------------------------------------------------------------------------
static constexpr UINT_PTR kBookmarkTimerId = 0xAB43;
static int  g_pendingBookmarkNavLine = -1;

static void CALLBACK BookmarkTimerProc(HWND hwnd, UINT, UINT_PTR id, DWORD)
{
    ::KillTimer(hwnd, id);
    if (g_pendingBookmarkNavLine < 0) return;

    int line = g_pendingBookmarkNavLine;
    g_pendingBookmarkNavLine = -1;

    int visible = static_cast<int>(
        ::SendMessage(hwnd, SCI_LINESONSCREEN, 0, 0));
    if (visible <= 1) visible = 30;

    int firstVisible = line - visible / 2;
    if (firstVisible < 0) firstVisible = 0;

    ::SendMessage(hwnd, SCI_SETFIRSTVISIBLELINE,
                  static_cast<WPARAM>(firstVisible), 0);

    intptr_t pos = ::SendMessage(hwnd, SCI_POSITIONFROMLINE,
                                 static_cast<WPARAM>(line), 0);
    ::SendMessage(hwnd, SCI_SETEMPTYSELECTION,
                  static_cast<WPARAM>(pos), 0);

    ::SendMessage(hwnd, SCI_SETFIRSTVISIBLELINE,
                  static_cast<WPARAM>(firstVisible), 0);
}

static void NextBookmark()
{
    HWND hSci = GetCurrentScintilla();
    if (!hSci) return;

    BufferState& buf = CurrentBuffer();
    InvalidateIfStale(buf);

    // Scan for ourselves when the cache is empty. Parse Log is not a
    // precondition — it merely fills this cache early when it happens to run.
    //
    // The scan goes through ParseDocument rather than a bookmark-only pass:
    // Aho-Corasick costs the same regardless of pattern count, so filtering
    // afterwards is free. No progress dialog — repeated presses hit the cache,
    // so the scan happens at most once per edit.
    if (!buf.bookmarksCached)
    {
        buf.bookmarkLines   = BookmarkLinesFrom(hSci, ParseDocument(hSci));
        buf.bookmarksCached = true;
    }

    const std::vector<int>& startLines = buf.bookmarkLines;

    if (startLines.empty())
    {
        const wchar_t* msg = L"log-highlighter: no Bookmark matches found.";
        ::SendMessage(g_nppData._nppHandle, NPPM_SETSTATUSBAR,
                      STATUSBAR_DOC_TYPE, reinterpret_cast<LPARAM>(msg));
        return;
    }

    int caretPos  = static_cast<int>(::SendMessage(hSci, SCI_GETCURRENTPOS, 0, 0));
    int caretLine = static_cast<int>(::SendMessage(hSci, SCI_LINEFROMPOSITION,
                                                    static_cast<WPARAM>(caretPos), 0));

    // Find next bookmark after caret
    int targetLine = startLines[0]; // default: wrap to first
    for (int line : startLines)
    {
        if (line > caretLine)
        {
            targetLine = line;
            break;
        }
    }

    g_pendingBookmarkNavLine = targetLine;
    ::SetTimer(hSci, kBookmarkTimerId, 10, BookmarkTimerProc);
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
// Command dispatch for custom reports
//
// A Notepad++ command callback takes no arguments, so every report needs its
// own distinct function pointer. These thunks are generated at compile time.
// The cap limits only how many reports can be registered, not how they are
// written; raise it here if CUSTOM_REPORTS[] ever outgrows it.
// ---------------------------------------------------------------------------
static constexpr int kMaxReports = 16;

template <int N>
static void ReportThunk() { RunCustomReport(N); }

template <int... Is>
static void FillReportThunks(PFUNCPLUGINCMD*                 out,
                             std::integer_sequence<int, Is...>)
{
    ((out[Is] = &ReportThunk<Is>), ...);
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
    static PFUNCPLUGINCMD thunks[kMaxReports] = {};
    FillReportThunks(thunks, std::make_integer_sequence<int, kMaxReports>{});

    int reportCount = CustomReportCount();
    if (reportCount > kMaxReports) reportCount = kMaxReports;

    const int total = 2 + reportCount + 1;   // Parse Log, Next Bookmark, ..., About

    // Sized once, before any _pShKey pointer is taken — a later reallocation
    // would dangle every shortcut Notepad++ is holding.
    g_funcItems.assign(static_cast<size_t>(total), FuncItem{});
    g_shortcutKeys.assign(static_cast<size_t>(total), ShortcutKey{});

    const auto bindShortcut = [](int slot, UCHAR key)
    {
        g_shortcutKeys[slot]._isCtrl  = true;
        g_shortcutKeys[slot]._isAlt   = true;
        g_shortcutKeys[slot]._isShift = false;
        g_shortcutKeys[slot]._key     = key;
        g_funcItems[slot]._pShKey     = &g_shortcutKeys[slot];
    };

    int i = 0;

    // --- Parse Log (Ctrl+Alt+Q) ---
    _tcscpy_s(g_funcItems[i]._itemName, TEXT("Parse Log"));
    g_funcItems[i]._pFunc      = ParseLog;
    g_funcItems[i]._cmdID      = 0;
    g_funcItems[i]._init2Check = false;
    bindShortcut(i, 'Q');
    ++i;

    // --- Next Bookmark (Ctrl+Alt+W) ---
    _tcscpy_s(g_funcItems[i]._itemName, TEXT("Next Bookmark"));
    g_funcItems[i]._pFunc      = NextBookmark;
    g_funcItems[i]._cmdID      = 0;
    g_funcItems[i]._init2Check = false;
    bindShortcut(i, 'W');
    ++i;

    // --- One entry per report registered in config/CustomReports.h ---
    for (int r = 0; r < reportCount; ++r, ++i)
    {
        _tcsncpy_s(g_funcItems[i]._itemName, MENU_ITEM_MAX_LENGTH,
                   CustomReportTitle(r), _TRUNCATE);
        g_funcItems[i]._pFunc      = thunks[r];
        g_funcItems[i]._cmdID      = 0;
        g_funcItems[i]._init2Check = false;

        const char shortcut = CustomReportShortcut(r);
        if (shortcut)
            bindShortcut(i, static_cast<UCHAR>(shortcut));
        else
            g_funcItems[i]._pShKey = nullptr;   // assignable via Shortcut Mapper
    }

    // --- About ---
    _tcscpy_s(g_funcItems[i]._itemName, TEXT("About"));
    g_funcItems[i]._pFunc      = ShowAbout;
    g_funcItems[i]._cmdID      = 0;
    g_funcItems[i]._init2Check = false;
    g_funcItems[i]._pShKey     = nullptr;  // no shortcut key

    *nbF = total;
    return g_funcItems.data();
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

    case SCN_MODIFIED:
        // The document changed, so every cached byte offset for this buffer is
        // now suspect. Flag it and nothing else — no parse, no scan, no report
        // is ever started from a notification.
        if (notification->modificationType &
            (SC_MOD_INSERTTEXT | SC_MOD_DELETETEXT))
        {
            const int marked = MarkAllBuffersStale();

            // Log the transition only. SCN_MODIFIED fires on every keystroke;
            // once everything is already stale there is nothing new to say, so
            // continued typing stays silent until something rescans.
            if (marked > 0 && ReportDebugEnabled())
            {
                wchar_t path[MAX_PATH * 2] = { 0 };
                ::SendMessage(g_nppData._nppHandle, NPPM_GETFULLCURRENTPATH,
                              static_cast<WPARAM>(MAX_PATH * 2 - 1),
                              reinterpret_cast<LPARAM>(path));
                path[MAX_PATH * 2 - 1] = L'\0';

                const wchar_t* name = ::wcsrchr(path, L'\\');
                name = name ? name + 1 : path;

                wchar_t line[MAX_PATH + 96];
                ::swprintf_s(line,
                             L"SCN_MODIFIED  %s  -> %d buffer(s) marked stale",
                             *name ? name : L"(untitled)", marked);
                ReportEngineLogW(line);
            }
        }
        break;

    case NPPN_READY:
        // Earliest point at which Notepad++ is fully initialised. The console
        // must not be allocated from DllMain, which runs under the loader lock.
        if (ReportDebugEnabled())
        {
            OpenDebugConsole();
            ReportEngineLog("debug console ready (REPORT_DEBUG_MODE is on)");
        }
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
