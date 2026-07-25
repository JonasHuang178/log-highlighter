## MODIFIED Requirements

### Requirement: Panel follows active Scintilla view
When `Update()` is called with a Scintilla HWND different from the currently subclassed one (the user switched views), the panel SHALL migrate: remove the subclass from the old Scintilla, restore its full client area via `SWP_FRAMECHANGED`, install the subclass on the new Scintilla, and trigger `SWP_FRAMECHANGED` on it. The panel is a single resource that follows the active view.

#### Scenario: Switch from main view to second view
- **WHEN** the panel is subclassed on scintillaMainHandle and `Update()` is called with scintillaSecondHandle
- **THEN** the subclass is removed from scintillaMainHandle (its client area is restored to full width) and installed on scintillaSecondHandle (which gains the 14px panel strip)

#### Scenario: Single-view tab switch — no migration
- **WHEN** all tabs are in the same view and the user switches tabs
- **THEN** the Scintilla HWND is unchanged, no subclass migration occurs, and the panel redraws with the new buffer's marks (or clears if unparsed)

### Requirement: Synchronous repaint on Update
`Update()` SHALL use `RedrawWindow` with `RDW_UPDATENOW` (in addition to `RDW_FRAME | RDW_INVALIDATE`) to force synchronous repaint. This ensures the panel reflects the active buffer's marks immediately on tab switch, without waiting for the next deferred paint cycle.

`UpdateViewport()` (called on `SCN_UPDATEUI` during scroll) SHALL continue using asynchronous `InvalidateFrame()` to avoid unnecessary synchronous overhead on every scroll tick.

#### Scenario: Panel clears immediately on tab switch to unparsed buffer
- **WHEN** file A has been parsed and the user switches to unparsed file B
- **THEN** the panel clears (no marks) within the same message-processing pass — the user never sees A's marks on B's tab

#### Scenario: Panel restores immediately on tab switch to parsed buffer
- **WHEN** files A and B are both parsed, the user is on A, and switches to B
- **THEN** the panel immediately shows B's marks — no flicker or stale A marks visible
