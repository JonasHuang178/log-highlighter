## Requirements

### Requirement: Lazy indicator application after Ctrl+Alt+Q
After parsing, Scintilla indicators SHALL be applied only for the visible viewport plus a lookahead of `APPLY_LOOKAHEAD_LINES = 1000` lines. The remaining off-screen document is deferred and applied progressively as the user scrolls.

This is necessary because each `SCI_INDICATORFILLRANGE` call causes Scintilla to compute pixel coordinates for the affected range and invalidate a screen rectangle. For documents with tens of thousands of matches, applying all indicators synchronously would take several seconds.

`g_appliedByteEnd` tracks the furthest byte position up to which indicators have been applied. It is reset to `-1` on every new parse or cancel.

#### Scenario: Instant response on large file
- **WHEN** the user presses Ctrl+Alt+Q on a file with 270 000 lines and 64 000 matches
- **THEN** the visible area is highlighted within ~1 second; the status bar shows the total parse time

#### Scenario: Off-screen area highlighted on scroll
- **WHEN** the user scrolls to a previously unseen section
- **THEN** `SCN_UPDATEUI` fires and `ApplyHighlightsInRange` applies indicators for the newly visible area

---

### Requirement: GetLookaheadByteEnd
The target byte end for each apply operation SHALL be computed as:
```
endLine = clamp(firstVisibleLine + linesOnScreen + APPLY_LOOKAHEAD_LINES,
                0, totalLines - 1)
byteEnd = SCI_GETLINEENDPOSITION(endLine)
```

---

### Requirement: SCN_UPDATEUI extension
On every `SCN_UPDATEUI` notification, if `g_highlightActive` is true and `g_matches` is non-empty:
1. Compute `needed = GetLookaheadByteEnd(hSci)`.
2. If `needed > g_appliedByteEnd`:
   - Call `ApplyHighlightsInRange(hSci, g_matches, g_appliedByteEnd + 1, needed + 1, repaintAfter=false)`.
   - Set `g_appliedByteEnd = needed`.

`repaintAfter = false` because Scintilla repaints itself after `SCN_UPDATEUI`.

#### Scenario: No extra work when viewport is already covered
- **WHEN** the user moves the cursor within the already-applied region
- **THEN** `needed <= g_appliedByteEnd` and `ApplyHighlightsInRange` is not called

---

### Requirement: ApplyHighlightsInRange
`ApplyHighlightsInRange(hSci, matches, fromByte, toByteExclusive, repaintAfter)` SHALL:
1. Use `std::lower_bound` on `matches` (sorted by `byteOffset`) to find the first match in range — O(log N).
2. Iterate matches while `byteOffset < toByteExclusive`.
3. For each match call `SCI_SETINDICATORCURRENT` + `SCI_INDICATORFILLRANGE`.
4. Suppress `SCN_MODIFIED(SC_MOD_CHANGEINDICATOR)` notifications during the fill by temporarily masking the event flag via `SCI_SETMODEVENTMASK`.
5. Restore the original event mask after filling.
6. If `repaintAfter = true`, call `RedrawWindow(RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE)`.

---

### Requirement: SC_MOD_CHANGEINDICATOR suppression
Each `SCI_INDICATORFILLRANGE` normally triggers a `SCN_MODIFIED(SC_MOD_CHANGEINDICATOR)` notification sent synchronously back to NPP via `WM_NOTIFY`. Even if the plugin's `beNotified` handler ignores this notification, the round-trip adds measurable overhead per fill.

Both `ApplyHighlights` and `ApplyHighlightsInRange` SHALL suppress this notification by:
```cpp
LRESULT mask = Sci(hSci, SCI_GETMODEVENTMASK, 0, 0);
Sci(hSci, SCI_SETMODEVENTMASK, mask & ~SC_MOD_CHANGEINDICATOR, 0);
// ... fills ...
Sci(hSci, SCI_SETMODEVENTMASK, mask, 0);
```

`SC_MOD_CHANGEINDICATOR = 0x4000`. `SCI_GETMODEVENTMASK = 2378`. `SCI_SETMODEVENTMASK = 2359`. These constants are not present in the bundled `Scintilla.h` and are defined locally in `log-highlighter.cpp`.

---

### Requirement: Indicator index layout
Indicators are allocated starting at index 11 to avoid conflicts with Notepad++'s built-in indicators (0–10):

```
11 .. (11 + LOG_RULE_COUNT  - 1)  →  LOG_TYPE  indicators  (INDIC_TEXTFORE)
(11 + LOG_RULE_COUNT) .. ...      →  STEP_TYPE indicators  (INDIC_FULLBOXBLOCK)
```

With the default 6 LOG_TYPE rules and 1 STEP_TYPE rule: indices 11–16 are used.

LOG_TYPE uses `INDIC_TEXTFORE` (foreground color only).
STEP_TYPE uses `INDIC_FULLBOXBLOCK` with `INDIC_SETUNDER=TRUE` and `INDIC_SETALPHA=255` (fully opaque background, rendered under the text layer).

---

### Requirement: SCN_MODIFIED full apply
The `SCN_MODIFIED` real-time re-highlight path (user typing/pasting) SHALL call `ApplyHighlights` (full document, not range-limited) because:
- Documents that trigger `SCN_MODIFIED` are typically small enough that a full apply is fast.
- After a text change, `g_appliedByteEnd` is set to `SCI_GETLENGTH` to reflect that indicators cover the entire document.

#### Scenario: Real-time re-highlight after typing
- **WHEN** the user types `[ ERROR ]` in the document after Ctrl+Alt+Q has been pressed
- **THEN** the keyword is highlighted immediately; `g_appliedByteEnd` is updated to the document length

---

### Requirement: Overview Panel unaffected by lazy rendering
The Overview Panel reads directly from the in-memory `g_matches` vector (set by `BuildPanelMarks`). It does not depend on which byte ranges have had indicators applied. Panel marks for the entire document appear immediately after parsing, even before off-screen indicators are applied.
