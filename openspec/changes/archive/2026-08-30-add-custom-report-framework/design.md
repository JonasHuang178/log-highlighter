## Context

`Parse Log` (Ctrl+Alt+Q) scans the active document with an Aho-Corasick automaton built from the
three rule tables in `config/LogPatterns.h`, producing a `std::vector<Match>` that is cached per
buffer in `g_bufferStates` and used for two things: filling Scintilla indicators and building
Overview Panel marks. `Next Bookmark` (Ctrl+Alt+W) reuses that same cached vector, which is why
it does nothing until Parse Log has run.

The plugin's extension model is compile-time: users edit a header under `config/` and rebuild.
`LogPatterns.h` and `OverviewConfig.h` are both pure data tables, and the shipped entries double
as the documentation. This change extends that model to code rather than data, which is a new
kind of surface for this codebase and drives most of the decisions below.

Two existing implementation details are load-bearing here. `ParseDocument` copies the document
into a local buffer before scanning, so the scan survives edits made while the progress callback
pumps messages — the report engine needs the same snapshot for a different reason (pointer
stability). And the progress callback drains the message queue with `PeekMessage` every 500
lines, which is what keeps the progress dialog painted; the report engine reuses that mechanism.

## Goals / Non-Goals

**Goals:**

- A user can add a report by writing one function and one table row in `config/CustomReports.h`
- A report function never sees `HWND`, `SendMessage`, `SCI_*`, `Match`, or `windows.h`
- The framework predicts nothing about what a user wants to extract
- Report output is displayed in a form that can be read and copied — monospace, scrollable,
  selectable
- Ctrl+Alt+Q, Ctrl+Alt+W and Ctrl+Alt+E are mutually independent; no command requires another
  to have run first
- A stale cache can never produce a report that describes the previous version of the document

**Non-Goals:**

- Crash isolation for report functions (see Decision 9)
- A console harness for testing report functions outside Notepad++
- Any declarative pattern table for extraction (see Decision 3)
- Re-running a report automatically when the document changes; Ctrl+Alt+E stays the only trigger

## Decisions

### Decision 1: Three independent commands, each with its own cached scan

**Choice**: Ctrl+Alt+Q, Ctrl+Alt+W and Ctrl+Alt+E each perform their own scan on first use and
cache the result per buffer. None of them reads another's results as a precondition.

```
per-buffer state (g_bufferStates, keyed by NPP buffer ID)
  ├─ matches[]        filled by Ctrl+Alt+Q   -> indicators + Overview Panel
  ├─ bookmarkLines[]  filled by Ctrl+Alt+W   -> navigation targets
  ├─ reportCache      filled by Ctrl+Alt+E   -> last rendered report text
  └─ stale            set by SCN_MODIFIED    -> invalidates all three
```

**Rationale**: An ordering dependency between commands is invisible in the UI and produces the
"nothing happened" failure that Ctrl+Alt+W has today. Independence costs one extra document
traversal per command, and a traversal is the cheap half of Parse Log — the expensive half is
`SCI_INDICATORFILLRANGE`, which neither W nor R performs. Caching means the traversal is paid
once per edit, not once per keypress, so repeated Ctrl+Alt+W still cycles instantly.

**Free optimization**: Ctrl+Alt+Q already visits every `BOOKMARK_RULES` hit, so it populates
`bookmarkLines[]` as a side effect. This creates no dependency in either direction — Ctrl+Alt+W's
rule is "scan if the cache is empty", and Ctrl+Alt+Q merely sometimes makes it non-empty first.

### Decision 2: `ReportApi.h` is a firewall

**Choice**: `config/CustomReports.h` includes exactly one plugin header, `src/ReportApi.h`, which
declares `ReportContext`, `ReportBuilder`, `Hit`, and the string helpers using only
`std::string_view`, `int`, `size_t` and `const wchar_t*`. No Win32 or Scintilla type crosses it.

```
config/CustomReports.h     user code, the only file a report author edits
        |  #include "../src/ReportApi.h"
        v
src/ReportApi.h            std::string_view / int only
        v
src/Report.cpp             snapshot, cache, progress, encoding, invocation
src/ReportDialog.cpp       modal output window
```

**Rationale**: "Easy to write" is definable — it means the author's file contains only statements
about their log, and none about Notepad++. The firewall is what makes that testable as a
property: if a `HWND` ever appears in `CustomReports.h`, the design has failed.

### Decision 3: Function registration table, not a pattern table

**Choice**: `CUSTOM_REPORTS[]` maps a menu title to a function pointer and an optional shortcut
letter. There is no table describing *what to extract*.

An anchor-based `EXTRACT_RULES[]` (`{ "IP: ", L"IP", " " }`) was designed and rejected. It would
have let the extraction patterns join the existing Aho-Corasick automaton, making the scan free,
and it required no C++ from the author. It was rejected because any fixed schema is the framework
predicting what users care about; `"IP: "` is expressible but "steps that took longer than 500ms"
is not, and an author who hits that wall has nowhere to go.

