# log-highlighter

A Notepad++ 64-bit plugin that colorizes log keywords and step markers in real time.

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
- A blue rectangle tracking the current viewport position
- **Click** anywhere on the panel to jump to that position in the document

### Parse time display

After each **Ctrl+Alt+Q**, the Notepad++ status bar (bottom-left, where "Normal text file" appears) shows the total parse + render time:

| Duration | Format example |
|---|---|
| Under 60 seconds | `log-highlighter: parsed in 0.042 s` |
| 1 minute or more | `log-highlighter: parsed in 01:05.234` |
| 1 hour or more | `log-highlighter: parsed in 01:02:03.456` |

### Real-time re-highlight

After pressing **Ctrl+Alt+Q** once, any text change (insert or delete) in the document triggers an automatic full re-parse and re-highlight — no need to press the shortcut again.

---

## Usage

| Action | Result |
|---|---|
| **Ctrl+Alt+Q** | Scan the active document and apply all highlights |
| **Plugins > log-highlighter > Parse Log** | Same as Ctrl+Alt+Q |
| **Plugins > log-highlighter > About** | Show plugin version |

### Large files (≥ 5 000 lines)

A modeless progress dialog appears showing `Processing: N / M lines` with a **Cancel** button.
Parsing runs on a background thread so the dialog stays responsive.
Pressing Cancel leaves the document in its original (un-highlighted) state.

### Lazy rendering

Highlights are applied to the visible viewport + 1 000 lines ahead at parse time.
The remaining off-screen highlights are applied automatically as you scroll,
so response after Ctrl+Alt+Q is always fast regardless of file size.

> **Note** — the Overview Panel always shows marks for the entire document
> immediately after parsing, because it reads from the in-memory match list,
> not from Scintilla indicators.

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

Edit **`config/LogPatterns.h`** and rebuild. No other files need to change.

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
│   └── AboutInfo.h               ← plugin name, version, about text
└── src/                          ← implementation
    ├── dllmain.cpp
    ├── Plugin.h / Plugin.cpp     ← Notepad++ API exports, parse orchestration
    ├── Parser.h / Parser.cpp     ← Aho-Corasick single-pass scanner
    ├── log-highlighter.h / .cpp  ← Scintilla indicator styles & lazy rendering
    ├── OverviewPanel.h / .cpp    ← non-client-area minimap panel
    └── ProgressDialog.h / .cpp   ← modeless progress window for large files
```

### Key implementation notes

- **Scanner** (`Parser.cpp`): Aho-Corasick automaton built once at startup from
  `LogPatterns.h`. Scans the document in a single O(N) pass regardless of the
  number of patterns. Safe to call from any thread.

- **Lazy rendering** (`log-highlighter.cpp`): Scintilla indicators are applied
  only for the visible viewport + 1 000-line lookahead on first parse.
  `SCN_UPDATEUI` extends coverage as the user scrolls. Eliminates the per-fill
  `SCN_MODIFIED(CHANGEINDICATOR)` notification overhead that would otherwise
  make large files take several seconds to render.

- **Overview Panel** (`OverviewPanel.cpp`): Implemented via Win32 window
  subclassing of the Scintilla HWND (`WM_NCCALCSIZE` / `WM_NCPAINT`).
  No separate child window is created; the panel strip is carved out of
  Scintilla's non-client area.

- **Progress dialog** (`ProgressDialog.cpp`): Modeless `WS_POPUP` window.
  Parsing runs on a `std::thread`; the UI thread pumps messages via
  `MsgWaitForMultipleObjects` so the dialog stays alive and Cancel is responsive.
  `EnableWindow(hNpp, FALSE)` prevents re-entrant Ctrl+Alt+Q during parsing.
