## Context

The log-highlighter Notepad++ plugin parses log files using an Aho-Corasick scanner that matches keywords defined in `LogPatterns.h`. Matches are stored per-buffer in `BufferState::matches`. Currently, the only way to navigate to matched lines is by clicking the Overview Panel minimap. There is no keyboard-driven navigation.

The plugin registers commands via the Notepad++ `FuncItem` API. Currently 2 slots are used: Parse Log (Ctrl+Alt+Q) and About. Navigation to specific match types requires a new command slot.

## Goals / Non-Goals

**Goals:**
- Add a new `BookmarkRule` type separate from `LogTypeRule`, with `Start test` as its first keyword
- Provide Ctrl+Alt+W shortcut to jump to the next Bookmark match line, centered in the viewport
- Navigation wraps around to the first match when past the last one
- Reuse existing per-buffer match storage — no re-parsing needed for navigation

**Non-Goals:**
- Generic "navigate to any pattern type" framework — this is a single-purpose command
- Previous match navigation (Ctrl+Shift+Alt+W or similar) — can be added later
- Configurable shortcut keys — Notepad++ allows users to remap shortcuts via Settings > Shortcut Mapper

## Decisions

### Decision 1: Separate BookmarkRule type instead of reusing LOG_TYPE_RULES

**Choice**: Create a new `BookmarkRule` struct and `BOOKMARK_RULES[]` array, with a corresponding `MatchType::BOOKMARK` enum value.

**Rationale**: Bookmark keywords serve a different purpose (navigation targets) than log-level keywords. Keeping them separate allows independent configuration and filtering by `MatchType::BOOKMARK` for navigation.

### Decision 2: Navigation command filters by MatchType::BOOKMARK

**Choice**: The "Next Bookmark" command iterates `CurrentBuffer().matches`, filters for entries where `type == MatchType::BOOKMARK`, and jumps to the next one after the current caret position.

**Rationale**: Filtering by match type is clean and works regardless of how many bookmark keywords exist. The match list is already in memory — a linear scan is fast enough for any realistic log file.

### Decision 3: Deferred navigation via SetTimer for viewport centering

**Choice**: Use `SetTimer` with a 10ms delay and a `TIMERPROC` callback to perform the actual scroll centering, same pattern as `OverviewPanel::DoNavigation`.

**Rationale**: Notepad++ overrides scroll position after a shortcut command returns. Deferring the centering via timer ensures all command processing is done before we set the final scroll position.

### Decision 4: Expand FuncItem array from 2 to 3

**Choice**: Change `g_funcItems[2]` to `g_funcItems[3]` and add a new `ShortcutKey` static for Ctrl+Alt+W.

**Rationale**: Simple, follows existing pattern. The new command slot appears in the Plugins menu under log-highlighter.

## Risks / Trade-offs

- **[No matches found]** → Show a status bar message "No Bookmark matches found. Run Parse Log first." so the user knows to parse first.
- **[Performance with huge match lists]** → Linear scan of matches is O(n). Even with 100K matches, this completes in microseconds. Not a concern.
