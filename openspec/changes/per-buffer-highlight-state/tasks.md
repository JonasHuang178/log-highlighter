## 1. Define BufferState and replace globals in Plugin.cpp

- [ ] 1.1 Add `#include <unordered_map>` to Plugin.cpp
- [ ] 1.2 Define `struct BufferState { std::vector<Match> matches; intptr_t appliedByteEnd = -1; bool highlightActive = false; };`
- [ ] 1.3 Replace `static std::vector<Match> g_matches`, `static intptr_t g_appliedByteEnd`, `static bool g_highlightActive` with `static std::unordered_map<LRESULT, BufferState> g_bufferStates`
- [ ] 1.4 Add `static BufferState& CurrentBuffer()` helper that calls `NPPM_GETCURRENTBUFFERID` and returns `g_bufferStates[id]`

## 2. Update ParseLog() to write per-buffer state

- [ ] 2.1 Replace all `g_matches` references in `ParseLog()` with `CurrentBuffer().matches`
- [ ] 2.2 Replace all `g_appliedByteEnd` references in `ParseLog()` with `CurrentBuffer().appliedByteEnd`
- [ ] 2.3 Replace all `g_highlightActive` references in `ParseLog()` with `CurrentBuffer().highlightActive`

## 3. Update SCN_UPDATEUI lazy rendering to use per-buffer state

- [ ] 3.1 In the `SCN_UPDATEUI` handler, retrieve the current buffer's `BufferState` via `CurrentBuffer()`
- [ ] 3.2 Replace `g_matches`, `g_appliedByteEnd`, `g_highlightActive` references in the `SCN_UPDATEUI` block with the per-buffer fields

## 4. Add NPPN_BUFFERACTIVATED handler

- [ ] 4.1 Add `case NPPN_BUFFERACTIVATED:` to `beNotified`
- [ ] 4.2 Read buffer ID from `notification->nmhdr.idFrom`
- [ ] 4.3 Look up the buffer in `g_bufferStates`; if `highlightActive = true` call `g_overviewPanel.Update(hSci, BuildPanelMarks(hSci, state.matches))`
- [ ] 4.4 If buffer not found or `highlightActive = false`, call `g_overviewPanel.Update(hSci, {})` to clear the panel

## 5. Add NPPN_FILEBEFORECLOSE handler

- [ ] 5.1 Add `case NPPN_FILEBEFORECLOSE:` to `beNotified`
- [ ] 5.2 Erase the buffer entry: `g_bufferStates.erase(static_cast<LRESULT>(notification->nmhdr.idFrom))`

## 6. Remove SCN_MODIFIED re-parse block

- [ ] 6.1 Delete the entire `case SCN_MODIFIED:` block from `beNotified`

## 7. Verify and test

- [ ] 7.1 Parse a large log file with Ctrl+Alt+Q — highlights and Overview Panel appear
- [ ] 7.2 Switch to another tab — Overview Panel clears (if unparsed) or shows that buffer's marks
- [ ] 7.3 Switch back to the parsed tab — highlights are instantly visible, no re-parse
- [ ] 7.4 Open a new file — no auto-parse occurs
- [ ] 7.5 Close the parsed tab — no crash, memory is released
- [ ] 7.6 Press Ctrl+Alt+Q twice on the same buffer — results are replaced correctly
