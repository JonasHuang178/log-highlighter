## MODIFIED Requirements

### Requirement: Parse time displayed in Notepad++ status bar
After every successful `ParseLog()` invocation (not cancelled), the plugin SHALL write a timing string to the Notepad++ status bar using:
```cpp
SendMessage(g_nppData._nppHandle, NPPM_SETSTATUSBAR,
            STATUSBAR_DOC_TYPE, (LPARAM)wstr);
```

`STATUSBAR_DOC_TYPE = 0` targets the leftmost status bar field (where NPP normally shows "Normal text file", "Unix (LF)", etc.).

#### Scenario: Timing appears after a completed parse
- **WHEN** Ctrl+Alt+Q completes without cancellation
- **THEN** the leftmost status bar field shows the timing string in place of the document type Notepad++ normally displays there

---

### Requirement: Timing scope
The measured interval SHALL begin immediately after `InitStyles(hSci)` — before
the progress dialog is created and before any parse work — and SHALL end after
the Overview Panel `Update()` call returns. It SHALL therefore cover:

1. Parser scan (`ParseDocument` → `ScanBuffer`), including the document snapshot
2. `ClearAllHighlights`
3. `ApplyHighlights` for all matches in the document
4. `g_overviewPanel.Update` (building panel marks + NCA redraw)

The interval SHALL NOT stop at the end of the scan. Phase 2 dominates the wall
time on a match-dense document, so a figure excluding it would understate the
cost of the command the user actually invoked.

#### Scenario: Timing covers the full apply phase
- **WHEN** a parse produces 64 000 matches and Phase 2 takes most of the wall time
- **THEN** the reported duration includes that apply time, because `t1` is taken after `ApplyHighlights` and the panel update

---

### Requirement: Adaptive time format
The plugin SHALL select the timing format according to the magnitude of `elapsed` (in seconds):

| Condition | Format | Example |
|---|---|---|
| `elapsed < 60 s` (no minutes) | `log-highlighter: parsed in %.3f s` | `log-highlighter: parsed in 0.042 s` |
| `60 s ≤ elapsed < 3600 s` (has minutes) | `log-highlighter: parsed in MM:SS.mmm` | `log-highlighter: parsed in 01:05.234` |
| `elapsed ≥ 3600 s` (has hours) | `log-highlighter: parsed in HH:MM:SS.mmm` | `log-highlighter: parsed in 01:02:03.456` |

Where `mmm` is the millisecond component (0–999). Hours, minutes, and seconds SHALL be zero-padded to 2 digits.

#### Scenario: Sub-second parse
- **WHEN** total elapsed time is 0.042 s
- **THEN** status bar shows `log-highlighter: parsed in 0.042 s`

#### Scenario: Multi-minute parse
- **WHEN** total elapsed time is 65.234 s
- **THEN** status bar shows `log-highlighter: parsed in 01:05.234`

#### Scenario: Cancelled parse
- **WHEN** the user cancels the progress dialog
- **THEN** no timing string is written to the status bar

---

### Requirement: Timing source
Timing SHALL use `std::chrono::steady_clock`, a monotonic clock unaffected by
system time changes, so that a reported duration is never negative and never
absorbs a clock adjustment that happened to land mid-parse.

#### Scenario: System clock adjusted mid-parse
- **WHEN** the system wall clock is changed while a parse is running
- **THEN** the reported duration still reflects elapsed real time and is unaffected by the adjustment
