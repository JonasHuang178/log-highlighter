## Requirements

### Requirement: Parse time displayed in Notepad++ status bar
After every successful `ParseLog()` invocation (not cancelled), the plugin SHALL write a timing string to the Notepad++ status bar using:
```cpp
SendMessage(g_nppData._nppHandle, NPPM_SETSTATUSBAR,
            STATUSBAR_DOC_TYPE, (LPARAM)wstr);
```

`STATUSBAR_DOC_TYPE = 0` targets the leftmost status bar field (where NPP normally shows "Normal text file", "Unix (LF)", etc.).

---

### Requirement: Timing scope
`t0` is captured immediately after `InitStyles(hSci)` — before `SCI_GETLINECOUNT` and before any parse work. `t1` is captured after the Overview Panel `Update()` call completes. The elapsed time thus covers:

1. Buffer snapshot (large-file path) or direct parse (small-file path)
2. Parser scan (`ScanBuffer` / `ParseDocument`)
3. `ClearAllHighlights`
4. `ApplyHighlightsInRange` for the visible viewport
5. `g_overviewPanel.Update` (building panel marks + NCA redraw)

---

### Requirement: Adaptive time format
The format depends on the magnitude of `elapsed` (in seconds):

| Condition | Format | Example |
|---|---|---|
| `elapsed < 60 s` (no minutes) | `log-highlighter: parsed in %.3f s` | `log-highlighter: parsed in 0.042 s` |
| `60 s ≤ elapsed < 3600 s` (has minutes) | `log-highlighter: parsed in MM:SS.mmm` | `log-highlighter: parsed in 01:05.234` |
| `elapsed ≥ 3600 s` (has hours) | `log-highlighter: parsed in HH:MM:SS.mmm` | `log-highlighter: parsed in 01:02:03.456` |

Where `mmm` is the millisecond component (0–999). Hours, minutes, and seconds are zero-padded to 2 digits.

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
Timing SHALL use `std::chrono::steady_clock` (monotonic clock, unaffected by system time changes).
