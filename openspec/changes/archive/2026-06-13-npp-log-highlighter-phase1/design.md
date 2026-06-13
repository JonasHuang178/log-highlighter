## Project Setup

### MSVC Solution

| Setting | Value |
|---|---|
| Solution file | `log-highlighter.sln` |
| Project file | `log-highlighter.vcxproj` |
| Output type | DLL (`ConfigurationType = DynamicLibrary`) |
| Platform | x64 only (Debug\|x64, Release\|x64) |
| Toolset | MSVC v143 (VS 2022) |
| Language standard | C++17 (`stdcpp17`) |
| Character set | Unicode |
| Export definitions | `log-highlighter.def` |

### Include Directories (AdditionalIncludeDirectories)

```
$(ProjectDir)external   <- Notepad++ & Scintilla SDK headers
$(ProjectDir)src        <- internal headers
$(ProjectDir)config     <- user config headers
```

### Exports (log-highlighter.def)

```
LIBRARY LogHighlighter
EXPORTS
    isUnicode
    getName
    getFuncsArray
    setInfo
    beNotified
    messageProc
```

Note: `LIBRARY LogHighlighter` is the DLL identity name used internally.
The physical file is `log-highlighter.dll`.

---

## Project Structure

```
log-highlighter/
├── log-highlighter.sln
├── log-highlighter.vcxproj
├── log-highlighter.vcxproj.filters   <- Solution Explorer folder layout
├── log-highlighter.def
├── external/                         <- SDK headers, do not modify
│   ├── PluginInterface.h             <- NppData, FuncItem, ShortcutKey, SCNotification
│   ├── Scintilla.h                   <- SCI_* message constants
│   └── SciLexer.h                   <- SCLEX_* constants (placeholder)
├── config/                           <- user-editable, recompile to apply
│   ├── LogPatterns.h                 <- keyword rules and colors
│   └── AboutInfo.h                   <- plugin version and about text
└── src/
    ├── dllmain.cpp                   <- DllMain (no-op for all cases)
    ├── Plugin.h / Plugin.cpp         <- Notepad++ API exports, menu, notifications
    ├── Parser.h / Parser.cpp         <- document scanning, returns vector<Match>
    └── Highlighter.h / Highlighter.cpp  <- Scintilla indicator setup and rendering
```

---

## external/ — Notepad++ & Scintilla SDK

### PluginInterface.h

Defines the minimal Notepad++ plugin interface:

```cpp
struct ShortcutKey { bool _isCtrl; bool _isAlt; bool _isShift; UCHAR _key; };
struct FuncItem    { TCHAR _itemName[64]; PFUNCPLUGINCMD _pFunc; int _cmdID;
                     bool _init2Check; ShortcutKey* _pShKey; };
struct NppData     { HWND _nppHandle; HWND _scintillaMainHandle;
                     HWND _scintillaSecondHandle; };
struct SCNotification { NMHDR nmhdr; intptr_t position; int ch; int modifiers;
                        int modificationType; const char* text; intptr_t length;
                        intptr_t linesAdded; ... };

enum ScintillaNotification : UINT {
    SCN_MODIFIED = 2008,
    SCN_UPDATEUI = 2007,
    ...
};

// SCN_MODIFIED modificationType flags
static constexpr int SC_MOD_INSERTTEXT      = 0x001;
static constexpr int SC_MOD_DELETETEXT      = 0x002;
static constexpr int SC_MOD_CHANGEINDICATOR = 0x4000;

enum NppMsg : UINT {
    NPPMSG                   = WM_USER + 1000,
    NPPM_GETCURRENTSCINTILLA = NPPMSG + 4,
    ...
};
```

### Scintilla.h

Key constants used by this plugin:

```cpp
// Document
#define SCI_GETLENGTH           2006
#define SCI_GETCHARACTERPOINTER 2520   // returns const char* to internal UTF-8 buffer

// Indicators
#define SCI_INDICSETSTYLE       2080
#define SCI_INDICSETFORE        2082
#define SCI_INDICSETALPHA       2523
#define SCI_INDICSETUNDER       2510
#define SCI_SETINDICATORCURRENT 2500
#define SCI_INDICATORFILLRANGE  2504
#define SCI_INDICATORCLEARRANGE 2505

// Indicator styles
#define INDIC_FULLBOXBLOCK      16   // fills background block
#define INDIC_TEXTFORE          17   // changes text foreground color only
```

