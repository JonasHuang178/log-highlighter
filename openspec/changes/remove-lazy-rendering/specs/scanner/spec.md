## MODIFIED Requirements

### Requirement: Single ParseDocument entry point
`ParseDocument` SHALL expose exactly one overload, taking `HWND hScintilla` and an optional progress callback. It SHALL be called on the UI thread only, and SHALL:
1. Call `SCI_GETLENGTH` and `SCI_GETCHARACTERPOINTER` to get the buffer pointer and length.
2. Copy the buffer into a local `std::vector<char>`.
3. Call `SCI_GETLINECOUNT`.
4. Call `ScanBuffer` on the local copy.

The local copy is what makes the scan safe while the progress callback pumps messages: the pointer returned by `SCI_GETCHARACTERPOINTER` can be invalidated by a document edit, the copy cannot.

`ScanBuffer` itself remains `static`, makes no Win32 or Scintilla calls, and is therefore thread-agnostic — but it is not exposed outside `Parser.cpp`.

#### Scenario: Buffer stays valid across a callback
- **WHEN** the progress callback pumps messages and the user edits the document mid-scan
- **THEN** the scan continues on the local copy without crashing; results are for the pre-edit snapshot

## REMOVED Requirements

### Requirement: Two ParseDocument overloads
**Reason**: The worker-thread overload `ParseDocument(const std::vector<char>& docBuf, int totalLines, progressFn)` has no caller. Parsing runs on the UI thread with `PeekMessage` pumping inside the progress callback; the project contains no `std::thread`. The UI-thread overload already snapshots the document into a local buffer, which is what the second overload was factored out to allow.

**Migration**: Call `ParseDocument(hScintilla, progressFn)`. If background-thread parsing is wanted later, re-expose a buffer-taking wrapper around `ScanBuffer` — the scanner core needs no change.
