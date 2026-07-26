## 1. Add Bookmark rule type

- [x] 1.1 Add `BookmarkRule` struct and `BOOKMARK_RULES` array in `config/LogPatterns.h` with `Start test` entry, magenta color `MAKE_BGR(200,0,180)` and `showInPanel = true`

## 2. Register Next Bookmark command

- [x] 2.1 Expand `g_funcItems` array from `[2]` to `[3]` in `src/Plugin.cpp`
- [x] 2.2 Add a static `ShortcutKey g_nextBookmarkKey` for Ctrl+Alt+W
- [x] 2.3 Register "Next Bookmark" as `g_funcItems[1]` in `getFuncsArray()` with the Ctrl+Alt+W shortcut, update `*nbF = 3`

## 3. Implement navigation logic

- [x] 3.1 Implement `NextBookmark()` function in `src/Plugin.cpp` that: gets current caret line, scans `CurrentBuffer().matches` for BOOKMARK matches, finds the next match line after caret (wrapping to first if needed), and navigates using deferred timer for viewport centering
- [x] 3.2 Show status bar message via `NPPM_SETSTATUSBAR` when no matches are found

## 4. Verification

- [x] 4.1 Build the plugin and verify it compiles without errors
- [x] 4.2 Test: parse a log with `Start test` lines, press Ctrl+Alt+W, confirm it jumps to the next Bookmark line centered in viewport and wraps around