---

## config/ — User Configuration

### LogPatterns.h

Defines two rule arrays. Adding/removing entries automatically adjusts all
indicator indices at compile time (via `sizeof` array counting).

```cpp
#define MAKE_BGR(r, g, b)  (((COLORREF)(b)<<16)|((COLORREF)(g)<<8)|(COLORREF)(r))

struct LogTypeRule  { const char* keyword; COLORREF textColor; };
struct StepTypeRule { const char* prefix;  COLORREF bgColor;   };

static const LogTypeRule LOG_TYPE_RULES[] = {
    { "[ ERROR ]",  MAKE_BGR(220,   0,   0) },
    { "[ WARN ]",   MAKE_BGR(200, 160,   0) },
    { "[ DEBUG ]",  MAKE_BGR( 70, 150, 255) },
};

static const StepTypeRule STEP_TYPE_RULES[] = {
    { "Step",  MAKE_BGR(180, 230, 180) },
};
```

### AboutInfo.h

```cpp
#define PLUGIN_VERSION  L"v1.0"
#define PLUGIN_NAME     L"LogHighlighter"
#define ABOUT_TITLE     PLUGIN_NAME L" " PLUGIN_VERSION
#define ABOUT_CONTENT   L"Plugin : LogHighlighter " PLUGIN_VERSION L"\n" \
                        L"Author : (your name)"                    L"\n"
```

---

## src/ — Implementation

### Plugin.h / Plugin.cpp

**Responsibilities:** Notepad++ API exports, menu registration, real-time
notification handling.

**Globals:**
```cpp
NppData g_nppData = {};             // set by setInfo(), holds all HWNDs
static FuncItem    g_funcItems[2];  // [0]=Parse Log, [1]=About
static ShortcutKey g_parseLogKey;
static std::vector<Match> g_matches;
static bool g_highlightActive = false;  // guards SCN_MODIFIED handler
```

**Required exports (all in `extern "C"`):**

| Export | Purpose |
|---|---|
| `isUnicode()` | Returns `true` — plugin uses Unicode strings |
| `getName()` | Returns `TEXT("LogHighlighter")` — shown in Plugins menu |
| `getFuncsArray(int* nbF)` | Registers menu items and shortcut keys |
| `setInfo(NppData)` | Stores `g_nppData` (called once on load) |
| `beNotified(SCNotification*)` | Handles `SCN_MODIFIED` for real-time update |
| `messageProc(UINT, WPARAM, LPARAM)` | Returns `TRUE` (no-op) |

**Menu registration in `getFuncsArray()`:**
```cpp
*nbF = 2;

// [0] Parse Log — Ctrl+Alt+Q
g_funcItems[0]._pFunc = ParseLog;
g_parseLogKey = { true, true, false, 'Q' };
g_funcItems[0]._pShKey = &g_parseLogKey;

// [1] About — no shortcut
g_funcItems[1]._pFunc  = ShowAbout;
g_funcItems[1]._pShKey = nullptr;
```

**Helper:**
```cpp
HWND GetCurrentScintilla() {
    int view = 0;
    SendMessage(g_nppData._nppHandle, NPPM_GETCURRENTSCINTILLA, 0, (LPARAM)&view);
    return view == 0 ? g_nppData._scintillaMainHandle
                     : g_nppData._scintillaSecondHandle;
}
```

**ParseLog() flow:**
```
1. GetCurrentScintilla()
2. InitStyles(hSci)
3. g_matches = ParseDocument(hSci)
4. ClearAllHighlights(hSci)
5. ApplyHighlights(hSci, g_matches, repaintAfter=true)
6. g_highlightActive = true
```

**beNotified() — real-time handler:**
```cpp
case SCN_MODIFIED:
    if (!g_highlightActive) break;   // guard: must run ParseLog once first
    if (!(modificationType & (SC_MOD_INSERTTEXT | SC_MOD_DELETETEXT))) break;
    // SC_MOD_CHANGEINDICATOR (0x4000) is excluded — prevents infinite loop
    HWND hSci = (HWND)notification->nmhdr.hwndFrom;
    InitStyles(hSci);
    g_matches = ParseDocument(hSci);
    ClearAllHighlights(hSci);
    ApplyHighlights(hSci, g_matches, repaintAfter=false);
```

---

### Parser.h / Parser.cpp

**Responsibilities:** Scan the active Scintilla document and return all matches.