**Rationale**: Registration is unavoidable — the menu has to be built from something — but it
describes *plumbing*, not *content*, so it predicts nothing. A `constexpr` count from
`sizeof(CUSTOM_REPORTS)` also sizes the `FuncItem` array at compile time, matching how
`log-highlighter.cpp` derives indicator bases from the rule tables.

A self-registering macro (`REPORT("IP Report", 'R') { ... }`) was considered and rejected: it
hides the list of reports, whereas a table shows an author every existing report the moment they
open the file, and it is the shape this project's users are already trained on.

### Decision 4: Three access tiers, chosen by the author rather than imposed

**Choice**: `ReportContext` exposes the document three ways, and the author picks:

| Tier | API | Cost | Progress / cancel |
|---|---|---|---|
| 1 | `ctx.Lines()` | one `memchr` split + whatever the author does per line | yes |
| 2 | `ctx.FindAll(kw)` / `ctx.FindAll({a,b,c})` | one Aho-Corasick pass, independent of keyword count | yes |
| 3 | `ctx.text`, `ctx.length` | author's problem | no |

**Rationale**: This recovers the performance benefit of the rejected pattern table without its
cost. `FindAll` is a function the author *may* call, not a table they *must* fill in, so the fast
path exists without the framework predicting anything. Tier 1 remains the default because it is
the one an author can use without learning anything; tier 3 guarantees no requirement is ever
inexpressible.

### Decision 5: Line semantics are fixed in the author's favour

**Choice**:

| Aspect | Decision |
|---|---|
| Line numbers | 1-based |
| Trailing `\r` | stripped |
| Line ending | never included in `line` |
| Empty lines | still yielded |
| Final line without a newline | still yielded |

**Rationale**: 1-based diverges from Scintilla's 0-based convention used everywhere else in the
codebase, and the conversion happens once at the firewall. The trade is deliberate: an author's
frame of reference is the Notepad++ margin, and a 0-based API guarantees every author writes
`lineNo + 1` forever, with the forgotten case producing a plausible-looking wrong answer. Empty
lines must be yielded for the same reason — skipping them silently desynchronizes every
subsequent line number from the editor.

### Decision 6: Every helper fails to empty, never out of range

**Choice**: `After`, `Before`, `Between`, `Field` and `Trim` return an empty `string_view` when
they cannot produce a result, and empty input yields empty output. `ToInt` / `ToDouble` return
`bool` and leave their out-parameter untouched on failure. No helper throws, and none reads
outside the snapshot.

```cpp
auto ip = Field(After(line, "IP: "), ' ', 0);
if (ip.empty()) continue;               // one check covers both steps
```

**Rationale**: This is what makes helpers composable — an author does not check each level, so
extraction reads as one expression instead of a nest of guards. `ToInt` deliberately does not
return `int`: log files always contain malformed data, and forcing the author to acknowledge
failure beats handing them a silent `0`.

With Decision 9 (no crash guard) this contract is also the primary safety mechanism. An author
who stays on `ctx.Lines()`, `ctx.FindAll()` and the helpers cannot construct an out-of-range
access, so the contract must hold without exception.

### Decision 7: `string_view` lifetime is guaranteed for the whole call

**Choice**: `Report.cpp` snapshots the document into a contiguous buffer before invoking the
report function and releases it after the function returns. Every `string_view` produced by
`ctx` or by a helper points into that snapshot.

**Rationale**: Aggregation is what a ten-line report actually needs — distinct values, counts,
first occurrences — and that means `std::map<std::string_view, int>` and
`std::vector<std::string_view>`. Without a stated guarantee, authors defensively copy into
`std::string` everywhere, which is slower and noisier. The guarantee has to be documented at the
top of `CustomReports.h`, not merely be true, or nobody will rely on it.

### Decision 8: Modal dialog with a read-only multiline `EDIT`

**Choice**: `ReportDialog` is a modal window running its own message loop, containing a
`ES_MULTILINE | ES_READONLY | WS_VSCROLL` edit control in Consolas.

**Rationale**: `MessageBoxW` was the original request and is rejected on three grounds:
proportional font destroys the column alignment that makes a report readable, there is no
scrolling at all, and long text is truncated. Copy support is *not* one of the grounds —
`MessageBoxW` already supports Ctrl+C for the whole body.

Modal rather than modeless because the user does not need to read the report while scrolling the
log. Modality also removes a real hazard: Notepad++ runs its accelerator table before dispatching
messages, so a modeless dialog needs `NPPM_MODELESSDIALOG` registration and it is still uncertain
whether Ctrl+C reaches the edit control rather than Notepad++'s own Copy command. A modal loop
sidesteps the question entirely.

