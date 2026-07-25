## 1. Remove lazy rendering from the renderer

- [x] 1.1 In `src/log-highlighter.h`, delete the `ApplyHighlightsInRange` declaration and its comment block
- [x] 1.2 In `src/log-highlighter.h`, fix the stale comments on `InitStyles` ("Called on every SCN_MODIFIED…") and `ApplyHighlights` ("used for SCN_MODIFIED re-highlights") — SCN_MODIFIED re-highlighting no longer exists; `InitStyles` is called once per `ParseLog()`
- [x] 1.3 In `src/log-highlighter.cpp`, delete the `ApplyHighlightsInRange` definition
- [x] 1.4 In `src/log-highlighter.cpp`, drop `#include <algorithm>` (only `std::lower_bound` in the deleted function needed it)

## 2. Remove the lazy cursor from per-buffer state

- [x] 2.1 In `src/Plugin.cpp`, delete `intptr_t appliedByteEnd = -1;` from `struct BufferState` and update the struct comment (it still says "lazy-rendering cursor")
- [x] 2.2 In `src/Plugin.cpp`, delete `buf.appliedByteEnd = -1;` in the cancelled path of `ParseLog()`
- [x] 2.3 In `src/Plugin.cpp`, delete `buf.appliedByteEnd = -1;` in the success path of `ParseLog()`
- [x] 2.4 In `src/Plugin.cpp`, confirm the `SCN_UPDATEUI` handler keeps `g_overviewPanel.UpdateViewport()` — the viewport indicator box is unrelated to lazy rendering and must stay

## 3. Remove the worker-thread ParseDocument overload

- [x] 3.1 In `src/Parser.h`, delete the `ParseDocument(const std::vector<char>&, int, progressFn)` declaration and its comment
- [x] 3.2 In `src/Parser.h`, update the remaining overload's comment to state that the UI-thread call snapshots the buffer internally
- [x] 3.3 In `src/Parser.cpp`, delete the buffer-taking `ParseDocument` definition and its banner comment
- [x] 3.4 Verify `ScanBuffer` stays `static` and its "safe to call from any thread" comment remains accurate

## 4. Drop the ApplyHighlights repaintAfter parameter

- [x] 4.1 In `src/log-highlighter.h`, remove the `bool repaintAfter = true` parameter and rewrite the comment — `ApplyHighlights` always repaints now
- [x] 4.2 In `src/log-highlighter.cpp`, remove the parameter from the definition and make the closing `RedrawWindow` unconditional
- [x] 4.3 In `src/Plugin.cpp`, drop the now-meaningless `// repaintAfter = true (default)` comment from the call site

## 5. Update README.md

- [x] 5.1 Delete the "Real-time re-highlight" section from Features; replace with a note that Ctrl+Alt+Q is the only trigger and that switching tabs restores the panel from cached per-buffer results without re-parsing
- [x] 5.2 Delete the "Lazy rendering" section (including the Overview Panel note about reading from the in-memory match list — fold the useful half into the Overview Panel section)
- [x] 5.3 Replace "Large files (≥ 5 000 lines)" with an accurate progress-dialog description: shown on **every** parse, two phases (`Processing: N / M lines` → `Applying highlights...`), Cancel available during parsing only, runs on the UI thread with `PeekMessage` pumping, NPP window disabled for the duration
- [x] 5.4 In "Key implementation notes", replace the "Lazy rendering" bullet with a bullet on the `SC_MOD_CHANGEINDICATOR` mod-event mask trick in `ApplyHighlights` (the actual reason bulk fill is fast)
- [x] 5.5 In "Key implementation notes", fix the "Progress dialog" bullet — remove `std::thread` and `MsgWaitForMultipleObjects`; describe the `PeekMessage` pump inside the progress callback
- [x] 5.6 In "Project Structure", verify the file list still matches `src/` and `config/` (add `config/OverviewConfig.h`, which is missing)

## 6. Verify

- [x] 6.1 Build Release x64 — no warnings about unused includes or missing declarations
      (clean rebuild passes; remaining warnings are pre-existing: C4819 codepage on
      every file, C4312 in `ProgressDialog.cpp`, LNK4070 from the stale name in
      `log-highlighter.def`)
- [ ] 6.2 Ctrl+Alt+Q on a small log file: all keywords highlighted, panel marks present, status bar shows parse time
- [ ] 6.3 Ctrl+Alt+Q on a large log file (100 k+ lines): both dialog phases visible, all highlights present after dismissal, scrolling to the bottom shows highlights already applied (no lazy fill-in)
- [ ] 6.4 Switch tabs away and back: panel marks restored, no re-parse, no dialog
- [ ] 6.5 Cancel during parsing: highlights cleared, panel empty, no status-bar timing written
