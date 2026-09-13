## Purpose

Finding every configured keyword in the document in a single pass, at a cost
that does not grow with the number of keywords. Separate from the rule
configuration because it defines *how* matches are found, and it is reused by
both highlighting and the report framework.
## Requirements
### Requirement: Aho-Corasick single-pass scanner
`Parser.cpp` SHALL build a single Aho-Corasick automaton from all rules in `LogPatterns.h` at first use (lazy static init). The automaton is built once per process lifetime.

The automaton SHALL be constructed with:
1. **Trie insertion** — one path per `LOG_TYPE_RULES[i].keyword` and one path per `STEP_TYPE_RULES[i].prefix`, tagged with rule index and type.
2. **BFS failure links** — standard Aho-Corasick construction. The root's undefined transitions loop back to root. Every state's `next[]` array is fully populated (no `-1` entries) so scanning requires no conditional branching per character.
3. **Output inheritance** — during BFS, each state's output list is extended with the outputs of its failure state (suffix matches).

Scanning complexity is O(N) in the document length regardless of the number of patterns.

#### Scenario: Multiple keywords in one pass
- **WHEN** the document contains both `[ ERROR ]` and `Step1 ` on different lines
- **THEN** both are found in a single left-to-right scan with no backtracking

---

### Requirement: STEP_TYPE post-match validation
After the automaton reports a prefix match (e.g. `"Step"` ending at position `p`), the scanner SHALL validate the suffix:
1. The character immediately after the prefix (`p+1`) MUST be an ASCII digit (`isdigit`).
2. All subsequent digits are consumed.
3. The first non-digit character MUST be a space (`' '`), carriage return (`'\r'`), or newline (`'\n'`), OR the position MUST be at the end of the buffer.

If validation fails, the match is discarded. If validation passes, the matched range is extended to the end of the line (up to but not including `'\r'` or `'\n'`).

#### Scenario: Valid Step match
- **WHEN** text is `Step12 rest of line\n`
- **THEN** a STEP_TYPE match is recorded from the start of `Step` to the end of `line` (before `\n`)

#### Scenario: Invalid Step — no digit
- **WHEN** text is `Step ` (space after prefix, no digit)
- **THEN** no STEP_TYPE match is recorded

#### Scenario: Invalid Step — letter after digits
- **WHEN** text is `Step1init`
- **THEN** no STEP_TYPE match is recorded

---

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

