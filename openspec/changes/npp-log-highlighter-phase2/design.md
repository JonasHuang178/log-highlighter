## Context

Phase 1 delivers keyword highlighting inside the Scintilla editor via the Indicator API. `g_matches` (a `std::vector<Match>`) is already computed on every Ctrl+Alt+Q and on every `SCN_MODIFIED` text change. Phase 2 consumes this existing data to paint an Overview Panel and adds a progress dialog for large files.

## Goals / Non-Goals

**Goals:**
- Display a narrow panel on the right side of Scintilla showing proportional colored marks
- Mark colors follow rule configuration (`textColor` for LOG_TYPE, `bgColor` for STEP_TYPE)
- Adaptive mark height prevents solid-color panel when marks are dense
- Show a configurable viewport indicator box reflecting the currently visible region
- Single-click on panel navigates to the corresponding line (centered, caret placed)
- Panel updates whenever `g_matches` changes (zero extra parsing cost)
- Progress dialog with Cancel for large file parsing

**Non-Goals:**
- Drag-to-scroll the viewport box (click-to-navigate only)
- Notepad++ Docking API integration
- Per-file panel state persistence

## Decisions

### 1. Panel mechanism: Non-Client Area (NCA) painting on Scintilla HWND

The panel is painted in Scintilla's non-client area by stealing 14px from the right edge via `WM_NCCALCSIZE`. `SetWindowSubclass` (CommCtrl) safely intercepts `WM_NCCALCSIZE`, `WM_NCPAINT`, `WM_NCHITTEST`, `WM_NCLBUTTONDOWN`, and `WM_SETCURSOR`.

Alternative considered: `NPPM_DMMREGASDCKDLG` Notepad++ Docking API. Rejected — requires a separate registered window, more API surface, and the panel floats separately from the editor.

Alternative considered: child window inside Scintilla's parent. Rejected — fragile on NPP resize and version-dependent.

### 2. Navigation: single-click with centered scroll and caret placement

```
WM_NCLBUTTONDOWN in panel strip (HTBORDER)
  → store targetLine, SetTimer(10ms)  // defer past all click messages
  → WM_TIMER fires DoNavigation():
      targetFirst = targetLine - visibleLines/2
      SCI_SETFIRSTVISIBLELINE(targetFirst)   // center view
      SCI_SETEMPTYSELECTION(lineStartPos)    // place caret, no selection
      SCI_SETFIRSTVISIBLELINE(targetFirst)   // re-anchor after caret move
```

`SCI_SETEMPTYSELECTION` sets anchor=caret atomically — prevents selection from old cursor position.

Navigation is deferred via `SetTimer` (10 ms) because `WM_NCLBUTTONDOWN` arrives before Scintilla has processed its own click state; calling SCI_ messages synchronously inside `WM_NCLBUTTONDOWN` can produce drift.

`m_visibleLines` is cached during `WM_NCPAINT` (when Scintilla layout is settled) instead of querying `SCI_LINESONSCREEN` from the timer callback (which returns 1 while layout is unsettled).

### 3. Mark height: adaptive to prevent solid-color panel

```
markH_base = max(OVERVIEW_MARK_MIN_H=1, panelH / totalLines)
adaptive   = panelH * 0.5 / numMarks          // cap total coverage at 50%
markH      = max(1, min(markH_base, adaptive))
```

Consecutive marks of the same color that truly overlap (`y < mergeBot`) are merged into one taller mark. The 2px merge tolerance was removed to prevent premature merging on dense mark sets.

Alternative: fixed 3px minimum. Rejected — produces solid-color panel when many [ERROR] lines exist.

### 4. Mark colors from rule configuration

```cpp
// LOG_TYPE marks:  use rule.textColor  (MAKE_BGR == RGB, no swap needed)
// STEP_TYPE marks: use rule.bgColor
```

`MAKE_BGR(r,g,b)` produces the same byte layout as Windows `RGB(r,g,b)` — no channel swap is applied when passing to `CreateSolidBrush`.

### 5. Viewport indicator box

