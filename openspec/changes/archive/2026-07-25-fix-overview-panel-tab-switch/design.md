## Context

The Overview Panel is a single `OverviewPanel` instance (`g_overviewPanel`) that draws into Scintilla's non-client area via `SetWindowSubclass`. Notepad++ maintains two Scintilla HWNDs (`_scintillaMainHandle` and `_scintillaSecondHandle`); even in single-view mode both exist, and dual-view mode makes both visible.

The tab-switch update path was fully implemented in `Plugin.cpp`:
- `NPPN_BUFFERACTIVATED` → look up `g_bufferStates[id]` → call `g_overviewPanel.Update()` with the buffer's marks or an empty vector
- `NPPN_FILEBEFORECLOSE` → erase the buffer's entry from `g_bufferStates`

But neither handler ever executed because the notification constants in `PluginInterface.h` were wrong:

| Constant | Our value | NPP actual value | What ours matched |
|---|---|---|---|
| `NPPN_FILEBEFORECLOSE` | `NPPN_FIRST + 6` (1006) | `NPPN_FIRST + 3` (1003) | `NPPN_FILEBEFOREOPEN` |
| `NPPN_BUFFERACTIVATED` | `NPPN_FIRST + 13` (1013) | `NPPN_FIRST + 10` (1010) | Nothing (no standard notification at 1013) |

With the constants fixed, the handlers fire correctly. Two secondary issues in `OverviewPanel::Update()` are fixed in the same pass:

1. **Split-brain subclass state** — `Init()` installs the subclass on one Scintilla HWND. `Update()` overwrites `m_hSci` with whatever HWND is passed in, but never moves the subclass. When the user switches views (different Scintilla HWND), `InvalidateFrame()` targets the unsubclassed HWND (no effect) while the subclassed HWND keeps displaying stale marks.

2. **Deferred repaint** — `InvalidateFrame()` uses asynchronous `RedrawWindow` flags. During tab-switch message processing, the deferred `WM_NCPAINT` can be delayed.

## Goals / Non-Goals

**Goals:**
- Overview Panel immediately shows the active buffer's marks (or clears) on every tab switch
- Subclass follows the active Scintilla HWND when the user switches views
- Buffer state is properly cleaned up on file close (not file open)
- Zero user-visible behavior change beyond the fix

**Non-Goals:**
- Showing the panel on both Scintilla views simultaneously — only the active view gets the panel
- Changing `UpdateViewport()` to use synchronous repaint — scroll-driven repaints are fine asynchronous
- Auditing all other NPP notification constants — only the two that affect the bug are fixed

## Decisions

### Decision: Fix the constants, don't restructure the notification handling

The `NPPN_BUFFERACTIVATED` and `NPPN_FILEBEFORECLOSE` handlers in `Plugin.cpp` are correctly written — the only problem was that they never executed. Fixing the two constants in `PluginInterface.h` is the minimal, correct fix.

**Alternative**: Restructure to avoid depending on notification constants (e.g., poll buffer ID on timer). Rejected — the notification-driven approach is correct and efficient; the constants just need to match the SDK.

### Decision: Move subclass in `Update()`, not a separate method

`Update()` is the only place where `m_hSci` changes. Detecting the HWND mismatch and migrating the subclass there keeps the logic self-contained — callers don't need to know whether the view changed.

**Alternative**: Add a `SwitchScintilla(HWND)` method and call it from `Plugin.cpp` before `Update()`. Rejected — it forces the caller to track which Scintilla was previously active, duplicating state that `OverviewPanel` already has.

### Decision: Synchronous repaint only in `Update()`, not in `InvalidateFrame()`

`InvalidateFrame()` is also called by `UpdateViewport()` on every `SCN_UPDATEUI` (scroll, selection change). Adding `RDW_UPDATENOW` there would make every scroll tick synchronous, which is unnecessary overhead. `Update()` calls `RedrawWindow` directly with `RDW_UPDATENOW` instead.

### Decision: Remove subclass from old Scintilla before installing on new

The migration sequence is:
1. `RemoveWindowSubclass(oldHSci)` — detach from old Scintilla
2. `SetWindowPos(oldHSci, SWP_FRAMECHANGED)` — old Scintilla reclaims the 14px strip
3. `SetWindowSubclass(newHSci)` — attach to new Scintilla
4. `SetWindowPos(newHSci, SWP_FRAMECHANGED)` — new Scintilla shrinks by 14px

This ensures no moment where both Scintillas have a panel strip or neither does.

## Risks / Trade-offs

- [Risk] Other notification constants in `PluginInterface.h` might also be wrong. Currently only `NPPN_READY` (used but not critical) remains besides the two fixed constants. Future additions should be verified against the official Notepad++ SDK header (`Notepad_plus_msgs.h`).
- [Risk] Moving the subclass triggers `SWP_FRAMECHANGED` on both Scintillas, causing a brief layout recalculation. Acceptable — this only happens on view switch (rare), not on tab switch within the same view (common).
- [Trade-off] The panel strip disappears from the old view when switching views. Intentional — the panel follows focus, consistent with it being a single-instance resource.
