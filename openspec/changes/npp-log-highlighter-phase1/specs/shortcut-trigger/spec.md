## ADDED Requirements

### Requirement: Ctrl+Alt+1 快速鍵觸發掃描
Plugin SHALL 註冊 Ctrl+Alt+1 快速鍵，按下時觸發 log 掃描流程。

#### Scenario: 快速鍵觸發
- **WHEN** 使用者在 Notepad++ 中按下 Ctrl+Alt+1
- **THEN** 對當前作用中的編輯器文件執行 log 掃描與 highlight

#### Scenario: 無文件時按下快速鍵
- **WHEN** 使用者按下 Ctrl+Alt+1 且 Notepad++ 沒有開啟任何文件
- **THEN** 不執行任何動作，不顯示錯誤

### Requirement: 選單項目
Plugin SHALL 在 Plugins > LogHighlighter 選單下提供 "Parse Log (Ctrl+Alt+1)" 項目，與快速鍵觸發相同功能。

#### Scenario: 點選選單項目
- **WHEN** 使用者點選 Plugins > LogHighlighter > Parse Log
- **THEN** 執行與 Ctrl+Alt+1 相同的掃描與 highlight 流程
