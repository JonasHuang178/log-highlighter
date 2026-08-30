## Why

Writing a report parser is currently a blind edit-and-hope loop. An author changes
`config/CustomReports.h`, rebuilds, copies the DLL, restarts Notepad++, opens a file, presses
the shortcut — and sees only the finished report. If an extraction returned empty, there is no
way to see *which* line it failed on or *what* it actually matched. The only tool available is
attaching Visual Studio to `notepad++.exe`, which is far too heavy for "why is this field
blank".

The framework deliberately ships without a crash guard, which makes this worse: when something
goes wrong the editor simply vanishes, taking the evidence with it.

A recent bug made the gap concrete. `NPPM_GETFULLCURRENTPATH` was defined with the wrong message
id and crashed Notepad++ on every invocation. Locating it required reading the engine source,
because nothing in the running plugin said how far it had got.

## What Changes

- **Add** `REPORT_DEBUG_MODE` to `config/CustomReports.h`, default `0` (off)
- **Add** a console window, allocated at Notepad++ startup when the flag is on, that report
  authors print to from inside their report functions
- **Add** `Debug` / `Debugf` to `src/ReportApi.h` — printf syntax implemented as a variadic
  template, so `%s` accepts `std::string_view` directly and a format/argument mismatch cannot
  crash the editor
- **Add** `REPORT_DEBUG_MAX_LINES` throttling so a per-line print on a large log cannot make
  Notepad++ look hung
- **Add** engine breadcrumbs that report the per-buffer `stale` flag, making the cache
  invalidation machinery observable
- **Change** report caching: in debug mode the cache is bypassed, so pressing the shortcut
  repeatedly always re-runs the report
- **Verify** two previously unconfirmed assumptions in the existing per-buffer state machinery
  (see below)

## Capabilities

### New Capabilities

- `report-debug-console`: the debug flag, the console, the `Debug` / `Debugf` API, throttling,
  and engine breadcrumbs

### Modified Capabilities

- `custom-report`: report result caching is bypassed while debug mode is enabled

## Open questions this change closes

Two assumptions in the shipped per-buffer cache invalidation were never confirmed empirically.
Both are silent when wrong — they produce a stale-but-plausible report rather than any visible
error — and the breadcrumbs added here are the cheapest way to observe them.

1. **Does `SCN_MODIFIED` actually reach `beNotified`?** If Notepad++ does not forward it, the
   `stale` flag is never set and every command keeps serving pre-edit results.
2. **Is the `stale` flag attributed to the right buffer?** `SCN_MODIFIED` carries no buffer id,
   so the handler flags `CurrentBuffer()`. An edit to a non-active buffer — "Replace All in All
   Opened Documents" being the realistic case — may flag the wrong `BufferState` and leave the
   edited one holding a stale cache.

## Impact

- `config/CustomReports.h`: two new `#define`s near the top; header comment documents them
- `src/ReportApi.h`: `Debug` / `Debugf`, the enabled flag, the throttle counter
- `src/Report.h` / `src/Report.cpp`: `ReportDebugEnabled()` accessor, console lifecycle,
  breadcrumbs, cache bypass
- `src/Plugin.cpp`: allocate the console on `NPPN_READY`; breadcrumb on the `stale` transition
- `README.md`: a debugging section replacing the current "attach the debugger" advice as the
  first resort

## Non-Goals

- **No crash forensics.** The console belongs to the process and dies with it, so it cannot show
  the last lines before a fault. Mirroring output to a file would fix that and is deliberately
  excluded from this change.
- **No runtime toggle.** Enabling debug mode requires a rebuild, which costs nothing because the
  audience is already rebuilding to change their parser.
- **No "Close Debug Console" menu command.** The console's close box is disabled (closing it
  would terminate Notepad++) and the window then stays for the session; minimising it is the
  intended way to get it out of the way.
- **No SEH.** Unchanged from the framework change.
