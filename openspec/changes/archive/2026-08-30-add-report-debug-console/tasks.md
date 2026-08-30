## 1. Config flag and plumbing

- [x] 1.1 Add `REPORT_DEBUG_MODE` (default `0`) and `REPORT_DEBUG_MAX_LINES` (default `1000`) to
      `config/CustomReports.h`, near the top and above the report examples
- [x] 1.2 Document both in the file's header comment: what they do, that enabling costs a
      rebuild, that arguments to `Debug` are still evaluated when disabled, and that console
      output is slow on large files
- [x] 1.3 Declare `extern bool g_reportDebugEnabled` and `extern int g_reportDebugMaxLines` in
      `src/ReportApi.h`; define them in `src/Report.cpp` from the macros
- [x] 1.4 Add `bool ReportDebugEnabled()` to `src/Report.h` so `Plugin.cpp` can act on the flag
      without including `config/CustomReports.h`
- [x] 1.5 Confirm `config/CustomReports.h` is still included by `src/Report.cpp` only

## 2. Debug output API

- [x] 2.1 Add `void DebugWrite(const char* text, size_t len)` as the single sink — declared in
      `src/ReportApi.h`, implemented in `src/Report.cpp`. No Win32 types cross the header.
- [x] 2.2 Implement `Debug(std::string_view)` — early-returns on `g_reportDebugEnabled`
- [x] 2.3 Implement `Debugf` as `template <class... Args> void Debugf(const char* fmt, Args&&...)`
      — early-returns on the flag before any formatting work
- [x] 2.4 Walk the format string one conversion at a time; format exactly one argument per
      conversion using the argument's known type
- [x] 2.5 Delegate numeric conversions to `snprintf` with the single extracted specifier, so
      width, precision and flags keep working (`%5d`, `%-8s`, `%.2f`, `%x`, `%lld`)
- [x] 2.6 Accept `const char*`, `std::string` and `std::string_view` for `%s`
- [x] 2.7 Handle `%%` and a trailing lone `%` without reading past the format string
- [x] 2.8 On a conversion/argument type mismatch, emit a visible marker instead of the value —
      never an invalid access
- [x] 2.9 On more conversions than arguments, emit the marker for the surplus — never read past
      the pack
- [x] 2.10 Verify the above in a scratchpad harness: `%s` with `string_view` / `std::string` /
      `const char*`, empty `string_view`, width and precision, `%%`, mismatch, too few arguments,
      and that nothing reads out of range

## 3. Console lifecycle

- [x] 3.1 Add `src/DebugConsole.h / .cpp` — allocate, write, and the control handler
- [x] 3.2 `AllocConsole` guarded by `GetConsoleWindow()`, so an already-attached console is
      reused rather than failing
- [x] 3.3 Remove `SC_CLOSE` via `DeleteMenu(GetSystemMenu(GetConsoleWindow(), FALSE), ...)`
- [x] 3.4 Install a `SetConsoleCtrlHandler` that returns TRUE for `CTRL_CLOSE_EVENT`
- [x] 3.5 Set a console title identifying the plugin
- [x] 3.6 Implement writing via `WriteConsoleW`, converting from the active document code page
      (`SCI_GETCODEPAGE`) to UTF-16 — reuse the conversion already in `Report.cpp`
- [x] 3.7 Call `FreeConsole` on plugin unload
- [x] 3.8 Allocate from the `NPPN_READY` handler in `Plugin.cpp`, gated on `ReportDebugEnabled()`
      — **never** from `DllMain`, which runs under the loader lock

## 4. Engine breadcrumbs and cache bypass

- [x] 4.1 Add an internal breadcrumb helper that is exempt from the output cap and visually
      distinct from author output (`[engine] ` prefix)
- [x] 4.2 Breadcrumb the report run: report starting with buffer id and `stale` state, caches
      invalidated, cache bypassed, snapshot size, entering the report function, report function
      returned with row count, dialog shown
- [x] 4.3 Breadcrumb the `stale` transition in the `SCN_MODIFIED` handler — print only on
      `false -> true`, never while the flag is already set
- [x] 4.4 Include the buffer id in both the `SCN_MODIFIED` and report breadcrumbs, so the two can
      be compared (this is what makes task 7.2 testable)
- [x] 4.5 Bypass the report cache lookup in `RunCustomReport` while debug mode is enabled

## 5. Throttling

- [x] 5.1 Add a per-run author-output counter, reset at the start of each report run
- [x] 5.2 Stop author output once the counter reaches `REPORT_DEBUG_MAX_LINES`, printing one
      suppression notice
- [x] 5.3 Treat `0` as unlimited
- [x] 5.4 Exempt breadcrumbs from the counter entirely — the "report function returned" line must
      survive a capped run

## 6. README

- [x] 6.1 Add a "Debugging a report" section: enabling the flag, what the console shows, and the
      output cap
- [x] 6.2 Document `Debug` / `Debugf` in the report authoring reference, including that `%s`
      takes `string_view` directly
