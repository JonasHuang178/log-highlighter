## Why

The plugin can colorize keywords, but it cannot answer questions about a log. Users routinely
want a small digest of the file they are looking at — how many distinct IP addresses appear,
which step failed first, how many errors per severity — and today the only way to get that is
to read the file by hand.

The shape of those questions is unpredictable. `LogPatterns.h` works because keyword-plus-color
is a fixed shape, but "what do I want to know about this log" has no fixed shape, so a
declarative rule table cannot express it. What users need is a place to write a small amount of
C++ that reads the document and prints whatever they decide matters.

A second problem surfaces alongside this. `Next Bookmark` (Ctrl+Alt+W) currently reads the match
list produced by `Parse Log` (Ctrl+Alt+Q), so it does nothing until the user has parsed the
buffer, and it silently uses stale byte offsets once the document is edited. A report command
built on the same foundation would inherit both defects.

## What Changes

- **Add** `Custom Report` (Ctrl+Alt+E): runs a user-written function against the active
  document and shows its output in a modal, read-only, copyable dialog
- **Add** `config/CustomReports.h` — the single file a user edits to write report functions,
  shipped with three worked examples and the registration table
- **Add** `src/ReportApi.h` — the authoring surface: `ReportContext`, `ReportBuilder`, and the
  string helpers. Contains no Win32 or Scintilla types, so a report function never sees `HWND`,
  `SendMessage`, `SCI_*`, or `Match`
- **Add** `src/Report.cpp` — the engine: snapshot, cache lookup, invocation, progress, encoding
- **Add** `src/ReportDialog.cpp` — the modal output window
- **Change** `Next Bookmark` (Ctrl+Alt+W) to run its own scan instead of reading `Parse Log`
  results, removing the ordering dependency between the two commands
- **Add** a per-buffer `stale` flag driven by `SCN_MODIFIED`, invalidating every cached scan
  when the document changes
- **Add** `/utf-8` to the compiler options so non-ASCII keywords in config headers compile to
  the encoding the document is actually in

## Capabilities

### New Capabilities

- `custom-report`: the report authoring framework — `ReportApi.h`, `CustomReports.h`,
  registration, and the Ctrl+Alt+E command
- `report-dialog`: the modal read-only window that displays report output

### Modified Capabilities

- `bookmark-navigation`: Ctrl+Alt+W performs its own scan and no longer requires Ctrl+Alt+Q
  to have run first
- `per-buffer-state`: gains a bookmark-line cache, a report cache, and `SCN_MODIFIED`-driven
  invalidation shared by all three commands

## Impact

- `config/CustomReports.h`: new — the only file a report author edits
- `src/ReportApi.h`: new — authoring surface, no Win32 types
- `src/Report.cpp`: new — engine
- `src/ReportDialog.h / .cpp`: new — modal output window
- `src/Plugin.cpp`: `FuncItem` array grows by the number of registered reports;
  `NextBookmark` rewritten against its own cache; `SCN_MODIFIED` handler added
- `src/Parser.cpp`: document snapshot logic extracted so `Report.cpp` can reuse it;
  Aho-Corasick automaton generalized so `ctx.FindAll()` can build one from arbitrary keywords
- `log-highlighter.vcxproj` / `.filters`: new source files, `/utf-8` compiler option
- `README.md`: new Custom Report section; the Ctrl+Alt+W description no longer states that
  Parse Log must run first

## Non-Goals

- **No crash guard.** A report function runs in the Notepad++ process with no SEH wrapper, so a
  stray pointer takes down the editor. The framework mitigates this by making its own helpers
  incapable of reading out of range, not by catching faults.
- **No console test harness.** Report functions are debugged by attaching Visual Studio to
  `notepad++.exe`.
- **No declarative extraction table.** Rejected during design: any fixed rule schema predicts
  what users care about, and predicting wrong leaves them with no way forward.
- **No runtime scripting.** Report authors are expected to build the plugin from source.
- **No timestamp arithmetic helpers.** Timestamp formats vary per log; authors compose
  `Between` / `Field` / `ToInt` themselves.
