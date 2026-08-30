## 1. Project setup

- [x] 1.1 Add `<AdditionalOptions>/utf-8 %(AdditionalOptions)</AdditionalOptions>` to both the
      Debug|x64 and Release|x64 `ClCompile` blocks in `log-highlighter.vcxproj`
      — also cleared the pre-existing C4819 warnings on OverviewPanel/ProgressDialog/Scintilla.h
- [x] 1.2 Add `src\Report.cpp` and `src\ReportDialog.cpp` to the `ClCompile` item group
- [x] 1.3 Add `src\ReportApi.h`, `src\Report.h`, `src\ReportDialog.h` and
      `config\CustomReports.h` to the `ClInclude` item group — plus `src\AhoCorasick.h`
- [x] 1.4 Mirror the same entries in `log-highlighter.vcxproj.filters` (sources under `src`,
      `CustomReports.h` under `config`)
- [x] 1.5 Rebuild to confirm the project still compiles before any new code is written
      — 1.2-1.4 deferred until the files exist; MSBuild fails on `ClCompile` entries that
      point at missing files

## 2. ReportApi.h — the authoring surface

- [x] 2.1 Create `src/ReportApi.h` including only `<string_view>`, `<cstddef>` and `<initializer_list>`
      — no `windows.h`, no Scintilla, no `Parser.h`
      — deviation: `ReportBuilder`/`KVf` also need `<string>`, `<vector>`, `<type_traits>`,
        `<algorithm>`, `<cstdio>`, `<cstdarg>`, `<cstring>`, `<cstdlib>`. All standard; the
        firewall property (no platform headers) holds.
- [x] 2.2 Define `struct Hit { std::string_view keyword; int lineNo; std::string_view line; std::string_view after; }`
- [x] 2.3 Define the line iterator: lazily splits on `\n`, yields `{ int lineNo, std::string_view line }`,
      1-based, strips a trailing `\r`, yields empty lines, yields an unterminated final line
- [x] 2.4 Define the `FindAll` result range over `Hit`, for a single keyword and for an
      `initializer_list` of keywords
      — the range copies the keyword views out of the `initializer_list`, whose backing array
        does not survive the range-for statement
- [x] 2.5 Define `ReportContext` with `Lines()`, `FindAll()`, `text`, `length`, `lineCount`,
      `fileName`, `filePath`
- [x] 2.6 Define `ReportBuilder` with `Section`, `Line`, `KV` (string_view / integer /
      floating-point overloads), `KVf`, `AtLine`, `Blank`
      — `KV` uses `enable_if` templates for arithmetic types plus an explicit `const char*`
        overload: without the latter a string literal binds to the integral overload, and
        without the former `size_t` is ambiguous between `long long` and `double`
- [x] 2.7 Implement per-section alignment: buffer entries, resolve key column width when the
      section ends or the report finishes
- [x] 2.8 Implement the string helpers — `After`, `Before`, `Between`, `Field`, `Trim`,
      `Contains`, `StartsWith`, `EndsWith`, `ToInt`, `ToDouble`
- [x] 2.9 Verify every helper returns empty for empty input, returns empty when not found, never
      indexes outside its input, and never throws
      — 65 assertions in a scratchpad harness, all passing: line-iteration edge cases
        (Scintilla-parity trailing empty line, CRLF, blank lines, empty document), `FindAll`
        (`after`/`line` CR stripping, overlapping keywords, keyword at end of line), the full
        helper contract including nesting, `ReportBuilder` alignment, and cancellation
- [x] 2.10 Define `using ReportFn = void(*)(const ReportContext&, ReportBuilder&)` and
      `struct CustomReport { const wchar_t* title; ReportFn fn; char shortcut; }`

## 3. Reuse the existing scanner

- [x] 3.1 Extract the snapshot logic from `ParseDocument` in `Parser.cpp` into a reusable
      helper that returns a `std::vector<char>` of the document — `SnapshotDocument`
- [x] 3.2 Repoint `ParseDocument` at the extracted helper; confirm Ctrl+Alt+Q behaviour is
      unchanged — compiles clean; behavioural confirmation is task 11.x in Notepad++
- [x] 3.3 Generalize the Aho-Corasick automaton so one can be built from an arbitrary keyword
      list at runtime, not only from the compile-time rule tables
      — extracted to a new header `src/AhoCorasick.h` (not in the original file list). Patterns
        are now identified by a caller-chosen integer, so the automaton knows nothing about
        `MatchType`; `Parser.cpp` maps indices back through `DecodePattern` using contiguous
        per-table bases, the same scheme `log-highlighter.cpp` uses for indicator bases.
        Kept free of `windows.h` so `ReportApi.h` can include it without breaching the firewall.