- [x] 6.3 Record the caveats: console output is slow, the console cannot be closed for the
      session, `Debug` arguments are still evaluated when disabled, and the console dies with a
      crash so it is not a crash-forensics tool
- [x] 6.4 Note that debug mode bypasses the report cache
- [x] 6.5 Keep the Visual Studio attach procedure, but as the second resort rather than the first

## 7. Verify

- [ ] 7.1 A default build (`REPORT_DEBUG_MODE 0`) shows no console and produces no output
- [ ] 7.2 A debug build opens the console at Notepad++ startup, before any report is run
- [ ] 7.3 The console close box is unavailable; minimising still works; Notepad++ survives an
      attempt to close the console
- [ ] 7.4 `Debugf` renders `string_view`, `std::string`, `const char*`, integers and floats, with
      width and precision honoured
- [ ] 7.5 A deliberate format/argument mismatch prints a marker and does not crash Notepad++
- [ ] 7.6 Output past `REPORT_DEBUG_MAX_LINES` is suppressed with one notice, and the
      "report function returned" breadcrumb still appears afterwards
- [ ] 7.7 Pressing the report shortcut twice in a row produces debug output both times
- [ ] 7.8 Non-ASCII text prints correctly from both a UTF-8 and an ANSI document
- [ ] 7.9 Measure the cost of a per-line print on a mid-size log, to confirm the default cap of
      1000 is in the right region

## 8. Close the two open questions

These are pre-existing uncertainties in the shipped per-buffer cache invalidation, carried over
from `add-custom-report-framework`. Both fail silently when wrong — a stale but plausible result,
with no visible error — which is why they were never settled by inspection. The breadcrumbs from
section 4 make each a keystroke-level test.

- [ ] 8.1 **Does `SCN_MODIFIED` reach `beNotified`?** Open a file, run a report, type one
      character. Expect exactly one `SCN_MODIFIED ... -> stale` line.
- [ ] 8.2 If no line appears, the `stale` mechanism is inert: every command silently serves
      pre-edit results. Record the finding and raise a follow-up change — do not fix it here.
- [ ] 8.3 Confirm the flag actually invalidates: after the edit, run the report again and check
      the breadcrumb reports `stale=true -> caches invalidated`, and that the report content
      reflects the edit
- [ ] 8.4 Confirm the same for Ctrl+Alt+W: edit near a bookmark, then navigate, and check the
      target line reflects the edited positions
- [ ] 8.5 **Is `stale` attributed to the right buffer?** Open two files, note each buffer id from
      its breadcrumbs, then run Replace All in All Opened Documents.
- [ ] 8.6 Compare the buffer ids in the resulting `SCN_MODIFIED` lines against the files that
      were actually modified. If only the active buffer appears, the hole is real: the background
      buffer keeps a stale cache.
- [x] 8.7 Record both outcomes in this file before archiving. If 8.6 confirms the hole, note that
      the candidate fix is flagging every buffer on any modification — over-invalidating but
      never wrong — and raise it as a separate change rather than widening this one.

**Build status:** Release|x64 and Debug|x64 both build clean, with the flag off *and* with
`REPORT_DEBUG_MODE 1` plus a real `Debugf` call in a shipped example — confirming the template
instantiates correctly in the one translation unit where `windows.h` and `CustomReports.h` meet.
No new warnings beyond the two pre-existing `C4312`s in `ProgressDialog.cpp`.

**Formatter verification (task 2.10):** 48 assertions in a scratchpad harness, all passing —
`%s` with `string_view` / `std::string` / `const char*` / string literal / a non-NUL-terminated
slice / an empty view; `%d` with `short`, `size_t`, `long long`, `bool` and `char`; width, zero
pad, left align, precision; `%x %X %o %u %c`; floats via `%g` and `%.2f`; `%%`, a trailing lone
`%`, and an empty format string; every mismatch and arity case producing a marker rather than an
invalid access; and a 5000-character string surviving both the fast path and the width path
without truncation.

**Deviations:** `ReportBuilder::RowCount()` was added (one line) so the "report function
returned, N rows" breadcrumb can report something useful. The `Debugf` call in Example 2 of
`CustomReports.h` ships commented out — it shows an author exactly where such a call goes without
costing anything in a default build.

Sections 7 and 8 are runtime behaviour inside Notepad++ and need a human at the keyboard.

## 9. Findings from verification (2026-08-30)

- [x] 9.1 **`NPPM_GETFULLCURRENTPATH` was wrong a second time — now fixed from the authoritative
      header, not by inference.** It is not in the `NPPMSG` range at all:
      `NPPM_GETFULLCURRENTPATH = RUNCOMMAND_USER + FULL_CURRENT_PATH`, where
      `RUNCOMMAND_USER = WM_USER + 3000` and `FULL_CURRENT_PATH = 1`.
      Both earlier values were structurally wrong, and neither was harmless:
        * `NPPMSG + 17` is `NPPM_GETOPENFILENAMESPRIMARY` — writes through wParam as a pointer
          array. This was the crash.
        * `NPPMSG + 40` is `NPPM_SETMENUITEMCHECK` — silently toggled the check state of
          whatever menu item owns command id 1023, on every report run. No crash, which is
          exactly why it survived a round of "verification".
