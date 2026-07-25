# log-highlighter

A Notepad++ 64-bit plugin that colorizes log keywords and step markers on demand
(**Ctrl+Alt+Q**), with a clickable overview minimap of every match in the file.

---

## Features

### Log Type — foreground color

Exact keyword match (case-sensitive). Applies a foreground color to the matched keyword.

| Keyword | Color |
|---|---|
| `[ ERROR ]` | Red |
| `[ WARN ]` | Golden yellow |
| `[ DEBUG ]` | Cornflower blue |
| `[  ERROR  ]` | Red |
| `[ WARNING ]` | Golden yellow |
| `[  MSG    ]` | Cornflower blue |

### Step Type — background color

Matches `<prefix><digits>` followed by a space or end-of-line.
Applies a background color from the prefix to the end of the line.

| Pattern | Valid examples | Invalid examples | Color |
|---|---|---|---|
| `StepN` | `Step1 `, `Step12 `, `Step123` | `Step `, `Stepname`, `Step1init` | Light green |

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
| **Plugins > log-highlighter > Parse Log** | Same as Ctrl+Alt+Q |
| **Plugins > log-highlighter > About** | Show plugin version |

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

Edit **`config/LogPatterns.h`** (keywords, colors) or **`config/OverviewConfig.h`** (panel
appearance) and rebuild. No other files need to change.

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

### Overview Panel appearance

Edit **`config/OverviewConfig.h`** and rebuild.

| Constant | Default | Description |
|---|---|---|
| `OVERVIEW_PANEL_WIDTH` | `14` | Panel strip width in pixels |
| `OVERVIEW_MARK_MIN_H` | `1` | Minimum mark height in pixels |
| `OVERVIEW_BG_COLOR` | `RGB(60, 60, 60)` | Panel background color |
| `OVERVIEW_SNAP_RADIUS` | `50` | Click snap radius in document lines (0 = disable) |
| `OVERVIEW_VIEWPORT_BORDER_VISIBLE` | `true` | Show the viewport indicator box (`false` = hidden entirely) |
| `OVERVIEW_VIEWPORT_COLOR` | `RGB(130, 130, 130)` | Viewport box border color |
| `OVERVIEW_VIEWPORT_BORDER_WIDTH` | `1` | Viewport box border pen width in pixels |
| `OVERVIEW_VIEWPORT_BG_COLOR` | `CLR_NONE` | Viewport box fill color (`CLR_NONE` = system scrollbar color) |

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
│   └── AboutInfo.h               ← plugin name, version, about text
└── src/                          ← implementation
    ├── dllmain.cpp
    ├── Plugin.h / Plugin.cpp     ← Notepad++ API exports, parse orchestration
    ├── Parser.h / Parser.cpp     ← Aho-Corasick single-pass scanner
    ├── log-highlighter.h / .cpp  ← Scintilla indicator styles & bulk fill
    ├── OverviewPanel.h / .cpp    ← non-client-area minimap panel
    └── ProgressDialog.h / .cpp   ← modeless two-phase progress window
```

### Key implementation notes

- **Scanner** (`Parser.cpp`): Aho-Corasick automaton built once per process on
  first use from `LogPatterns.h`. Scans the document in a single O(N) pass
  regardless of the number of patterns. `ParseDocument` copies the document into
  a local buffer before scanning, so the scan survives edits made while the
  progress callback is pumping messages.

- **Bulk indicator fill** (`log-highlighter.cpp`): each `SCI_INDICATORFILLRANGE`
  normally fires an `SCN_MODIFIED(CHANGEINDICATOR)` notification back to
  Notepad++ over a synchronous `SendMessage` round-trip (~120 µs). At 64 k
  matches that alone costs 7+ seconds, so `ApplyHighlights` masks
  `SC_MOD_CHANGEINDICATOR` out of the mod-event mask for the duration of the
  fill and restores it afterwards.

- **Per-buffer state** (`Plugin.cpp`): match lists are keyed by NPP buffer ID.
  `NPPN_BUFFERACTIVATED` rebuilds the Overview Panel from the cached list;
  `NPPN_FILEBEFORECLOSE` drops it. No parse is ever triggered by a notification.

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
