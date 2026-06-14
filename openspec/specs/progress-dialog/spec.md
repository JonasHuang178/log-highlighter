## Requirements

### Requirement: Progress dialog threshold
A modeless progress dialog SHALL be shown only when the document has **≥ 5 000 lines** (`PROGRESS_MIN_LINES = 5000`). For smaller files the parse completes fast enough that a dialog would flash and disappear without being useful.

#### Scenario: Small file — no dialog
- **WHEN** the document has fewer than 5 000 lines
- **THEN** no progress dialog is shown; `ParseDocument(hSci)` is called directly on the UI thread

#### Scenario: Large file — dialog shown
- **WHEN** the document has 5 000 or more lines
- **THEN** the progress dialog appears before parsing begins

---

### Requirement: Buffer snapshot before dialog creation
Before showing the dialog, the plugin SHALL:
1. Call `SCI_GETLENGTH` and `SCI_GETCHARACTERPOINTER` on the UI thread to obtain the raw buffer pointer.
2. Copy the full document into a `std::vector<char>` (`docBuf`).

This snapshot MUST happen before `CreateProgressDialog` or any other window call, so that all three Scintilla calls are atomic with respect to the message queue. The worker thread reads only `docBuf` and never calls `SendMessage` to Scintilla.

---

### Requirement: Worker thread parsing
Parsing SHALL run on a `std::thread` so the UI thread remains in a message loop and the progress dialog stays alive and responsive.

The worker thread calls `ParseDocument(docBuf, totalLines, progressFn)`. The `progressFn` lambda:
- Posts `PostMessage(hDlg, WM_APP, cur, total)` to update the dialog label
- Checks `workerCancelled.load()` and returns `false` to abort if cancelled

The UI thread waits via `MsgWaitForMultipleObjects(1, &hDone, FALSE, INFINITE, QS_ALLINPUT)`. On each wake that is NOT `WAIT_OBJECT_0`:
- Drains the message queue with `PeekMessage / PM_REMOVE`
- `WM_APP` messages destined for `hDlg` are dispatched to `SetProgressLine` directly (not through `DispatchMessage`)
- All other messages go through `TranslateMessage / DispatchMessage`
- Checks `IsProgressCancelled(hDlg)` and sets `workerCancelled = true` if cancelled

After `WAIT_OBJECT_0`: drains any remaining messages, then calls `worker.join()`.

#### Scenario: Progress updates during parse
- **WHEN** the worker thread processes every 500 lines (or reaches the final line)
- **THEN** `PostMessage` is sent to `hDlg`; the UI thread processes it and updates the label to `"Processing: X / Y lines"`

#### Scenario: Parse completes normally
- **WHEN** the worker thread finishes without cancellation
- **THEN** `hDone` is signalled; the UI thread joins the worker, re-enables NPP, destroys the dialog, and proceeds to apply highlights and update the panel

---

### Requirement: NPP window disabled during parse
`EnableWindow(g_nppData._nppHandle, FALSE)` SHALL be called immediately after the dialog is shown. `EnableWindow(TRUE)` SHALL be called before `DestroyWindow(hDlg)`. This prevents re-entrant `Ctrl+Alt+Q` presses and interaction with NPP menus while parsing is in progress.

A separate boolean `g_parseInProgress` provides a re-entrancy guard independent of the window enable state.

---

### Requirement: Cancel support
The Cancel button (or closing the dialog via the X button) SHALL set `DlgState::cancelled = true`. `IsProgressCancelled` returns this flag. The UI thread propagates it to `workerCancelled` (atomic bool). The worker's `progressFn` returns `false` on the next 500-line tick, causing `ScanBuffer` to return an empty vector.

After cancellation:
- `ClearAllHighlights` is called
- `g_appliedByteEnd` is reset to `-1`
- The Overview Panel is cleared (empty mark list)
- `g_highlightActive` is set to `false`
- The document text is not modified

#### Scenario: User cancels mid-parse
- **WHEN** the user clicks Cancel while the progress dialog is visible
- **THEN** parsing stops within the next 500-line interval, all highlights are cleared, the panel shows no marks, and the editor content is unchanged

---

### Requirement: Progress dialog window
The dialog is a `WS_POPUP | WS_CAPTION | WS_SYSMENU` window with `WS_EX_DLGMODALFRAME`.

- Fixed size: 320 × 140 px, enforced via `WM_GETMINMAXINFO`
- Centred over the NPP main window via `GetWindowRect(hParent)`
- Contains one `STATIC` text label (updated by `SetProgressLine`) and one `BUTTON` Cancel
- Closing via X button (`WM_CLOSE`) is treated as Cancel (sets flag, does not destroy the window — the caller destroys it)
- `WM_DESTROY` frees the per-window `DlgState` struct

`SetProgressLine(hDlg, current, total)` updates the label to `"Processing: N / M lines"`.
`IsProgressCancelled(hDlg)` reads `DlgState::cancelled`.
