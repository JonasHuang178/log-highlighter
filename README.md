# log-highlighter

A Notepad++ 64-bit plugin that colorizes log keywords and step markers on demand
(**Ctrl+Alt+Q**), with a clickable overview minimap of every match in the file,
keyboard navigation between bookmark keywords (**Ctrl+Alt+W**), and custom
reports you write yourself in C++ (**Ctrl+Alt+E**).

---

## Features

### Log Type — foreground color

Exact keyword match (case-sensitive). Applies a foreground color to the matched keyword.

| Keyword | Color | Shown in panel |
|---|---|---|
| `[ ERROR ]` | Red | yes |
| `[ WARN ]` | Golden yellow | no |
| `[ DEBUG ]` | Cornflower blue | no |

### Step Type — background color

Matches `<prefix><digits>` followed by a space or end-of-line.
Applies a background color from the prefix to the end of the line.

| Prefix | Valid examples | Invalid examples | Color | Shown in panel |
|---|---|---|---|---|
| `Step` | `Step1 `, `Step12 `, `Step123` | `Step `, `Stepname`, `Step1init` | Light green | no |
| `Step123` | `Step1234 ` | `Step123 ` (no digit after the prefix) | Light green | no |

> The shipped `Step123` rule is redundant: the prefix still requires at least one
> digit after it, so everything it can match (`Step1234 `) is already matched by
> `Step`. It is safe to delete from `LogPatterns.h`.

### Bookmark Type — foreground color + Ctrl+Alt+W navigation

Exact keyword match (case-sensitive), colored the same way as a Log Type keyword,
but also usable as a jump target: **Ctrl+Alt+W** moves the caret to the next
bookmark line, wrapping back to the first one at the end of the file.

| Keyword | Color | Shown in panel |
|---|---|---|
| `Start test` | Magenta | yes |

Ctrl+Alt+W is independent of Ctrl+Alt+Q: it scans for bookmark keywords itself
the first time you press it, so it works on a freshly opened file. Repeated
presses reuse that scan and cycle instantly; editing the document makes the next
press rescan. If there is nothing to jump to, the status bar says so and the
caret stays put.

### Overview Panel

A fixed-width strip rendered inside the Scintilla editor's non-client area (right edge).
It shows a proportional minimap of the entire document:

- Colored tick marks for every match where `showInPanel = true` in `LogPatterns.h`
- A viewport indicator box tracking the current viewport position (configurable color, border width, and visibility via `OverviewConfig.h`)
- **Click** anywhere on the panel to jump to that position in the document

Clicks snap to the nearest mark within `OVERVIEW_SNAP_RADIUS` lines (default 50,
in `config/OverviewConfig.h`), so a 1px mark in a 100 000-line file is still
easy to hit. Outside that radius the click falls back to a plain proportional
jump. The target line is centered in the viewport with the caret at line start.

Marks are built from the in-memory match list, not from Scintilla indicators,
so the panel always covers the whole document.

### Custom Report — write your own parser

**Ctrl+Alt+E** runs a report function you write in `config/CustomReports.h` against
the active document and shows its output in a read-only window. What a report
extracts is entirely up to you — the plugin makes no assumptions about your log
format.

```
IP Report - device_20260830.log  (45,210 lines)
===============================================

---- IP addresses ----------------------------
10.0.0.3         :    27 hits   first @ L887
172.16.0.5       :     3 hits   first @ L1203
192.168.1.10     :  1823 hits   first @ L142

Unique addresses : 3
```

The window is monospaced so aligned columns line up, resizable, and scrollable.
Select any part of it and copy with **Ctrl+C** or the right-click menu; right-click
also offers **Select All**. **ESC** closes it.

Each report registered in `CUSTOM_REPORTS[]` gets its own menu item. Results are
cached per tab, so pressing the shortcut again is instant until you edit the file.