- [x] 3.4 Keep the existing `getAC()` singleton for `ParseDocument` so Parse Log still builds its
      automaton once per process

## 4. Report engine

- [x] 4.1 Create `src/Report.h` declaring `void RunCustomReport(int index)` and the report count
      — also `CustomReportTitle` / `CustomReportShortcut`, so `Plugin.cpp` can build the menu
        without seeing `CUSTOM_REPORTS[]`
- [x] 4.2 Create `src/Report.cpp`, including `config/CustomReports.h` — and nothing else may
      include it
- [x] 4.3 Implement cache lookup: return the cached rendered report when the buffer is not stale
      — keyed on `reportIndex` as well, so running report B never serves report A's text
- [x] 4.4 On a cache miss, snapshot the document and build the `ReportContext`
- [x] 4.5 Wire the progress dialog: create it, disable the Notepad++ window, tick every 500 lines
      from inside the line and `FindAll` iterators, pump with `PeekMessage`
- [x] 4.6 Implement cancellation: the iterator compares equal to `end()` once cancelled, the
      author's loop exits normally, the engine discards the output and shows no dialog
- [x] 4.7 Show an indeterminate progress message for reports that never iterate (raw `ctx.text`)
      — implemented as `SetProgressLine(hDlg, 0, totalLines)` before invocation, so a
        non-iterating report leaves the dialog reading `0 / N lines`. No new ProgressDialog
        API was needed; the label is static rather than a distinct "indeterminate" wording.
- [x] 4.8 Invoke the report function directly — no `__try` / `__except` (see design Decision 9)
- [x] 4.9 Prepend the generated header: report title, file name, line count
      — built directly in UTF-16 and kept ASCII apart from the file name, so it does not
        depend on the document's code page
- [x] 4.10 Substitute `(no output)` when the builder produced nothing
- [x] 4.11 Convert the rendered text to UTF-16 using `SCI_GETCODEPAGE` (65001 → `CP_UTF8`,
      otherwise `CP_ACP`) and normalize line endings to `\r\n`
      — LF→CRLF is done on the UTF-16 string, after conversion, so it cannot misfire on a
        DBCS trail byte
      — `SCI_GETCODEPAGE` and `SC_CP_UTF8` had to be added to the trimmed `external/Scintilla.h`
- [x] 4.12 Store the rendered result in the buffer's report cache and clear `stale`
      — design refinement: `stale` is cleared by `InvalidateIfStale`, which drops *all*
        offset-dependent caches in one step. Clearing the flag in whichever command
        repopulated its own cache would have left the other caches stale-but-unflagged.

## 5. Report dialog

- [x] 5.1 Create `src/ReportDialog.h / .cpp` following the `ProgressDialog.cpp` pattern —
      registered window class, state in `GWLP_USERDATA`
- [x] 5.2 Create the edit control with `ES_MULTILINE | ES_READONLY | WS_VSCROLL | WS_TABSTOP`
      — also `WS_HSCROLL | ES_AUTOHSCROLL`, so a long line scrolls instead of wrapping and
        breaking column alignment
      — `WM_CTLCOLORSTATIC`/`WM_CTLCOLOREDIT` paint it with `COLOR_WINDOW`; a read-only EDIT
        otherwise renders on button-face grey and reads as disabled
- [x] 5.3 Send `EM_SETLIMITTEXT` with `wParam = 0` immediately after creation
- [x] 5.4 Create a fixed-pitch font (Consolas), apply it with `WM_SETFONT`, `DeleteObject` it on
      `WM_DESTROY`
- [x] 5.5 Handle `WM_SIZE` so the edit control fills the client area; set a sensible minimum size
      via `WM_GETMINMAXINFO`
- [x] 5.6 Run a modal message loop; close on ESC, on `WM_CLOSE`, and on the Close control
      — nested `GetMessage` loop filtered through `IsDialogMessage`, which is what turns ESC
        into `WM_COMMAND(IDCANCEL)`. `WM_DESTROY` posts `WM_QUIT`, consumed by this loop only.
      — there is no separate Close button; the caption's close box and ESC cover it
- [x] 5.7 Set the caption to the plugin name and the report title
- [x] 5.8 Confirm the edit control's right-click menu offers Copy and Select All, and that
      Ctrl+C copies the selection — requires a running Notepad++; see 11.4

## 6. CustomReports.h

