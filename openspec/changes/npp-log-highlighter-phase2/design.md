## Context

Phase 1 delivers keyword highlighting inside the Scintilla editor via the Indicator API. `g_matches` (a `std::vector<Match>`) is already computed on every Ctrl+Alt+Q and on every `SCN_MODIFIED` text change. Phase 2 consumes this existing data to paint an Overview Panel — no additional document scanning is needed.

Notepad++ exposes a Docking API (`NPPM_DMMREGASDCKDLG`) that allows plugins to register a custom HWND as a docked panel. The panel is managed by Notepad++ (resize, show/hide, pin/float) while the plugin owns painting and interaction.

## Goals / Non-Goals

**Goals:**
- Display a narrow docked panel on the right side of Notepad++
- Paint proportional colored marks for each match where `showInPanel = true`
- Show a viewport indicator box reflecting the currently visible region
- Double-click on any mark navigates to that line (centered in viewport)
- Panel updates whenever `g_matches` changes (zero extra parsing cost)
- Panel width and minimum mark height configurable via `config/OverviewConfig.h`

**Non-Goals:**
- Drag-to-scroll the viewport box (click-to-navigate only)
- Overlay on Scintilla's actual scrollbar (Windows system control, not paintable)
- Per-file panel state persistence

## Decisions

### 1. Docking mechanism: NPPM_DMMREGASDCKDLG

Notepad++ provides a first-class docking API. The plugin registers a `tTbData` struct pointing to a custom HWND; Notepad++ handles resizing and the pin/float UI chrome.

Alternative considered: `CreateWindowEx` as a top-level child of the NPP HWND, manually positioned next to the scrollbar. Rejected — fragile on resize, no integration with NPP's docking system.

### 2. Panel initialization: NPPN_READY notification

The panel must be registered after Notepad++ has fully initialized its UI. `NPPN_READY` is sent via `WM_NOTIFY` to the plugin once NPP is ready. This is the standard point for docking panel registration.

Alternative: register in `setInfo()`. Rejected — `setInfo()` fires before the NPP window is ready; `RegisterAsDockDlg` would silently fail or crash.

### 3. Mark height: proportional with minimum protection

```
totalLines = SCI_GETLINECOUNT()
panelH     = client height of panel HWND (pixels)
lineH      = panelH / totalLines
markH      = max(OVERVIEW_MARK_MIN_H, lineH)   // never invisible
markY      = (matchLine / totalLines) * panelH
```

Consecutive marks of the same color closer than 2px are merged into one taller mark to preserve density information without producing a solid bar on very large files.

Alternative: fixed height (e.g., 4px). Rejected — gives no sense of document scale.

### 4. showInPanel flag in LogPatterns.h

Each rule independently controls editor coloring (`textColor` / `bgColor`) and panel visibility (`showInPanel`). This keeps the two concerns orthogonal — a rule can color the editor without cluttering the panel (e.g., `[ DEBUG ]`).

Default: `false` for all rules. `[ ERROR ]` ships with `true`.

### 5. Viewport indicator

```
firstVisible = SCI_GETFIRSTVISIBLELINE()
visibleLines = SCI_LINESONSCREEN()

boxTop    = (firstVisible / totalLines) * panelH
boxBottom = ((firstVisible + visibleLines) / totalLines) * panelH
```

Drawn as a semi-transparent rectangle (color: `OVERVIEW_VIEWPORT_COLOR`) over the mark layer. Redrawn on `SCN_UPDATEUI` to stay in sync with scrolling.

### 6. Double-click navigation

```
WM_LBUTTONDBLCLK → y (click pixel)
targetLine = (y / panelH) * totalLines
SCI_GOTOLINE(targetLine)
SCI_SCROLLCARET   // centers the line in viewport
```

Finds the nearest match to `targetLine` among panel-visible matches for precision.

### 7. Data flow

```
Ctrl+Alt+Q / SCN_MODIFIED
      │
      ├─→ ParseDocument()        → g_matches  (no change)
      ├─→ ClearAllHighlights()               (no change)
      ├─→ ApplyHighlights()                  (no change)
      └─→ g_overviewPanel.Update(hSci, g_matches)   ← NEW
                │
                └─→ InvalidateRect(m_hwnd)   triggers WM_PAINT

SCN_UPDATEUI (scroll / selection change)
      └─→ g_overviewPanel.UpdateViewport()   ← NEW (redraws viewport box only)
```

## Risks / Trade-offs

- **NPPN_READY value**: Must match the Notepad++ SDK exactly. Wrong value → panel never registers silently. Mitigation: cross-check against Notepad++ official `Notepad_plus_msgs.h`.
- **WM_PAINT performance on large match sets**: Thousands of marks redrawn on every scroll. Mitigation: use a back-buffer (paint to an off-screen DC, then `BitBlt`) to eliminate flicker.
- **DPI awareness**: Mark coordinates assume 1:1 pixel mapping. On high-DPI displays, panel height may differ. Mitigation: use `GetClientRect` at paint time, never cache panel height.