While writing a parser, `REPORT_DEBUG_MODE` opens a console you can print to from
inside your report function — see [Debugging a report](#debugging-a-report).

Like Ctrl+Alt+W, Ctrl+Alt+E is independent of Ctrl+Alt+Q — you never have to parse
first. A progress dialog with a working Cancel button appears while the report runs.

See [Write your own report](#write-your-own-report) for the authoring guide.

### Parse time display

After each **Ctrl+Alt+Q**, the Notepad++ status bar (bottom-left, where "Normal text file" appears) shows the total parse + render time:

| Duration | Format example |
|---|---|
| Under 60 seconds | `log-highlighter: parsed in 0.042 s` |
| 1 minute or more | `log-highlighter: parsed in 01:05.234` |
| 1 hour or more | `log-highlighter: parsed in 01:02:03.456` |

### Per-tab highlight state

Parse results are cached per open tab. Switching to another tab and back restores
that tab's Overview Panel marks instantly with no re-parse — Scintilla keeps the
in-editor highlights per buffer on its own. Closing a tab frees its match list.

Highlights are **not** refreshed automatically after you edit the document, and
opening a file never triggers a parse. **Ctrl+Alt+Q** is the only trigger.

---

## Usage

| Action | Result |
|---|---|
| **Ctrl+Alt+Q** | Scan the active document and apply all highlights |
| **Ctrl+Alt+W** | Jump to the next Bookmark keyword, centered in the viewport |
| **Ctrl+Alt+E** | Run the first registered custom report and show the result |
| **Plugins > log-highlighter > Parse Log** | Same as Ctrl+Alt+Q |
| **Plugins > log-highlighter > Next Bookmark** | Same as Ctrl+Alt+W |
| **Plugins > log-highlighter > _\<report name\>_** | Run that report |
| **Plugins > log-highlighter > About** | Show plugin version |

The three commands are independent — none of them requires another to have run
first. Each caches its own result per tab and rescans after you edit the document.

### Progress dialog

A modeless progress dialog appears on **every** parse and covers two phases:

| Phase | Label | Cancel |
|---|---|---|
| 1 — parsing | `Processing: N / M lines`, updated every 500 lines | available |
| 2 — applying highlights | `Applying highlights...` | hidden |

Parsing runs on the UI thread; the progress callback pumps messages with
`PeekMessage` every 500 lines, so the dialog keeps repainting and Cancel stays
clickable. The Notepad++ window is disabled for the duration of both phases to
block re-entrant Ctrl+Alt+Q presses.

Cancelling during phase 1 clears all highlights and the Overview Panel, leaving
the document in its original un-highlighted state and writing no timing to the
status bar. Phase 2 cannot be cancelled — it blocks until every match is filled.

---

## Installation

1. Build the project in **Release x64** (see [Build](#build))
2. Copy `log-highlighter.dll` to:
   ```
   %APPDATA%\Notepad++\plugins\log-highlighter\log-highlighter.dll
   ```
3. Restart Notepad++

---

## Build

**Requirements**

- Visual Studio 2022 (MSVC v143)
- Windows SDK 10.0
- Target: x64 only (Notepad++ 64-bit)

**Steps**

1. Open `log-highlighter\log-highlighter.sln`
2. Select configuration **Release | x64**
3. Build → output is `x64\Release\log-highlighter.dll`

---

## Customization

Edit **`config/LogPatterns.h`** (keywords, colors), **`config/OverviewConfig.h`** (panel
appearance) or **`config/CustomReports.h`** (your own reports, plus the debug-mode
flags) and rebuild. No other files need to change.

### Add a Log Type keyword

```cpp
static const LogTypeRule LOG_TYPE_RULES[] = {
    { "[ ERROR ]",  MAKE_BGR(220,   0,   0), true  },  // red          — shown in panel
    { "[ WARN ]",   MAKE_BGR(200, 160,   0), false },  // golden yellow
    { "[ DEBUG ]",  MAKE_BGR( 70, 150, 255), false },  // cornflower blue
    { "[ INFO ]",   MAKE_BGR(  0, 180,   0), false },  // green  ← add here
};
```

Fields:

| Field | Type | Description |
|---|---|---|
| `keyword` | `const char*` | Exact UTF-8 string to match (case-sensitive) |
| `textColor` | `COLORREF` | Foreground color — use `MAKE_BGR(r, g, b)` |
| `showInPanel` | `bool` | `true` = show tick mark in Overview Panel |

### Add a Step Type prefix

```cpp
static const StepTypeRule STEP_TYPE_RULES[] = {
    { "Step",   MAKE_BGR(180, 230, 180), false },  // light green
    { "Phase",  MAKE_BGR(180, 210, 255), false },  // light blue  ← add here
};
```

Fields:

| Field | Type | Description |
|---|---|---|
| `prefix` | `const char*` | Literal text before the digit sequence, e.g. `"Step"` |
| `bgColor` | `COLORREF` | Background color — use `MAKE_BGR(r, g, b)` |
| `showInPanel` | `bool` | `true` = show tick mark in Overview Panel |

Match rule: `<prefix>` + one or more digits + (space or end-of-line).
The highlighted range runs from the start of the prefix to the end of the line.

### Add a Bookmark keyword

```cpp
static const BookmarkRule BOOKMARK_RULES[] = {
    { "Start test", MAKE_BGR(200,   0, 180), true },  // magenta — shown in panel
    { "End test",   MAKE_BGR(  0, 180, 180), true },  // teal    ← add here
};
```

Fields are identical to `LogTypeRule` (`keyword`, `textColor`, `showInPanel`).
Every entry here is also a **Ctrl+Alt+W** jump target — all bookmark rules share
one navigation cycle, ordered by position in the document.

### Write your own report

Edit **`config/CustomReports.h`** and rebuild. Write a function, add a row to
`CUSTOM_REPORTS[]`, done.

```cpp
static void IpReport(const ReportContext& ctx, ReportBuilder& out)
{
    out.Section("IP addresses");

    for (auto [lineNo, line] : ctx.Lines())
    {
        auto ip = Field(After(line, "IP: "), ' ', 0);
        if (ip.empty()) continue;

        out.AtLine(lineNo, ip);
    }
}

static const CustomReport CUSTOM_REPORTS[] = {
//   Menu title       Function    Shortcut (0 = none)
    { L"IP Report",  IpReport,   'R' },
};
```

`CUSTOM_REPORTS[]` fields:

| Field | Type | Description |
|---|---|---|
| `title` | `const wchar_t*` | Menu item text under Plugins > log-highlighter |
| `fn` | `ReportFn` | `void (const ReportContext&, ReportBuilder&)` |
| `shortcut` | `char` | Letter for Ctrl+Alt+`<letter>`, or `0` for none |

The file ships with three worked examples — a two-line one, one that aggregates
with `std::map`, and one that scans several keywords in a single pass.

#### Reading the document — `ReportContext`

| Member | Description |
|---|---|
| `ctx.Lines()` | `for (auto [lineNo, line] : ctx.Lines())` — 1-based, no line ending, no trailing `\r`, empty lines included |
| `ctx.FindAll("kw")` | `for (auto hit : ctx.FindAll("kw"))` |
| `ctx.FindAll({"a","b"})` | One Aho-Corasick pass; ten keywords cost the same as one |
| `ctx.text` / `ctx.length` | Raw snapshot bytes — no progress, no cancel |
| `ctx.lineCount` | Total number of lines |
| `ctx.fileName` / `ctx.filePath` | Active document, as `const wchar_t*` |

A `hit` carries `keyword`, `lineNo`, `line` and `after` (the rest of the line
starting past the keyword).

#### Writing output — `ReportBuilder`

| Call | Output |
|---|---|
| `out.Section("Title")` | `---- Title -----------------`, and resets alignment |
| `out.Line(text)` | Free text, not aligned |
| `out.KV(key, value)` | `key : value` — value may be text or any number |
| `out.KVf(key, fmt, ...)` | Value rendered with a printf format |
| `out.AtLine(n, text)` | `L  142 : text` |
| `out.Blank()` | Empty line |

Key columns are aligned automatically, independently per `Section`. The report
title, file name and line count are added for you.

#### String helpers

| Helper | Result |
|---|---|
| `After(s, kw)` | Text after the first `kw` |
| `Before(s, kw)` | Text before the first `kw` |
| `Between(s, a, b)` | Text between `a` and the first `b` after it |
| `Field(s, delim, n)` | The n-th field, 0-based; runs of delimiters give empty fields |
| `Trim(s)` | Leading and trailing whitespace removed |
| `Contains` / `StartsWith` / `EndsWith` | `bool` |
| `ToInt(s, out)` / `ToDouble(s, out)` | `bool`; `out` untouched on failure |

**Every extraction helper returns empty rather than failing**, and empty input
gives empty output. That is what lets them nest without a check at each level:

```cpp
auto ip = Field(After(line, "IP: "), ' ', 0);
if (ip.empty()) continue;          // one check covers both steps
```

#### Printing while you work

`Debug(text)` and `Debugf(fmt, ...)` print from inside a report function to a
console window, which is the quickest way to see what your parser is actually
matching. Both are inert until you set `REPORT_DEBUG_MODE` — see
[Debugging a report](#debugging-a-report).

#### Guarantees

- Every `string_view` from `ctx` or a helper points into a private snapshot that
  lives until your function returns. Collecting them into a `std::map` or
  `std::vector` is safe and copies no characters — don't store them outside the
  function.
- Line numbers are 1-based, matching the Notepad++ margin.
- Progress and Cancel work by themselves as long as you iterate with
  `ctx.Lines()` or `ctx.FindAll()`. Reading `ctx.text` directly opts out of both.

#### Hazards

- **There is no crash guard.** A stray pointer in a report function takes down
  Notepad++ and every unsaved tab with it. Stay on `ctx.Lines()`, `ctx.FindAll()`
  and the helpers and you cannot go out of range. To see what your parser is
  actually doing, turn on [debug mode](#debugging-a-report).
- **Save `CustomReports.h` as UTF-8 with BOM** if you use non-ASCII keywords.
  Without the BOM, MSVC reads the source in the system ANSI code page and your
  literal silently never matches the UTF-8 document.
- Keyword characters passed to `FindAll` must outlive the loop. String literals
  always do; a temporary `std::string` does not.
- `CustomReports.h` is included by `src/Report.cpp` only.

#### Debugging a report

Set `REPORT_DEBUG_MODE` to `1` in `config/CustomReports.h` and rebuild. Notepad++
then opens a console window at startup, and `Debug` / `Debugf` calls inside your
report function print to it.

```cpp
// config/CustomReports.h
#define REPORT_DEBUG_MODE       1      // 0 = off (default)
#define REPORT_DEBUG_MAX_LINES  1000   // 0 = unlimited

static void IpReport(const ReportContext& ctx, ReportBuilder& out)
{
    for (auto [lineNo, line] : ctx.Lines())
    {
        auto ip = Field(After(line, "IP: "), ' ', 0);

        Debugf("L%-5d raw=[%s] ip=[%s]", lineNo, line, ip);

        if (ip.empty()) continue;
        out.AtLine(lineNo, ip);
    }
}
```

```
==== IP Report ====
[engine] buf=0x1a2f  stale=true -> caches invalidated
[engine] report cache bypassed (debug mode)
[engine] snapshot 45210 lines / 3355443 bytes
[engine] entering report function
L1     raw=[10:23:40 [ DEBUG ] boot] ip=[]
L3     raw=[10:23:42 IP: 192.168.1.10 up] ip=[192.168.1.10]
... debug output suppressed after 1000 lines
[engine] report function returned, 7 rows
[engine] dialog shown
```

| Constant | Default | Description |
|---|---|---|
| `REPORT_DEBUG_MODE` | `0` | `1` = open the console at startup and enable `Debug` / `Debugf` |
| `REPORT_DEBUG_MAX_LINES` | `1000` | Cap on your own output per report run (`0` = unlimited) |

| Call | Output |
|---|---|
| `Debug(text)` | One line of plain text |
| `Debugf(fmt, ...)` | One line, printf syntax |

`%s` takes `std::string_view`, `std::string` and `const char*` directly — there is
no size-and-pointer pair to write. Supported conversions are `d i u o x X c`,
`e E f F g G a A`, `s` and `%%`, with the usual width and precision. `Debugf` is a
variadic template rather than C varargs, so a conversion that does not match its
argument prints `<!bad-arg>` and a missing argument prints `<!no-arg>` — it cannot
crash the editor the way a real `printf("%s", 42)` would.

Lines prefixed `[engine]` come from the plugin itself. They are never suppressed
by the output cap, so the line telling you the report function returned survives
even when your own output was capped.

**Worth knowing:**

- Console output is slow, and a report runs on the UI thread. Debug against a
  small sample file; `REPORT_DEBUG_MAX_LINES` exists so a per-line print on a
  large log cannot make Notepad++ look hung.
- **Debug mode bypasses the report cache**, so pressing the shortcut twice really
  runs your report twice. Without that, the second press would print nothing.
- Arguments are still evaluated when debug mode is off, so avoid
  `Debugf("%s", SomethingExpensive())` in a shipping build.
- **The console cannot be closed while Notepad++ runs.** Closing a console
  terminates the process that owns it, which would take the editor and every
  unsaved tab with it, so the close box is disabled. Minimise it, or rebuild with
  `REPORT_DEBUG_MODE 0`.
- The console dies with the process, so it cannot show you the last lines before
  a crash.

If you need a breakpoint rather than a print:

1. Build **Debug | x64**
2. Copy `log-highlighter.dll` into the plugin folder and start Notepad++
3. In Visual Studio: **Debug > Attach to Process** → `notepad++.exe`
4. Set a breakpoint in your report function and press its shortcut

### Overview Panel appearance

Edit **`config/OverviewConfig.h`** and rebuild.

| Constant | Default | Description |
|---|---|---|
| `OVERVIEW_PANEL_WIDTH` | `14` | Panel strip width in pixels |
| `OVERVIEW_MARK_MIN_H` | `1` | Minimum mark height in pixels |
| `OVERVIEW_SNAP_RADIUS` | `50` | Click snap radius in document lines (0 = disable) |
| `OVERVIEW_VIEWPORT_BORDER_VISIBLE` | `true` | Show the viewport indicator box (`false` = hidden entirely) |
| `OVERVIEW_VIEWPORT_COLOR` | `RGB(130, 130, 130)` | Viewport box border color |
| `OVERVIEW_VIEWPORT_BORDER_WIDTH` | `1` | Viewport box border pen width in pixels |
| `OVERVIEW_VIEWPORT_BG_COLOR` | `CLR_NONE` | Viewport box fill color (`CLR_NONE` = system scrollbar color) |

`OverviewConfig.h` also defines `OVERVIEW_BG_COLOR`, but nothing reads it — the
panel background is painted with the system color `COLOR_BTNFACE` so the strip
matches the scrollbar next to it. Changing that constant has no effect.

### Color macro

```cpp
MAKE_BGR(red, green, blue)   // each channel 0–255
```

### Update version / About text

Edit **`config/AboutInfo.h`** and rebuild.

---

## Project Structure

```
log-highlighter/
├── log-highlighter.sln
├── log-highlighter.vcxproj
├── log-highlighter.vcxproj.filters
├── log-highlighter.def
├── external/                     ← Notepad++ & Scintilla API headers (do not modify)
│   ├── PluginInterface.h
│   ├── Scintilla.h
│   └── SciLexer.h
├── config/                       ← user-editable settings
│   ├── LogPatterns.h             ← keywords, colors, panel visibility
│   ├── OverviewConfig.h          ← panel width, mark height, snap radius, colors
│   ├── CustomReports.h           ← your own report functions (UTF-8 with BOM)
│   └── AboutInfo.h               ← plugin name, version, about text
└── src/                          ← implementation
    ├── dllmain.cpp
    ├── Plugin.h / Plugin.cpp     ← Notepad++ API exports, per-buffer caches
    ├── Parser.h / Parser.cpp     ← document snapshot + rule-table scan
    ├── AhoCorasick.h             ← multi-pattern automaton (no platform types)
    ├── ReportApi.h               ← report authoring surface (the firewall)
    ├── Report.h / Report.cpp     ← report engine: snapshot, progress, encoding
    ├── ReportDialog.h / .cpp     ← modal read-only report window
    ├── DebugConsole.h / .cpp     ← debug console (REPORT_DEBUG_MODE)
    ├── log-highlighter.h / .cpp  ← Scintilla indicator styles & bulk fill
    ├── OverviewPanel.h / .cpp    ← non-client-area minimap panel
    └── ProgressDialog.h / .cpp   ← modeless two-phase progress window
```

### Key implementation notes

- **Scanner** (`Parser.cpp`, `AhoCorasick.h`): the automaton lives in
  `AhoCorasick.h`, identifies patterns by a caller-chosen integer, and knows
  nothing about rule tables — which is what lets `ReportApi.h` reuse it for
  `ctx.FindAll()`. `Parser.cpp` builds one once per process from `LogPatterns.h`
  and maps indices back to rule tables through contiguous per-table bases, the
  same scheme used for indicator bases. Scanning is a single O(N) pass regardless
  of pattern count. `SnapshotDocument` copies the document into a local buffer
  before any scan, so a scan survives edits made while a progress callback is
  pumping messages.

- **Report firewall** (`ReportApi.h`): the report authoring surface contains no
  `windows.h`, no Scintilla and no `Parser.h`, so a report function never sees
  `HWND`, `SendMessage`, `SCI_*` or `Match`. Everything crosses the boundary as
  `std::string_view` and `int`. `Report.cpp` handles the platform side —
  snapshot, progress, `SCI_GETCODEPAGE` conversion to UTF-16, caching.

- **Report progress** (`ReportApi.h`): the line and `FindAll` iterators tick a
  `ProgressSink` every 500 lines and pump messages, so an author writing an
  ordinary range-for gets a working progress bar and Cancel button without
  knowing either exists. On cancel the iterator compares equal to `end()`, the
  author's loop ends normally, and the engine discards the output.

- **No crash guard** (`Report.cpp`): report functions are invoked directly, with
  no SEH wrapper — a fault takes down Notepad++. The mitigation is that every
  helper in `ReportApi.h` returns empty instead of reading out of range, so an
  author who stays on the provided APIs cannot construct an invalid access.

- **Independent commands** (`Plugin.cpp`): Parse Log, Next Bookmark and Custom
  Report each fill their own per-buffer cache on first use; none is a
  precondition for another. `SCN_MODIFIED` sets a single `stale` flag that
  invalidates the offset-dependent caches on next use — it never starts a scan
  itself. Parse Log fills the bookmark cache as a free side effect of its own
  scan, which creates no dependency in either direction.

- **Debug console** (`DebugConsole.cpp`): allocated from the `NPPN_READY` handler,
  never from `DllMain` — `AllocConsole` under the loader lock is not safe. The
  close box is removed with `DeleteMenu(SC_CLOSE)` because closing a console
  sends `CTRL_CLOSE_EVENT` to its owner, whose default handling would terminate
  Notepad++. Output goes through `WriteConsoleW` after conversion from the
  document's code page, so console code pages never enter into it.

- **`Debugf` is a variadic template** (`ReportApi.h`): C varargs cannot carry a
  `std::string_view` — which is what every extraction helper returns — and
  `printf("%s", 42)` would dereference an integer. Since the framework ships no
  crash guard, the debugging tool must not be the easiest way to crash the
  editor. The format string is walked at runtime and each conversion formats one
  argument of a type the template knows, delegating to `snprintf` for numerics so
  width and precision still work.

- **Debug flag is read at runtime** (`ReportApi.h` / `Report.cpp`):
  `CustomReports.h` includes `ReportApi.h` *before* defining
  `REPORT_DEBUG_MODE`, so an `#ifdef` in the header would never see the macro and
  would silently compile `Debug` away even with debug mode on. An `extern bool`
  set in `Report.cpp` avoids the ordering trap and keeps the flag in the one file
  authors already edit.

- **Report menu wiring** (`Plugin.cpp`): a Notepad++ command callback takes no
  arguments, so each report needs a distinct function pointer. Compile-time
  generated thunks (`ReportThunk<N>`) supply them, which is why `Plugin.cpp` can
  drive the menu through `Report.h` accessors and leave `CustomReports.h`
  included by exactly one translation unit.

- **Bulk indicator fill** (`log-highlighter.cpp`): each `SCI_INDICATORFILLRANGE`
  normally fires an `SCN_MODIFIED(CHANGEINDICATOR)` notification back to
  Notepad++ over a synchronous `SendMessage` round-trip (~120 µs). At 64 k
  matches that alone costs 7+ seconds, so `ApplyHighlights` masks
  `SC_MOD_CHANGEINDICATOR` out of the mod-event mask for the duration of the
  fill and restores it afterwards.

- **Indicator layout** (`log-highlighter.cpp`): indicator indices start at 11 to
  clear Notepad++'s built-ins (0–10, including Smart Highlight at 8). The three
  rule tables are laid out back to back — Log Type, then Step Type, then
  Bookmark — with each base derived at compile time from the previous table's
  `sizeof`, so adding a rule shifts the later ranges automatically.

- **Bookmark navigation** (`Plugin.cpp`): `NextBookmark` scans for itself when
  its cache is empty, picks the first bookmark line past the caret (wrapping to
  the first match otherwise), and centers it via the same deferred
  `SetTimer(10ms)` trick the Overview Panel uses — a direct scroll from the
  command handler gets overridden by Notepad++ afterwards. The scan goes through
  `ParseDocument` rather than a bookmark-only pass because Aho-Corasick costs the
  same either way, so filtering afterwards is free.

- **Per-buffer state** (`Plugin.cpp`): caches are keyed by NPP buffer ID.
  `NPPN_BUFFERACTIVATED` rebuilds the Overview Panel from the cached match list;
  `NPPN_FILEBEFORECLOSE` drops the whole entry. No parse, scan or report is ever
  triggered by a notification. Editing leaves `matches` alone so the panel still
  restores something on tab switch — the same trade the editor's own indicators
  make — while the bookmark and report caches are dropped.

- **Overview Panel** (`OverviewPanel.cpp`): Implemented via Win32 window
  subclassing of the Scintilla HWND (`WM_NCCALCSIZE` / `WM_NCPAINT`).
  No separate child window is created; the panel strip is carved out of
  Scintilla's non-client area.

- **Progress dialog** (`ProgressDialog.cpp`): Modeless `WS_POPUP` window with a
  fixed size (`WM_GETMINMAXINFO`). Everything runs on the UI thread — no worker
  thread. The parse progress callback drains the message queue with
  `PeekMessage` every 500 lines, which is what keeps the window painted and
  Cancel clickable. `SetProgressApplying` switches the label for phase 2 and
  hides Cancel, then forces a repaint via `UpdateWindow` before the apply loop
  blocks the thread. `EnableWindow(hNpp, FALSE)` prevents re-entrant Ctrl+Alt+Q.