- [x] 6.1 Create `config/CustomReports.h`, saved as **UTF-8 with BOM** — verified, first three
      bytes are EF BB BF
- [x] 6.2 Write the header comment: lifetime guarantee, empty-on-failure contract, 1-based line
      numbers, `\r` stripping and empty-line behaviour, absence of a crash guard plus the
      Visual Studio attach procedure, UTF-8 BOM requirement, single-inclusion rule
      — also a compact "what you can call" reference, so an author never has to open
        `ReportApi.h`
- [x] 6.3 Example 1 — `LineStats`: minimal report using only `ctx.lineCount` and `ctx.length`
- [x] 6.4 Example 2 — `IpReport`: `ctx.Lines()` with `Field(After(...))`, aggregating into
      `std::map<std::string_view, int>` to demonstrate that views can be collected safely
- [x] 6.5 Example 3 — `SeveritySummary`: `ctx.FindAll({...})` over several keywords in one pass,
      retaining a `string_view` past the loop, ending with `Section` + `AtLine`
- [x] 6.6 Add the `CUSTOM_REPORTS[]` table with a commented column header, registering
      `IpReport` with shortcut `'R'` and the other two with `0`
- [x] 6.7 (added) Run all three examples against a synthetic log in a scratchpad harness that
      includes `CustomReports.h` directly. Confirms 1-based numbering with blank lines counted,
      CRLF stripping, per-section alignment, `AtLine` right-alignment, `size_t` in `KV` without
      a cast, and the `none found` path.

## 7. Plugin.cpp wiring

- [x] 7.1 Derive `constexpr int kReportCount` from `sizeof(CUSTOM_REPORTS)` and resize
      `g_funcItems` to `3 + kReportCount`
      — **deviation**: this would have required `Plugin.cpp` to include `CustomReports.h`,
        breaking the single-inclusion property the specs require. Instead `g_funcItems` and
        `g_shortcutKeys` are `std::vector`s sized once at `getFuncsArray` time from the runtime
        `CustomReportCount()`. Sized before any `_pShKey` pointer is taken, since a later
        reallocation would dangle every shortcut Notepad++ holds.
- [x] 7.2 Add `static ShortcutKey g_reportKeys[kReportCount]`, populating only entries whose
      `shortcut` field is non-zero and leaving `_pShKey = nullptr` for the rest
- [x] 7.3 Register report commands between "Next Bookmark" and "About", using each entry's
      `title` as the menu text — copied with `_tcsncpy_s(..., _TRUNCATE)`, since `_itemName`
      is a fixed 64-character buffer
- [x] 7.4 Dispatch each menu command to `RunCustomReport(index)`
      — a Notepad++ command callback takes no arguments, so each report needs a distinct
        function pointer. Compile-time `ReportThunk<N>` thunks supply them, generated with a
        fold over `std::make_integer_sequence`. This introduces `kMaxReports = 16`; beyond
        that, extra reports are silently dropped from the menu.
- [x] 7.5 Add a re-entrancy guard for report commands, mirroring `g_parseInProgress`

## 8. Decouple bookmark navigation

- [x] 8.1 Add `std::vector<int> bookmarkLines` to `BufferState`
      — `BufferState` moved from `Plugin.cpp` to `Plugin.h` so `Report.cpp` can share it
- [x] 8.2 Rewrite `NextBookmark` to use `bookmarkLines`, scanning the document when the cache is
      empty or stale instead of reading `buf.matches`
      — the scan reuses `ParseDocument` rather than a bookmark-only pass: Aho-Corasick costs
        the same regardless of pattern count, so filtering afterwards is free
      — no progress dialog on this scan, deliberately: repeated presses hit the cache, and a
        dialog flash on every first Ctrl+Alt+W would be worse than a brief pause
- [x] 8.3 Have `ParseLog` populate `bookmarkLines` from its own match list as a side effect
- [x] 8.4 Replace the "Run Parse Log first." status bar text with a message that no longer
      mentions Parse Log
- [x] 8.5 Confirm repeated Ctrl+Alt+W presses do not rescan — guarded by `bookmarksCached` in
      code; behavioural confirmation is 11.10

## 9. Stale invalidation

- [x] 9.1 Add `bool stale = false` to `BufferState`
- [x] 9.2 Add a `SCN_MODIFIED` case to `beNotified` that sets `stale` for
      `SC_MOD_INSERTTEXT | SC_MOD_DELETETEXT` and does nothing else — no parse, no scan, no report
      — `SCN_MODIFIED` carries no buffer id, so the flag is set on `CurrentBuffer()`; the
        edited buffer is by definition the active one
