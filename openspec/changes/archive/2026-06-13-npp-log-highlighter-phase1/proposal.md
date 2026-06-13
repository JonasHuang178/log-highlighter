## Status: COMPLETED

## Why

When analyzing log files, distinguishing error levels visually is critical.
Plain text display makes it slow and error-prone to manually scan through large
log files. This plugin provides instant visual feedback by colorizing log
keywords and step markers directly inside Notepad++.

## What

A Notepad++ 64-bit C++ Plugin DLL that:
- Colorizes `[ ERROR ]` / `[ WARN ]` / `[ DEBUG ]` with distinct text colors
- Highlights `StepN ` lines with a background color from the keyword to end of line
- Applies highlights on demand (Ctrl+Alt+Q) and in real time (typing/paste)
- Exposes an About dialog from the Plugins menu
- Allows keyword and color customization via `config/LogPatterns.h` (recompile to apply)

## Capabilities

| Capability | File(s) |
|---|---|
| Plugin scaffold & API exports | `src/Plugin.h`, `src/Plugin.cpp` |
| Ctrl+Alt+Q shortcut registration | `src/Plugin.cpp` → `getFuncsArray()` |
| Document scanning (two match types) | `src/Parser.h`, `src/Parser.cpp` |
| Scintilla indicator rendering | `src/Highlighter.h`, `src/Highlighter.cpp` |
| Real-time update on text change | `src/Plugin.cpp` → `beNotified()` |
| User-configurable rules | `config/LogPatterns.h` |
| About dialog content | `config/AboutInfo.h` |

## Output

- `log-highlighter.dll` (64-bit)
- Install path: `%APPDATA%\Notepad++\plugins\log-highlighter\log-highlighter.dll`
- Dependencies: Windows SDK, MSVC v143+