Copy therefore has two independent paths: the edit control's built-in right-click menu (Copy and
Select All are enabled even when read-only) and Ctrl+C. No Copy button is added.

`EM_SETLIMITTEXT` with `wParam = 0` is still sent despite reports being short — the default
32767-character limit truncates silently, and lifting it costs one line.

### Decision 9: No SEH wrapper

**Choice**: The report function is called directly. A fault in author code terminates Notepad++.

An `InvokeGuarded` wrapper using `__try` / `__except (EXCEPTION_EXECUTE_HANDLER)` was designed
and rejected by the project owner. For the record, the C2712 restriction that motivated the
rejection applies only to the function *containing* `__try`; author code called through a
function pointer is unaffected and can use `std::string` and the rest of the STL freely either
way.

**Consequence**: the empty-on-failure contract in Decision 6 becomes the only safety mechanism,
and the debugging path must be documented — build Debug x64, attach Visual Studio to
`notepad++.exe`, set a breakpoint in the report function. Both go in `CustomReports.h`'s header
comment and in the README.

### Decision 10: Progress and cancel are driven from inside the iterators

**Choice**: `ctx.Lines()` and `ctx.FindAll()` tick the existing `ProgressDialog` every 500 lines
and pump messages with `PeekMessage`, matching `Parser.cpp`. On cancel, the iterator compares
equal to `end()`, the author's loop terminates normally, the function returns, and the engine
discards the output without showing a dialog.

**Rationale**: The author writes an ordinary range-for and gets a working progress bar and a
working Cancel button without knowing either exists. Terminating through `end()` rather than an
exception or a longjmp keeps the author's code on its normal path — any statements after the loop
still run, which is harmless because the output is discarded. Tier 3 (`ctx.text`) cannot be
instrumented and shows an indeterminate message; that is the stated cost of the escape hatch.

### Decision 11: Encoding is converted once, at the firewall

**Choice**: Report functions see raw document bytes as `string_view`. `Report.cpp` queries
`SCI_GETCODEPAGE` and converts the finished report text to UTF-16 with `CP_UTF8` (code page
65001) or `CP_ACP` (code page 0) before handing it to the edit control. Additionally, `/utf-8` is
added to the project's compiler options.

**Rationale**: Without `/utf-8`, MSVC reads a BOM-less source file in the system ANSI code page,
so `After(line, "錯誤: ")` on a zh-TW machine compiles to Big5 bytes and can never match a UTF-8
document. The failure is silent, has no diagnostic, and only appears the first time somebody uses
a non-ASCII keyword. `/utf-8` plus saving `CustomReports.h` as UTF-8 with BOM makes literals
match the buffer. The existing rule tables are unaffected because their keywords are ASCII.

### Decision 12: `ReportBuilder` aligns per section

**Choice**: `KV` / `KVf` / `AtLine` entries are buffered and the key column width is resolved when
the section ends, so each `Section` aligns independently. The engine prepends a header with the
report title, file name and line count, and substitutes `(no output)` when a report writes
nothing.

**Rationale**: Alignment is the one piece of formatting that is tedious by hand and identical for
every author, so the framework should own it. Automatic headers mean every report carries its
context without the author writing boilerplate, and every report in the plugin looks the same —
worth having when reports are written by different people. `(no output)` distinguishes "found
nothing" from "the feature is broken", which an empty window does not.

## Risks / Trade-offs

- **[A report function crashes Notepad++]** → Accepted per Decision 9. Mitigated by the
  never-out-of-range helper contract, by shipping examples that only use safe APIs, and by
  documenting the debugger attach procedure. Not mitigated for authors who use tier 3.
- **[Ctrl+Alt+E collides with a Notepad++ built-in]** → Plugin shortcuts lose silently against
  built-ins. Verify on first build that Ctrl+Alt+E triggers the command; if it is taken, register
  no default and document assignment through Settings > Shortcut Mapper.
- **[Notepad++ cannot assign a shortcut to a command registered with `_pShKey = nullptr`]** →
  Affects reports listed with shortcut `0`. Verify on first build; if unsupported, every entry in
  `CUSTOM_REPORTS[]` needs a default letter.
- **[First Ctrl+Alt+E on a very large file is slow]** → One traversal, no indicator fill, cached
  afterwards, with progress and cancel. Acceptable; measure before optimizing further.
- **[Folding report scanning into Ctrl+Alt+Q to avoid the extra traversal]** → Deliberately not
  done. It would optimize steady-state use while making the authoring loop worse, since every
  tweak to a report would require a full re-parse *and* re-highlight. Because the API is defined
  in terms of what the author calls rather than when the scan happens, this can be revisited
  later as a purely internal change.
- **[`CustomReports.h` included more than once]** → It defines functions, not just data, so a
  second inclusion produces duplicate-symbol errors whose message does not point at the cause.
  Stated in the file's header comment; only `Report.cpp` includes it.
