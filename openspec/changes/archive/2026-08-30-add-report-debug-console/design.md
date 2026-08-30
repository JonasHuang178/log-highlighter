## Context

`config/CustomReports.h` is the only file a report author edits. It defines report functions and
the `CUSTOM_REPORTS[]` table, and it includes `src/ReportApi.h` — the firewall header that keeps
Win32 and Scintilla types out of author code. `src/Report.cpp` is the single translation unit
that includes `CustomReports.h`; `Plugin.cpp` reaches reports only through accessors in
`Report.h`.

`Report.cpp` already owns a progress dialog, a per-buffer report cache, and code-page conversion.
`Plugin.cpp` owns `BufferState` and the `SCN_MODIFIED` handler that sets the shared `stale` flag.
Both are natural homes for the debug plumbing, and neither requires new architecture.

One structural fact drives the central decision below: `CustomReports.h` includes `ReportApi.h`
at line 87, *after* its header comment. Anything the author defines in `CustomReports.h` is
invisible to `ReportApi.h` at the point where `ReportApi.h` is expanded.

## Goals / Non-Goals

**Goals:**

- A report author can print arbitrary values from inside a report function and watch them appear
  while iterating on the parser
- Debug output is off by default and costs a shipping build nothing observable
- `Debugf` cannot crash Notepad++ regardless of what the author passes it
- `std::string_view` — what every extraction helper returns — prints with a plain `%s`
- Debug output on a large log cannot make Notepad++ appear hung
- The `stale` invalidation machinery becomes observable, closing the two open questions in the
  proposal

**Non-Goals:**

- Surviving a crash (the console dies with the process; no file mirror)
- Toggling debug mode without a rebuild
- A menu command to close the console
- Any change to what a report *produces* — only to what an author can observe

## Decisions

### Decision 1: The flag lives in `CustomReports.h` and is consumed at runtime

**Choice**: `config/CustomReports.h` defines `REPORT_DEBUG_MODE`. `ReportApi.h` declares
`extern bool g_reportDebugEnabled`; `Report.cpp` defines it, initialised from the macro. `Debug`
and `Debugf` early-return on that bool before doing any formatting work.

```
config/CustomReports.h
  #define REPORT_DEBUG_MODE 1          <- author edits here
  #include "../src/ReportApi.h"        <- line 87: too late for #ifdef
        |
        v
src/ReportApi.h
  extern bool g_reportDebugEnabled;
  inline void Debugf(...) { if (!g_reportDebugEnabled) return; ... }
        |
        v
src/Report.cpp   (the only TU that sees both)
  bool g_reportDebugEnabled = REPORT_DEBUG_MODE;
```

**Rationale**: the obvious design — `#ifdef REPORT_DEBUG_MODE` inside `ReportApi.h` — cannot
work. The macro is defined *after* the include, so `ReportApi.h` never sees it and `Debug` would
silently compile to nothing even with the flag on. Requiring the author to place the `#define`
above the include would work but fails silently when forgotten, which is the worst possible
failure mode for a diagnostic feature.

Moving the flag to a separate `config/DebugConfig.h` that `ReportApi.h` includes would restore
true compile-time elimination, at the cost of splitting the author's surface across two files.
The runtime bool costs one predictable branch per `Debug` call — unmeasurable next to the work a
report already does per line — and keeps everything the author touches in one place.

The one thing given up: with the flag off, the arguments to `Debug` are still evaluated. An
author who writes `Debugf("%s", ExpensiveDump())` pays for `ExpensiveDump()` in a shipping build.
Documented rather than designed around.

### Decision 2: `Debugf` is a variadic template, not varargs

**Choice**: keep printf syntax; implement as `template <class... Args> void Debugf(const char*
fmt, Args&&... args)`. The format string is walked at runtime and each conversion formats exactly
one argument whose type the template knows. Numeric conversions delegate to `snprintf` with that
single specifier, so width, precision and flags keep working. String conversions accept
`const char*`, `std::string` and `std::string_view`.

**Rationale**: real varargs fails this API twice over.

`std::string_view` cannot travel through `...` usefully — `va_arg` cannot know what it is, and
retrieving it as `const char*` reads a length field as a pointer. Since every extraction helper
returns `string_view`, the single most common thing an author would print is the one thing
varargs cannot carry. The alternative is making authors write `(int)sv.size(), sv.data()` for
`%.*s`, which is precisely the papercut the rest of `ReportApi.h` was designed to eliminate.

More seriously, varargs printf is a crash vector. `Debugf("%s", 42)` dereferences `42`;
`Debugf("%d %d", 1)` reads a non-existent argument. This framework deliberately ships without a
crash guard, and its stated safety property is that an author using the provided APIs *cannot*
construct an invalid access. Handing them raw varargs would put the largest hole in that property
directly into the debugging tool — the thing reached for when something is already wrong.

**Trade-off**: the compiler's printf format checking is lost, and a format/argument mismatch is
detected at runtime rather than compile time (C++17 has no `consteval`, so the literal cannot be
checked against argument types). On mismatch the implementation emits a visible marker in place
of the value instead of misbehaving. Losing a compiler warning to gain "cannot crash the editor"
is the right side of that trade.

### Decision 3: The console is allocated on `NPPN_READY`, never in `DllMain`

**Choice**: `Plugin.cpp` handles `NPPN_READY` and asks `Report.h` whether debug mode is on. If it
is, the console is allocated there.

