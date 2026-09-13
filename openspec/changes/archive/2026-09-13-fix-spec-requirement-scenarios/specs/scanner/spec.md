## MODIFIED Requirements

### Requirement: Match struct
Each match found by `ScanBuffer` SHALL be recorded as one `Match` entry:

| Field | Type | Description |
|---|---|---|
| `type` | `MatchType` | `LOG_TYPE`, `STEP_TYPE` or `BOOKMARK` |
| `ruleIndex` | `int` | Index into the rule table named by `type` |
| `byteOffset` | `intptr_t` | Start byte position in the document |
| `length` | `intptr_t` | Byte length of the highlighted range |

`length` SHALL be the byte length of the matched keyword for `LOG_TYPE` and
`BOOKMARK`, and the distance from the first byte of the prefix to the last
character of the line, excluding the line ending, for `STEP_TYPE`.

The returned vector SHALL be ordered by `byteOffset` ascending, which
`ScanBuffer` obtains for free by scanning left to right. Consumers SHALL be able
to rely on that order: `BookmarkLinesFrom` deduplicates adjacent lines by
comparing against the previous entry only, which is correct only for a sorted
vector.

#### Scenario: Keyword match covers the keyword only
- **WHEN** a `LOG_TYPE` rule whose keyword is 9 bytes long matches at byte 400
- **THEN** the entry records `byteOffset` 400 and `length` 9
- **AND** `ruleIndex` indexes `LOG_TYPE_RULES[]`

#### Scenario: Step match extends to end of line
- **WHEN** the text `Step12 rest of line\n` begins at byte 400
- **THEN** the entry records `byteOffset` 400 and a `length` reaching the last character before `\n`

#### Scenario: Bookmark match is distinguishable from a log match
- **WHEN** a `BOOKMARK` rule matches
- **THEN** the entry's `type` is `BOOKMARK` and its `ruleIndex` indexes `BOOKMARK_RULES[]`
- **AND** it is not reported as `LOG_TYPE`, even though both are exact keyword matches coloured the same way

#### Scenario: Results are ordered by position
- **WHEN** a document contains matches on several lines
- **THEN** the entries appear in ascending `byteOffset` order

---

### Requirement: Progress callback
`ScanBuffer` SHALL accept an optional `std::function<bool(int cur, int total)>`
progress callback. When one is supplied it SHALL be invoked every 500 completed
lines and once for the final line.

When the callback returns `false` the scan SHALL abort and return an empty
vector. A cancelled scan SHALL NOT return the matches it had already collected,
so that a caller cannot mistake a partial result for a complete one.

#### Scenario: Callback invoked at the tick interval
- **WHEN** a scan processes a document of 1200 lines with a callback supplied
- **THEN** the callback is invoked on lines 500 and 1000, and once more for the final line

#### Scenario: Cancellation discards partial results
- **WHEN** the callback returns `false` after matches have already been collected
- **THEN** the scan stops and returns an empty vector
- **AND** the caller receives nothing that could be mistaken for a complete scan

#### Scenario: No callback supplied
- **WHEN** `ScanBuffer` is called without a progress callback
- **THEN** the scan runs to completion and reports progress to nobody
