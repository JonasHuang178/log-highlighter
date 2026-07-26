## Why

When reviewing long log files, users need to quickly jump to specific bookmark markers (e.g., `Start test`) that indicate the beginning of a test or section. Currently there is no keyboard-driven navigation to jump between pattern-matched lines — the only navigation is clicking the Overview Panel minimap. Adding a Bookmark keyword with a dedicated shortcut (Ctrl+Alt+W) enables fast, keyboard-only navigation through log sections.

## What Changes

- Add a new `BookmarkRule` struct and `BOOKMARK_RULES` array in `LogPatterns.h`, separate from `LOG_TYPE_RULES`, with `Start test` as the first entry
- Introduce a "Next Bookmark" command registered as a new `FuncItem` with shortcut Ctrl+Alt+W
- The command iterates through the current buffer's parsed matches, finds the next Bookmark match after the caret, and jumps the editor to that line (wrapping to the top if no match is found below)
- Expand `g_funcItems` array from 2 to 3 to accommodate the new command

## Capabilities

### New Capabilities
- `bookmark-navigation`: Keyboard shortcut (Ctrl+Alt+W) to jump to the next Bookmark keyword match in the current buffer, cycling through all matches

### Modified Capabilities
- `log-patterns-config`: Adding a new `BookmarkRule` struct and `BOOKMARK_RULES` array with its own color and `showInPanel = true`

## Impact

- `config/LogPatterns.h` — new `BookmarkRule` struct and `BOOKMARK_RULES` array
- `src/Parser.h` — new `BOOKMARK` value in `MatchType` enum
- `src/Parser.cpp` — register bookmark patterns in Aho-Corasick scanner
- `src/Plugin.cpp` — new command function, expanded `FuncItem` array from 2 to 3, new `ShortcutKey` for Ctrl+Alt+W
- `src/log-highlighter.cpp` — new indicator range for bookmark matches
