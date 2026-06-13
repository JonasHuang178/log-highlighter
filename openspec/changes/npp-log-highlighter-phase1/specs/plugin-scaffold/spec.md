## ADDED Requirements

### Requirement: DLL 專案骨架
Plugin SHALL 以 MSVC .sln 專案形式組織，輸出 64-bit DLL，包含所有 Notepad++ Plugin API 必要匯出函數。

#### Scenario: 正確的 DLL 匯出
- **WHEN** Notepad++ 載入 LogHighlighter.dll
- **THEN** 能成功呼叫 `isUnicode()`、`getName()`、`getFuncsArray()`、`setInfo()`、`beNotified()`、`messageProc()` 六個匯出函數

#### Scenario: Plugin 出現在選單
- **WHEN** Notepad++ 啟動並載入 DLL
- **THEN** Plugins 選單下出現 "LogHighlighter" 子選單，包含 "Parse Log" 項目

### Requirement: SDK Headers 納入專案
專案 SHALL 包含 Notepad++ Plugin SDK 所需的 headers：`PluginInterface.h`、`Scintilla.h`、`SciLexer.h`，放置於 `include/` 目錄。

#### Scenario: 編譯成功
- **WHEN** 以 MSVC 2019+ x64 Release 組態建置
- **THEN** 無編譯錯誤，產出 `LogHighlighter.dll`（64-bit）
