## MODIFIED Requirements

### Requirement: Per-buffer state map
The plugin SHALL maintain a `std::unordered_map<LRESULT, BufferState>` named `g_bufferStates` where the key is the NPP buffer ID returned by `NPPM_GETCURRENTBUFFERID`.

`BufferState` SHALL contain:
- `std::vector<Match> matches` — parse results for this buffer
- `bool highlightActive = false` — whether this buffer has been parsed at least once

#### Scenario: New buffer default state
- **WHEN** Ctrl+Alt+Q is pressed on a buffer that has never been parsed
- **THEN** a new `BufferState` entry is created with `highlightActive = false`

#### Scenario: Parsed buffer retains state
- **WHEN** Ctrl+Alt+Q completes on buffer A, then the user switches to buffer B and back to A
- **THEN** `g_bufferStates[idA].matches` still contains A's parse results and `highlightActive` is still `true`

## REMOVED Requirements

### Requirement: BufferState lazy-rendering cursor
**Reason**: The `intptr_t appliedByteEnd = -1` field existed only to drive lazy rendering, which was never implemented and is now removed. `ParseLog()` assigned it twice and no code ever read it.

**Migration**: None. All highlights for a buffer are applied in a single `ApplyHighlights` call at parse time, so there is no partial-coverage cursor to track.