- [x] 9.2 Root cause of it surviving: task 11.14 of `add-custom-report-framework` was marked
      done on the evidence "the crash stopped". That only disproved the old value; it never
      confirmed the new one. The `(untitled)` fallback then masked the failure completely.
- [x] 9.3 `CurrentFilePath()` now logs a breadcrumb when the message returns FALSE *and* leaves
      the buffer empty, so a wrong id can no longer hide behind the fallback.
- [x] 9.4 Audited every other constant in the trimmed `external/PluginInterface.h` against the
      upstream header. One more was wrong: `NPPM_GETCURRENTVIEW` was `NPPMSG + 98`, correct
      value `NPPMSG + 88`. It was declared but never called, so it had no effect. Fixed.
      All others confirmed correct: `GETCURRENTSCINTILLA +4`, `GETCURRENTLANGTYPE +5`,
      `SETSTATUSBAR +24`, `DMMREGASDCKDLG +33`, `GETPLUGINSCONFIGDIR +46`, `MENUCOMMAND +48`,
      `GETCURRENTBUFFERID +60`, and the three `NPPN_` codes.
- [x] 9.5 Added the file name to the report header and the `SCN_MODIFIED` breadcrumb, after the
      first verification run was invalidated by two presses landing on the same tab without it
      being apparent from opaque hex buffer ids.

### 8.5 / 8.6 status: strong evidence, one data point short

Second run, with both buffers correctly distinguished by size:

```
bufA.log   buf=0x2a070534240   6 lines / 86 bytes
bufB.log   buf=0x2a070533b80   5 lines / 63 bytes

step 3     SCN_MODIFIED  buf=0x2a070533b80  -> stale     (bufB only; bufA never appeared)
step 4     buf=0x2a070534240  stale=false                (bufA)
```

`bufA.log` contains two `IP: 10.0.0.1` lines, so Replace All in All Opened Documents must have
matched it, yet its `stale` flag was never set. Consistent with the hole being real. Not yet
recorded as confirmed: the step-4 report dialog was not read, so it is not strictly established
that bufA was modified. One re-check settles it.

### Third run (2026-08-30): file name fix confirmed, hole still untested

```
==== IP Report  -  bufA.log ====     buf=0x1c39aa9e0f0  stale=false   6 lines / 86 bytes
==== IP Report  -  bufB.log ====     buf=0x1c39aa9e450  stale=true -> caches invalidated
bufA dialog:  10.0.0.1  :  2 hits   first @ L2
```

- [x] 9.6 **`NPPM_GETFULLCURRENTPATH` fix verified by observation.** Report headers now show
      `bufA.log` / `bufB.log` instead of `(untitled)`. This also retroactively settles task
      11.14 of `add-custom-report-framework`, which had been marked done on the far weaker
      evidence that the crash stopped.

**8.5 / 8.6 remain untested.** The bufA dialog still reads `10.0.0.1`, and because debug mode
bypasses the cache that dialog is a fresh read of the document — so bufA was never modified by
the replace. `stale=false` on an unmodified buffer is correct behaviour, not the defect. The
bufA/bufB asymmetry looks like the hole but is fully explained by the replace only reaching the
active document; most likely "Replace All" was used rather than "Replace All in All Opened
Documents".

The procedure was missing a precondition check. Corrected ordering: after the replace, **read
bufA's text in the editor** and confirm it shows `10.9.9.1` *before* invoking the report. Only
once the buffer is known to have been modified does its `stale` value carry any information:

| bufA modified | stale | meaning |
|---|---|---|
| no  | false | correct — nothing to invalidate (all three runs so far) |
| yes | false | **the hole** |
| yes | true  | no hole; Notepad++ activates each buffer in turn |

### Disposition of 8.1 - 8.6: superseded, not completed

Tasks 8.1 through 8.6 were **not** carried out. After four attempts, each invalidated for a
different reason, the project owner chose to apply the defensive fix instead of continuing to
establish whether the defect was reachable.

That fix is a separate change: **`fix-stale-flag-attribution`**, which marks every tracked buffer
stale on any text modification. It removes the risk whether or not Notepad++ can produce the
condition, so the open question is now moot rather than answered.

What this change did deliver against those tasks: the breadcrumbs that make `stale` observable at
all, plus the file-name fix (9.6) without which three of the four attempts could not even be
attributed to the right tab.

Attempts, and why each was invalid:

| # | Outcome |
|---|---|
| 1 | Both measurements landed on the same tab; opaque hex ids hid it |
| 2 | Report dialog not read, so it was unknown whether bufA had been modified |
| 3 | bufA still read `10.0.0.1` — the replace never reached it, so `stale=false` was correct |
| 4 | Buffer ids came from different address ranges; Notepad++ had restarted and the state table was empty |