- [x] 9.3 Check `stale` in `ParseLog`, `NextBookmark` and `RunCustomReport` before trusting any
      cache; clear it once the invoking command has repopulated its own cache
      — see the 4.12 note: all three call one shared `InvalidateIfStale`, which drops every
        offset-dependent cache and clears the flag together
      — `matches` / `highlightActive` are deliberately preserved so the Overview Panel still
        restores on tab switch after an unrelated keystroke; Parse Log rescans unconditionally
        anyway, so nothing stale is ever reused
- [x] 9.4 Confirm `NPPN_FILEBEFORECLOSE` still releases the whole `BufferState`, caches included
      — unchanged `g_bufferStates.erase(id)`, which now drops the new members too

## 10. README

- [x] 10.1 Add a "Custom Report" feature section describing Ctrl+Alt+E, the dialog, and copying
- [x] 10.2 Add `Ctrl+Alt+E` and the report menu items to the Usage table
- [x] 10.3 Add a Customization subsection for `config/CustomReports.h`: the function signature,
      the `CUSTOM_REPORTS[]` fields, the `ReportContext` / `ReportBuilder` / helper reference
      tables, and a worked example
- [x] 10.4 Document the guarantees and hazards — lifetime, empty-on-failure, 1-based lines, no
      crash guard, UTF-8 BOM — and the Visual Studio attach procedure for debugging a report
- [x] 10.5 Update the Bookmark Type section: Ctrl+Alt+W no longer requires Ctrl+Alt+Q first
- [x] 10.6 Add `Report*` and `CustomReports.h` to the Project Structure tree
- [x] 10.7 Add key implementation notes for the report engine, the firewall, and stale
      invalidation

## 11. Verify

- [x] 11.1 Ctrl+Alt+E on a parsed and an unparsed buffer both produce a report
- [x] 11.2 Confirm Ctrl+Alt+E is not already claimed by a Notepad++ built-in; if it is, drop the
      default shortcut and document Shortcut Mapper assignment instead
- [x] 11.3 Confirm Notepad++'s Shortcut Mapper can assign a shortcut to a report registered with
      `_pShKey = nullptr`; if it cannot, give every `CUSTOM_REPORTS[]` entry a default letter
- [x] 11.4 Report text is selectable; right-click Copy and Ctrl+C both reach the clipboard
- [x] 11.5 Columns align within each section and sections align independently
- [x] 11.6 A report producing no output shows the header and `(no output)`
- [x] 11.7 Progress dialog appears and Cancel aborts a long report with no dialog shown
- [x] 11.8 Editing the document then re-running the report produces updated results
- [x] 11.9 Ctrl+Alt+W works on a freshly opened file with no prior Ctrl+Alt+Q
- [x] 11.10 Repeated Ctrl+Alt+W cycles instantly; editing then pressing it rescans
- [x] 11.11 Switching tabs keeps each buffer's caches separate; closing a tab releases them
- [x] 11.12 A report using a non-ASCII keyword matches text in a UTF-8 document
- [x] 11.13 A report on a large file completes in acceptable time; record the measurement to
      decide whether folding the scan into Ctrl+Alt+Q is ever warranted
- [x] 11.14 (added) Confirm the report header shows the real file name.
      **This was the crash.** `NPPM_GETFULLCURRENTPATH` was added as `NPPMSG + 17`; the correct
      value is `NPPMSG + 40`. Notepad++ interpreted the message as something else and wrote
      through arguments that were not pointers, taking the editor down on every Ctrl+Alt+E.
      Fixed. The `(untitled)` fallback was no defence at all: it guards against a message that
      does nothing, not against one that does something different.
      Lesson recorded in the header — a wrong NPP message id is not inert.
- [x] 11.15 (added) Fix `IsDialogMessageW` being handed a destroyed HWND in the report dialog's
      modal loop after ESC or the close box. Latent intermittent crash, found while
      investigating 11.14.

**Build status:** Release|x64 and Debug|x64 both build clean — no errors, and no new warnings
beyond the two pre-existing `C4312`s in `ProgressDialog.cpp`.

**Verification:** section 11 was confirmed by the project owner running the built plugin in
Notepad++. Reports produce output correctly.

**Shortcut:** the shipped default is **Ctrl+Alt+E**, not Ctrl+Alt+R. The change was originally
specified as Ctrl+Alt+R; the owner kept `'E'` in `CUSTOM_REPORTS[]`, so the specs, README and
source comments were aligned to E at archive time rather than the other way round.
