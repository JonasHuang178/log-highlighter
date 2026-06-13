# LogHighlighter

A Notepad++ 64-bit plugin that colorizes log keywords and step markers in real time.

## Features

### Log Type — text color
Exact keyword match. Applies a foreground color to the keyword itself.

| Keyword | Color |
|---|---|
| `[ ERROR ]` | Red |
| `[ WARN ]` | Golden yellow |
| `[ DEBUG ]` | Cornflower blue |

### Step Type — background color
Matches `StepN ` (prefix + one or more digits + space).
Applies a background color from the keyword to the end of the line.

| Pattern | Example | Color |
|---|---|---|
| `StepN ` | `Step1 `, `Step12 `, `Step123 ` | Light green |

### Real-time highlighting
After pressing **Ctrl+Alt+Q** once, any keyword typed or pasted into the
document is highlighted immediately — no need to re-run the shortcut.

---

## Usage

| Action | Result |
|---|---|
| **Ctrl+Alt+Q** | Scan the active document and apply all highlights |
| **Plugins > LogHighlighter > Parse Log** | Same as Ctrl+Alt+Q |
| **Plugins > LogHighlighter > About** | Show plugin version info |

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

**Steps**
1. Open `log-highlighter\log-highlighter.sln`
2. Select **Release | x64**
3. Build → output is `x64\Release\LogHighlighter.dll`

---

## Customization

Edit **`config/LogPatterns.h`** and rebuild. No other files need to change.

### Add a Log Type keyword

```cpp
static const LogTypeRule LOG_TYPE_RULES[] = {
    { "[ ERROR ]",  MAKE_BGR(220,   0,   0) },  // red
    { "[ WARN ]",   MAKE_BGR(200, 160,   0) },  // golden yellow
    { "[ DEBUG ]",  MAKE_BGR( 70, 150, 255) },  // cornflower blue
    { "[ INFO ]",   MAKE_BGR(  0, 180,   0) },  // green  <-- add here
};
```

### Add a Step Type prefix

```cpp
static const StepTypeRule STEP_TYPE_RULES[] = {
    { "Step",   MAKE_BGR(180, 230, 180) },  // light green
    { "Phase",  MAKE_BGR(180, 210, 255) },  // light blue  <-- add here
};
```

### Color format

```cpp
MAKE_BGR(red, green, blue)   // values 0–255
```

### Update About / version

Edit **`config/AboutInfo.h`** and rebuild.

---

## Project Structure

```
log-highlighter/
├── log-highlighter.sln
├── log-highlighter.vcxproj
├── log-highlighter.vcxproj.filters
├── log-highlighter.def
├── external/                        <- Notepad++ & Scintilla API (do not modify)
│   ├── PluginInterface.h
│   ├── Scintilla.h
│   └── SciLexer.h
├── config/                          <- user-editable settings
│   ├── LogPatterns.h                <- keywords and colors
│   └── AboutInfo.h                  <- version and about text
└── src/                             <- implementation
    ├── dllmain.cpp
    ├── Plugin.h / Plugin.cpp        <- plugin entry & Notepad++ API exports
    ├── Parser.h / Parser.cpp        <- document scanning & match logic
    └── Highlighter.h / Highlighter.cpp  <- Scintilla indicator rendering
```

