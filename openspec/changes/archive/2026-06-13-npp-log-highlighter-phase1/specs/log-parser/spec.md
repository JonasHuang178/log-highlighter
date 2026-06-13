## ADDED Requirements

### Requirement: 分塊掃描文件
Parser SHALL 以 64KB 為單位分塊讀取文件，不一次複製整份文件到記憶體，以支援大型 log 檔案。

#### Scenario: 小型文件（< 64KB）
- **WHEN** 文件大小小於 64KB
- **THEN** 一次掃描完成，回傳所有 match 的 byte offset 與長度

#### Scenario: 大型文件（> 64KB）
- **WHEN** 文件大小超過 64KB
- **THEN** 分多個 chunk 掃描，所有 chunk 的結果合併後回傳完整 match 清單

### Requirement: 跨 chunk 邊界不漏掃
Parser SHALL 在每個 chunk 結尾保留 16 bytes overlap，確保跨邊界的關鍵字不被遺漏。

#### Scenario: 關鍵字跨 chunk 邊界
- **WHEN** `[ ERROR ]` 字串橫跨兩個 64KB chunk 的邊界
- **THEN** 該關鍵字仍被正確偵測，回傳正確的 byte offset

### Requirement: 識別三種 log 等級關鍵字
Parser SHALL 識別以下固定格式的關鍵字（區分大小寫，全大寫）：
- `[ ERROR ]`
- `[ WARN ]`
- `[ DEBUG ]`

#### Scenario: 找到所有 ERROR
- **WHEN** 文件中有 3 個 `[ ERROR ]`
- **THEN** 回傳清單包含 3 筆 ERROR type 的 match，每筆含正確 byte offset 與長度

#### Scenario: 大小寫不符不匹配
- **WHEN** 文件中有 `[ error ]` 或 `[ Error ]`
- **THEN** 不回傳該字串為 match
