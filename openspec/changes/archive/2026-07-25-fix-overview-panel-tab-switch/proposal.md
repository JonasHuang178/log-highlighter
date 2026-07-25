## Why

The Overview Panel does not update when the user switches tabs. After parsing file A with Ctrl+Alt+Q, switching to file B still shows A's marks in the panel. Switching back to A does not restore A's marks either.

The per-buffer state (`g_bufferStates`) and the `NPPN_BUFFERACTIVATED` handler in `Plugin.cpp` were correctly designed, but three issues prevented them from working:

1. **Wrong notification constants (root cause)** — `PluginInterface.h` defined `NPPN_BUFFERACTIVATED` as `NPPN_FIRST + 13` (1013) and `NPPN_FILEBEFORECLOSE` as `NPPN_FIRST + 6` (1006). The actual Notepad++ values are `NPPN_FIRST + 10` (1010) and `NPPN_FIRST + 3` (1003) respectively. As a result, the `NPPN_BUFFERACTIVATED` case in `beNotified` never matched any notification — the entire tab-switch handler was dead code. Similarly, `NPPN_FILEBEFORECLOSE` was actually matching `NPPN_FILEBEFOREOPEN`, so buffer state was never cleaned up on file close.

2. **Subclass stuck on one Scintilla HWND** — `Init()` installs the `SetWindowSubclass` hook on whichever Scintilla was active during the first parse. When the user switches to a tab in the other view (second Scintilla), `Update()` changes `m_hSci` to the new HWND, but the subclass remains on the old one. `InvalidateFrame()` then calls `RedrawWindow` on the unsubclassed Scintilla (no effect), while the old Scintilla keeps displaying stale marks.

3. **Asynchronous repaint** — `InvalidateFrame()` uses `RDW_FRAME | RDW_INVALIDATE` without `RDW_UPDATENOW`, so the repaint is deferred. During a tab switch, the deferred `WM_NCPAINT` can be delayed or coalesced, leaving the old marks visible until the next scroll or resize event.

## What Changes

- **Fix `NPPN_BUFFERACTIVATED` and `NPPN_FILEBEFORECLOSE` constants** in `external/PluginInterface.h` to match the official Notepad++ SDK values
- `OverviewPanel::Update()` detects when the Scintilla HWND changes and moves the subclass from the old HWND to the new one
- `Update()` uses synchronous `RedrawWindow` (`RDW_UPDATENOW`) to force an immediate panel repaint on tab switch

## Capabilities

### New Capabilities

None.

### Modified Capabilities

- `overview-panel`: `Update()` now handles Scintilla HWND changes (view switches) by migrating the subclass, and forces synchronous repaint for immediate visual feedback

## Impact

- `external/PluginInterface.h` — fix `NPPN_BUFFERACTIVATED` (`+13` → `+10`) and `NPPN_FILEBEFORECLOSE` (`+6` → `+3`) constants
- `src/OverviewPanel.cpp` — `Update()`: add HWND-change detection with `RemoveWindowSubclass` / `SetWindowSubclass` migration; replace `InvalidateFrame()` with synchronous `RedrawWindow`
- No changes to `OverviewPanel.h` or `Plugin.cpp`
- No user-visible behavior change beyond the bug fix: the panel now correctly reflects the active buffer's marks on every tab switch, and buffer state is properly cleaned up on file close
