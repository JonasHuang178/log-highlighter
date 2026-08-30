#pragma once
#include "../external/PluginInterface.h"
#include "Parser.h"
#include <vector>
#include <string>

extern NppData    g_nppData;
extern HINSTANCE  g_hInstance;

// Returns the HWND of the currently active Scintilla view.
HWND GetCurrentScintilla();

// ---------------------------------------------------------------------------
//  BufferState - per-buffer scan caches, keyed by NPP buffer ID.
//
//  Parse Log (Ctrl+Alt+Q), Next Bookmark (Ctrl+Alt+W) and Custom Report
//  (Ctrl+Alt+E) are independent commands: each fills its own cache on first use
//  and none of them requires another to have run. `stale` is set by
//  SCN_MODIFIED and is the single invalidation signal shared by all of them.
// ---------------------------------------------------------------------------
struct BufferState
{
    // Ctrl+Alt+Q — drives indicators and the Overview Panel.
    std::vector<Match> matches;
    bool               highlightActive = false;

    // Ctrl+Alt+W — 0-based Scintilla line numbers of bookmark matches.
    std::vector<int>   bookmarkLines;
    bool               bookmarksCached = false;

    // Ctrl+Alt+E — last rendered report, and which report it belongs to.
    std::wstring       reportText;
    int                reportIndex = -1;

    // Set by SCN_MODIFIED on text insert/delete.
    bool               stale = false;
};

// Returns the BufferState for the currently active NPP buffer, creating it on
// first access.
BufferState& CurrentBuffer();

// If the buffer has been edited since its caches were filled, drops the caches
// whose correctness depends on byte offsets and clears the flag.
//
// `matches` is deliberately left alone: Parse Log always rescans anyway, and
// keeping it means the Overview Panel still restores something on tab switch
// instead of going blank after an unrelated keystroke.
void InvalidateIfStale(BufferState& buf);
