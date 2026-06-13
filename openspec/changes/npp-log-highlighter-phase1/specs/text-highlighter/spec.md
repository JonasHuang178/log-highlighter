## ADDED Requirements

### Requirement: 三種 log 等級套用不同底色
Highlighter SHALL 使用 Scintilla Indicator API 對三種關鍵字套用不同背景色，indicator index 使用 8–10（避開 Notepad++ 內建 0–7）。

| 關鍵字      | Indicator Index | 底色 | BGR 值     |
|-------------|-----------------|------|------------|
| `[ ERROR ]` | 8               | 紅   | 0x0000FF   |
| `[ WARN ]`  | 9               | 黃   | 0x00FFFF   |
| `[ DEBUG ]` | 10              | 藍   | 0xFF8800   |

#### Scenario: ERROR 行套紅底色
- **WHEN** 掃描結果包含 `[ ERROR ]` 的 byte offset 與長度
- **THEN** 該範圍顯示紅色背景（INDIC_FULLBOXBLOCK 樣式）

#### Scenario: WARN 行套黃底色
- **WHEN** 掃描結果包含 `[ WARN ]` 的 byte offset 與長度
- **THEN** 該範圍顯示黃色背景

#### Scenario: DEBUG 行套藍底色
- **WHEN** 掃描結果包含 `[ DEBUG ]` 的 byte offset 與長度
- **THEN** 該範圍顯示藍色背景

### Requirement: 每次掃描前清除舊 highlight
Highlighter SHALL 在套用新 highlight 前，清除整份文件的所有三個 indicator，避免重複套色。

#### Scenario: 重複觸發 Ctrl+Alt+1
- **WHEN** 使用者連續按兩次 Ctrl+Alt+1
- **THEN** 第二次結果與第一次相同，不產生疊加或殘留

### Requirement: 批次重繪優化
Highlighter SHALL 在所有 `SCI_INDICATORFILLRANGE` 呼叫前後包夾 `SCI_SETREDRAW FALSE/TRUE`，減少 UI 刷新次數。

#### Scenario: 大量 highlight 不閃爍
- **WHEN** 文件包含 1000 個以上的關鍵字
- **THEN** highlight 完成後畫面一次更新，過程中不出現逐行閃爍
