## 1. Define BufferState and replace globals in Plugin.cpp

- [x] 1.1 Add `#include <unordered_map>` to Plugin.cpp
- [x] 1.2 Define `struct BufferState { std::vector<Match> matches; intptr_t appliedByteEnd = -1; bool highlightActive = false; };`
- [x] 1.3 Replace `static std::vector<Match> g_matches`, `static intptr_t g_appliedByteEnd`, `static bool g_highlightActive` with `static std::unordered_map<LRESULT, BufferState> g_bufferStates`
- [x] 1.4 Add `static BufferState& CurrentBuffer()` helper that calls `NPPM_GETCURRENTBUFFERID` and returns `g_bufferStates[id]`

## 2. Update ParseLog() to write per-buffer state

- [x] 2.1 Replace all `g_matches` references in `ParseLog()` with `CurrentBuffer().matches`
- [x] 2.2 Replace all `g_appliedByteEnd` references in `ParseLog()` with `CurrentBuffer().appliedByteEnd`
- [x] 2.3 Replace all `g_highlightActive` references in `ParseLog()` with `CurrentBuffer().highlightActive`

## 3. Update SCN_UPDATEUI lazy rendering to use per-buffer state

- [x] 3.1 N/A — this branch (phase2 base) has no lazy rendering in SCN_UPDATEUI; handler just calls UpdateViewport(), correct as-is
- [x] 3.2 N/A — same reason as 3.1

## 4. Add NPPN_BUFFERACTIVATED handler

- [x] 4.1 Add `case NPPN_BUFFERACTIVATED:` to `beNotified`
- [x] 4.2 Read buffer ID from `notification->nmhdr.idFrom`
- [x] 4.3 Look up the buffer in `g_bufferStates`; if `highlightActive = true` call `g_overviewPanel.Update(hSci, BuildPanelMarks(hSci, state.matches))`
- [x] 4.4 If buffer not found or `highlightActive = false`, call `g_overviewPanel.Update(hSci, {})` to clear the panel

## 5. Add NPPN_FILEBEFORECLOSE handler

- [x] 5.1 Add `case NPPN_FILEBEFORECLOSE:` to `beNotified`
- [x] 5.2 Erase the buffer entry: `g_bufferStates.erase(static_cast<LRESULT>(notification->nmhdr.idFrom))`

## 6. Remove SCN_MODIFIED re-parse block

- [x] 6.1 Delete the entire `case SCN_MODIFIED:` block from `beNotified`

## 7. Verify and test

- [ ] 7.1 Parse a large log file with Ctrl+Alt+Q — highlights and Overview Panel appear
- [ ] 7.2 Switch to another tab — Overview Panel clears (if unparsed) or shows that buffer's marks
- [ ] 7.3 Switch back to the parsed tab — highlights are instantly visible, no re-parse
- [ ] 7.4 Open a new file — no auto-parse occurs
- [ ] 7.5 Close the parsed tab — no crash, memory is released
- [ ] 7.6 Press Ctrl+Alt+Q twice on the same buffer — results are replaced correctly