**Types:**
```cpp
enum class MatchType { LOG_TYPE, STEP_TYPE };

struct Match {
    MatchType type;       // which rule array
    int       ruleIndex;  // index into LOG_TYPE_RULES[] or STEP_TYPE_RULES[]
    intptr_t  byteOffset; // start byte in document
    intptr_t  length;     // byte length of highlighted range
};
```

**ParseDocument() — entry point:**
```cpp
std::vector<Match> ParseDocument(HWND hScintilla);
```

**Buffer access:**
```cpp
const char* text = (const char*)SendMessage(hSci, SCI_GETCHARACTERPOINTER, 0, 0);
```
`SCI_GETCHARACTERPOINTER` returns a direct pointer into Scintilla's internal
UTF-8 buffer. No allocation. Valid until the next text-modifying call.
Scanning always completes before `ApplyHighlights` is called.

**Log Type matching:**
- For each rule in `LOG_TYPE_RULES[]`: `strstr` scan across full document
- `length` = `strlen(keyword)`

**Step Type matching:**
- For each rule in `STEP_TYPE_RULES[]`: `strstr` scan for `prefix`
- After prefix: require `\d+` (at least one digit)
- After digits: require `' '` (space) — rejects `Step1init`, `Step1\t`
- `length` = from prefix start to last char before `\r` or `\n`

---

### Highlighter.h / Highlighter.cpp

**Responsibilities:** Configure Scintilla indicators and apply/clear highlight ranges.

**Indicator index layout:**
```
11 .. (11 + LOG_RULE_COUNT  - 1)  ->  Log Type   INDIC_TEXTFORE
14 .. (14 + STEP_RULE_COUNT - 1)  ->  Step Type  INDIC_FULLBOXBLOCK
```

Indices are computed at compile time:
```cpp
static constexpr int LOG_RULE_COUNT  = sizeof(LOG_TYPE_RULES)  / sizeof(LOG_TYPE_RULES[0]);
static constexpr int STEP_RULE_COUNT = sizeof(STEP_TYPE_RULES) / sizeof(STEP_TYPE_RULES[0]);
static constexpr int INDIC_LOG_BASE  = 11;
static constexpr int INDIC_STEP_BASE = INDIC_LOG_BASE + LOG_RULE_COUNT;
```

Adding a rule in `LogPatterns.h` automatically shifts all subsequent indices.

Indices 0–10 are avoided: 0–7 for Scintilla lexer built-ins, 8 for
Notepad++ Smart Highlight (clears full document on cursor move), 9–10 unsafe.

**InitStyles():**
```cpp
// Log Type: text foreground color only, no background change
SCI_INDICSETSTYLE(idx, INDIC_TEXTFORE)
SCI_INDICSETFORE (idx, textColor)

// Step Type: solid background fill, rendered under text
SCI_INDICSETSTYLE(idx, INDIC_FULLBOXBLOCK)
SCI_INDICSETFORE (idx, bgColor)
SCI_INDICSETALPHA(idx, 255)   // fully opaque
SCI_INDICSETUNDER(idx, TRUE)  // draw below text layer
```

**ClearAllHighlights():**
```cpp
for each indicator index:
    SCI_SETINDICATORCURRENT(idx)
    SCI_INDICATORCLEARRANGE(0, docLen)
```

**ApplyHighlights(repaintAfter):**
```cpp
if (repaintAfter) WM_SETREDRAW(FALSE)
for each match:
    idx = (LOG_TYPE) ? INDIC_LOG_BASE + ruleIndex
                     : INDIC_STEP_BASE + ruleIndex
    SCI_SETINDICATORCURRENT(idx)
    SCI_INDICATORFILLRANGE(byteOffset, length)
if (repaintAfter) WM_SETREDRAW(TRUE) + InvalidateRect()
// repaintAfter=false: used from SCN_MODIFIED, Scintilla repaints itself
```

---

## Indicator API vs Styling API

The Scintilla Styling API (`SCI_STARTSTYLING` / `SCI_SETSTYLING`) is owned
by the lexer pipeline. Notepad++ re-runs its lexer on every repaint via
`SCN_STYLENEEDED`, overwriting any plugin-applied styles.

The Indicator API is independent of the lexer. Scintilla automatically
adjusts indicator ranges when text is inserted or deleted, so highlights
survive all editing operations. This is the only reliable way for a plugin
to apply persistent visual decoration.