```
firstVisible = SCI_GETFIRSTVISIBLELINE()
visibleLines = SCI_LINESONSCREEN()           // cached as m_visibleLines

boxTop    = (firstVisible / totalLines) * panelH
boxBottom = ((firstVisible + visibleLines) / totalLines) * panelH

Fill: OVERVIEW_VIEWPORT_BG_COLOR (CLR_NONE → GetSysColor(COLOR_SCROLLBAR))
Border: OVERVIEW_VIEWPORT_COLOR
```

Fill color is configurable; default `CLR_NONE` matches the system scrollbar track color. `CLR_NONE` sentinel (0xFFFFFFFF) is checked at paint time.

### 6. Progress dialog

Modeless Win32 window (`WS_POPUP | WS_CAPTION | WS_SYSMENU`) — not a modal DialogBox. Fixed size enforced via `WM_GETMINMAXINFO`. Shown for every `ParseLog` invocation regardless of file size.

```
ParseLog()
  CreateProgressDialog()
  EnableWindow(hNpp, FALSE)    // block menus / shortcuts during parse
  ParseDocument(hSci, progressFn)
    // line-by-line scan, every 500 lines:
    //   SetProgressLine(hDlg, cur, total)
    //   PeekMessage loop (UI stays responsive)
    //   return !IsProgressCancelled(hDlg)
  EnableWindow(hNpp, TRUE)
  DestroyWindow(hDlg)
  if (cancelled) → ClearAllHighlights, clear panel, g_highlightActive=false
```

A `g_parseInProgress` re-entrancy guard prevents nested ParseLog calls even if a WM_COMMAND leaks through before `EnableWindow(FALSE)` takes effect.

### 7. Parser: line-by-line with local buffer copy

```cpp
// Copy Scintilla buffer before PeekMessage loop (pointer may be invalidated
// if user edits document during pumping):
std::vector<char> localBuf(raw, raw + docLen);

// Scan line by line with bounded find_range():
while (lineStart < docEnd) {
    lineEnd = find_newline(lineStart);
    // LOG_TYPE:  all keyword occurrences within [lineStart, lineEnd)
    // STEP_TYPE: prefix + digits + (space | end-of-line)
    if (progressFn && lineNo % 500 == 0)
        if (!progressFn(lineNo, totalLines)) return {};
    advance past \r\n;
}
```

### 8. Data flow

```
Ctrl+Alt+Q
      │
      ├─→ CreateProgressDialog()
      ├─→ ParseDocument(hSci, progressFn)  → g_matches
      ├─→ DestroyWindow(hDlg)
      ├─→ ClearAllHighlights()
      ├─→ ApplyHighlights()                → RDW_UPDATENOW (synchronous repaint)
      └─→ g_overviewPanel.Init(hNpp, hSci, hInst)   // lazy, first call only
            └─→ SetWindowSubclass(hSci)
                SetWindowPos(SWP_FRAMECHANGED)  // triggers WM_NCCALCSIZE
      └─→ g_overviewPanel.Update(hSci, BuildPanelMarks(...))

SCN_UPDATEUI (scroll / selection change)
      └─→ g_overviewPanel.UpdateViewport()  → InvalidateFrame()
```

`g_overviewPanel.Init()` is called after `ApplyHighlights()` so `SWP_FRAMECHANGED`'s queued `WM_SIZE` does not interfere with indicator rendering. `ApplyHighlights` uses `RDW_UPDATENOW` to flush the repaint synchronously before `WM_SIZE` is processed.

## Risks / Trade-offs

- **NCA subclassing**: Must remove subclass on `WM_NCDESTROY` to avoid dangling callbacks. Handled via `RemoveWindowSubclass` in `WM_NCDESTROY` case.
- **WM_NCPAINT performance**: Full panel repaint on every scroll event. Mitigated by double-buffering (off-screen DC + `BitBlt`).
- **PeekMessage during parse**: Document could be modified while scan uses a local buffer copy — safe, but result is slightly stale if user types during parse. Acceptable trade-off.