**Rationale**: `AllocConsole` from `DllMain` runs under the loader lock and is not safe. The
author asked for the console to appear when Notepad++ starts, and `NPPN_READY` is the earliest
point at which the editor is fully initialised. `NPPN_READY` is already declared in the trimmed
`external/PluginInterface.h` and currently unused, so no new message id has to be introduced —
which matters, because guessing a Notepad++ message id is exactly what caused the crash that
motivated this change.

### Decision 4: The console close box is disabled

**Choice**: after `AllocConsole`, remove `SC_CLOSE` from the console window's system menu, and
install a `SetConsoleCtrlHandler` that swallows `CTRL_CLOSE_EVENT`.

**Rationale**: a console allocated by a process belongs to that process. Closing its window sends
`CTRL_CLOSE_EVENT`, whose default handling terminates the owner — Notepad++, with every unsaved
tab. A user closing what looks like a stray console window would lose their work. Removing the
close box is the primary defence; the control handler is a second line, and only that, because
Windows allows a bounded time to respond and terminates regardless afterwards.

The console then cannot be closed for the session. That is accepted: debug mode is a deliberate
compile-time opt-in, minimising still works, and turning it off is a rebuild the author is
already performing.

### Decision 5: Debug mode bypasses the report cache

**Choice**: while `REPORT_DEBUG_MODE` is on, `RunCustomReport` skips the cache lookup and always
re-runs the report function.

**Rationale**: an author iterating on a parser presses the shortcut repeatedly. With the cache
active, the second press returns the stored text without executing the report function, so
nothing is printed. The console falls silent and the natural conclusion is that debug output is
broken. Caching exists to make repeated viewing cheap; while debugging, re-running *is* the point.

### Decision 6: Breadcrumbs report the `stale` flag, not cache hits

**Choice**: engine breadcrumbs print the per-buffer `stale` state and the buffer id. The
`SCN_MODIFIED` handler prints one line when the flag transitions from `false` to `true`.

```
[engine] SCN_MODIFIED  buf=0x1a2f  -> stale
[engine] IP Report  buf=0x1a2f  stale=true -> caches invalidated
[engine] report cache bypassed (debug mode)
[engine] snapshot 45210 lines / 3.2 MB
[engine] entering report function
...
[engine] report function returned, 7 rows
```

**Rationale**: the obvious instrumentation — logging cache hit versus miss — is unobservable
under Decision 5, because debug mode never consults the cache. The `stale` flag is the state that
actually matters, and printing it directly answers both open questions in the proposal: whether
the `SCN_MODIFIED` line appears at all settles question 1, and whether the buffer id matches the
file that was really edited settles question 2.

Printing on every `SCN_MODIFIED` would flood the console, since it fires per keystroke. Printing
only on the `false -> true` transition yields one line per "first edit since the last scan",
which is exactly the event with information in it.

### Decision 7: Throttling counts author output; engine breadcrumbs are exempt

**Choice**: `REPORT_DEBUG_MAX_LINES` (default 1000, `0` = unlimited) caps author output per
report run. On reaching the cap, one suppression notice is printed and further author output is
dropped. Engine breadcrumbs never count toward the cap and are never suppressed.

**Rationale**: console I/O is slow, and a report runs on the UI thread. An author printing one
line per document line on a 500,000-line log would make Notepad++ appear to hang — from the
debugging tool, which is the worst place to produce that impression. A hard stop is predictable;
sampling every Nth line would be cleverer but would silently change what the author sees.

Exempting breadcrumbs matters because the most valuable line — `report function returned` — comes
*after* the author's output. Counting it toward the cap would suppress exactly the line that says
whether the run finished.

### Decision 8: Console output is written with `WriteConsoleW`

**Choice**: convert author output from the document's code page to UTF-16 using the same
`SCI_GETCODEPAGE` logic the report renderer already uses, then write with `WriteConsoleW`.

**Rationale**: what an author prints is usually a `string_view` into the document snapshot, so it
carries the document's encoding. Writing those bytes to a console with `printf` renders them
through whatever the console code page happens to be, which mangles both UTF-8 and Big5 logs in
different ways. Converting once and writing UTF-16 sidesteps console code pages entirely and
reuses a conversion path that already exists.

## Risks / Trade-offs

- **[Author output is evaluated even when disabled]** → Decision 1's cost. Arguments to `Debug`
  are computed before the early return. Documented in the header comment.
- **[Console output makes reports much slower]** → Mitigated by throttling, but even 1000 console
  writes are noticeable. Documented: debug against a small sample file, not a production log.
- **[Format mismatch produces a marker instead of a compile error]** → Accepted; C++17 cannot
  check a runtime-parsed format string against argument types. The marker is visible in the
  output, and the failure is inert.
- **[`SCN_MODIFIED` may not reach the plugin at all]** → Open question 1. If confirmed broken,
  every command silently serves pre-edit results and a follow-up change is needed; the breadcrumb
  makes this a one-keystroke test rather than a code audit.
- **[The `stale` flag may be attributed to the wrong buffer]** → Open question 2. If confirmed,
  the likely fix is flagging every buffer on any modification — over-invalidating but never
  wrong — at the cost of losing per-buffer cache benefit whenever any file is edited. Out of
  scope here; this change only establishes whether the problem is real.
- **[Only one console per process]** → Not a limitation in practice; a second Notepad++ instance
  is a separate process with its own console.
