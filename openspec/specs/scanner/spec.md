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
Each match produces one `Match` entry:

| Field | Type | Description |
|---|---|---|
| `type` | `MatchType` | `LOG_TYPE` or `STEP_TYPE` |
| `ruleIndex` | `int` | Index into `LOG_TYPE_RULES[]` or `STEP_TYPE_RULES[]` |
| `byteOffset` | `intptr_t` | Start byte position in the document |
| `length` | `intptr_t` | Byte length of the highlighted range |

`LOG_TYPE` length = length of the keyword string.
`STEP_TYPE` length = from prefix start to last character of the line (exclusive of newline).

Matches in the returned vector are **sorted by `byteOffset`** (ascending), because `ScanBuffer` scans left-to-right.

---

### Requirement: Progress callback
`ScanBuffer` accepts an optional `std::function<bool(int cur, int total)>` progress callback. The callback is invoked every 500 completed lines and on the final line. Returning `false` from the callback aborts the scan and returns an empty vector.

---

### Requirement: Two ParseDocument overloads

**UI-thread overload** — takes `HWND hScintilla`:
1. Calls `SCI_GETLENGTH` and `SCI_GETCHARACTERPOINTER` to get buffer pointer and length.
2. Copies the buffer into a local `std::vector<char>`.
3. Calls `SCI_GETLINECOUNT`.
4. Calls `ScanBuffer` on the local copy.

Used by the `SCN_MODIFIED` real-time re-highlight path (small/moderate files, UI thread only).

**Worker-thread overload** — takes `const std::vector<char>& docBuf` and `int totalLines`:
1. Calls `ScanBuffer` directly on the pre-snapshotted buffer.
2. Never calls `SendMessage` to Scintilla.

Used by the large-file path where the buffer is snapshotted on the UI thread before the worker thread starts.

#### Scenario: Thread safety
- **WHEN** the worker thread calls `ParseDocument(docBuf, totalLines, progressFn)`
- **THEN** no Scintilla or NPP API calls are made from the worker thread
