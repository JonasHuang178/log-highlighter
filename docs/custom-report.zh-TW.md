# Custom Report 開發文件

> 對應功能：**Ctrl+Alt+E** — 讓使用者自行定義 log 解析方式，並把結果呈現給使用者。
>
> 規格來源：`openspec/specs/custom-report/spec.md`、`openspec/specs/report-dialog/spec.md`、
> `openspec/specs/report-debug-console/spec.md`
>
> 本文件中所有輸出範例皆由 `config/CustomReports.h` 內實際出貨的報表函式執行產生，非手寫示意。

---

## 1. 這個功能是什麼

Notepad++ 外掛 log-highlighter 的其他兩個指令（Ctrl+Alt+Q 上色、Ctrl+Alt+W 跳轉）都依賴
`config/LogPatterns.h` 裡預先定義的關鍵字規則。**Custom Report 不預設任何 log 格式。**

使用者在 `config/CustomReports.h` 裡用 C++ 寫一個函式，決定要從文件中抽出什麼、怎麼彙總、
怎麼排版，重新編譯後就會在選單多出一個項目。按下去，外掛把目前分頁的內容交給那個函式，
把函式產出的文字顯示在一個唯讀、等寬字型、可選取複製的視窗裡。

規格把這個設計前提寫得很直接：

> 這是一個 capability 而非 feature，因為外掛刻意不對 log 格式做任何假設：報表要抽出什麼
> 完全是作者的決定，而外掛的義務是給作者一個安全的撰寫介面。

換句話說 —— 這個功能的價值不在於外掛「會解析什麼」，而在於它**把解析權交出去，同時確保
交出去的過程不會把編輯器弄壞**。後面第 4 節的「防火牆」與第 9 節的安全邊界，是這份文件
的重點，而不是附註。

### 和另外兩個指令的關係

三個指令**互相獨立**，沒有任何一個是另一個的前置條件。

| 快捷鍵 | 指令 | 需要先跑過別的指令嗎 |
|---|---|---|
| Ctrl+Alt+Q | Parse Log | 否 |
| Ctrl+Alt+W | Next Bookmark | 否 — 快取空的時候自己掃 |
| **Ctrl+Alt+E** | **Custom Report** | **否 — 剛開啟的檔案就能直接跑** |

---

## 2. 使用者看到的行為

### 2.1 選單與快捷鍵

`CUSTOM_REPORTS[]` 裡每一筆都會註冊成一個獨立的 Notepad++ 指令，出現在
**Plugins > log-highlighter** 底下，位置在既有指令之後、About 之前。

出貨預設有三個：

| 選單項目 | 函式 | 快捷鍵 |
|---|---|---|
| IP Report | `IpReport` | Ctrl+Alt+E |
| Severity Summary | `SeveritySummary` | 無 |
| Line Stats | `LineStats` | 無 |

`shortcut` 欄位填 `0` 代表不綁預設快捷鍵；使用者仍可在
**Settings > Shortcut Mapper > Plugin commands** 自行指定。

`CUSTOM_REPORTS[]` 留空也是合法的 —— 外掛正常載入，只是不會有任何報表選單項目。

### 2.2 執行過程

1. 進度對話框出現，Notepad++ 主視窗被 `EnableWindow(FALSE)` 停用，防止重入
2. 報表函式跑在 **UI 執行緒**上（沒有 worker thread）
3. 只要作者是用 `ctx.Lines()` 或 `ctx.FindAll()` 迭代，進度會自動更新、Cancel 按鈕可按
4. 結束後結果顯示在報表視窗

按下 Cancel：迭代中止、**輸出整份丟棄**、不開任何視窗、文件不受影響。

### 2.3 結果視窗

| 特性 | 說明 |
|---|---|
| 模態 | 自己跑訊息迴圈 —— 否則 Notepad++ 的快速鍵表會攔截 Ctrl+C |
| 唯讀 | `ES_READONLY`，使用者打字不會改變內容 |
| 等寬字型 | 讓對齊後的欄位真的對齊 |
| 無長度上限 | `EM_SETLIMITTEXT` 帶 `wParam = 0`，超過 32767 字元不會被截斷 |
| 可調整大小 | 文字區域跟著 client area 縮放 |
| 複製 | Ctrl+C，或右鍵選單的 Copy / Select All |
| 關閉 | ESC、關閉鈕、Close 控制項 |

### 2.4 快取

結果**以分頁為單位快取**。同一個報表在同一個 buffer 上再按一次，直接顯示快取內容，不會
重新走訪文件。文件被編輯後（`SCN_MODIFIED` 的插入/刪除）快取失效，下次呼叫會重新解析。

例外：**debug 模式會完全略過快取**（見第 8 節）。

---

## 3. 資料流

```
使用者按下 Ctrl+Alt+E
        │
        ▼
Plugin.cpp  ReportThunk<N>()  ──► Report.cpp  RunCustomReport(N)
        │
        ├─ 檢查 stale → InvalidateIfStale(buf)
        ├─ 快取命中且非 debug 模式 ──────────────► ShowReportDialog()  （結束）
        │
        ├─ SnapshotDocument(hSci)      整份文件複製到 std::vector<char>
        ├─ CreateProgressDialog()      + EnableWindow(hNpp, FALSE)
        │
        ├─ 組出 ReportContext { text, length, lineCount, fileName, filePath, sink }
        │
        ├─ CUSTOM_REPORTS[N].fn(ctx, out)      ◄── 使用者的函式在這裡執行
        │                                           （沒有 SEH 保護，見 9.1）
        ├─ 取消？ → 丟棄輸出，結束
        │
        ├─ out.Render()  →  std::string（LF 換行）
        ├─ 依 SCI_GETCODEPAGE 轉成 UTF-16
        ├─ 前置產生 header（標題 / 檔名 / 行數）
        ├─ LF → CRLF
        ├─ 存入 buf.reportText 快取
        └─ ShowReportDialog()
```

### 相關檔案

| 檔案 | 角色 |
|---|---|
| `config/CustomReports.h` | **使用者編輯的唯一檔案** — 報表函式 + 註冊表 + debug 旗標 |
| `src/ReportApi.h` | 作者面向的 API（防火牆），無平台型別 |
| `src/Report.cpp` | 引擎：快照、進度、編碼、快取、debug 輸出 |
| `src/Report.h` | `Plugin.cpp` 唯一能看到的報表介面 |
| `src/ReportDialog.cpp` | 模態唯讀結果視窗 |
| `src/DebugConsole.cpp` | debug 模式的 console |
| `src/AhoCorasick.h` | `ctx.FindAll()` 底層的多模式自動機 |

---

## 4. 架構重點：防火牆

`src/ReportApi.h` 標頭最上方寫著：

```
//  This header is the firewall between config/CustomReports.h and the rest of
//  the plugin. It deliberately contains no <windows.h>, no Scintilla, and no
//  Parser.h: a report function never sees HWND, SendMessage, SCI_* or Match.
//  Keep it that way — if a platform type ever appears here, the whole point of
//  the design is lost.
```

所有東西都只用 `std::string_view` 和 `int` 跨越邊界。這帶來三個具體後果：

**1. 報表作者不可能誤用 Scintilla API。** 他拿不到 `HWND`，也就沒辦法在報表函式裡
`SendMessage` 去改文件。報表在定義上是唯讀的。

**2. `ctx.FindAll()` 能重用掃描器。** `AhoCorasick.h` 同樣刻意不含平台型別，而且用
呼叫端自選的整數識別 pattern、不認識規則表 —— 所以 `Parser.cpp`（給上色用）和
`ReportApi.h`（給報表用）可以共用同一份實作。

**3. 報表邏輯可以脫離 Notepad++ 單獨編譯測試。** 本文件第 7 節的所有輸出，就是把
`CustomReports.h` 連同 `ReportApi.h` 用 `cl /std:c++17` 單獨編譯成一個 console 程式跑出來的，
全程沒有 Notepad++、沒有 Scintilla。只需要補三個引擎符號：

```cpp
bool g_reportDebugEnabled  = false;
int  g_reportDebugMaxLines = 0;
void DebugWrite(const char*, size_t) {}
```

這是驗證防火牆是否還完整的最快方法：**哪天這個 harness 編不過了，就代表有平台型別漏進
`ReportApi.h` 了。**

---

## 5. 撰寫一份報表

### 5.1 函式簽章與註冊

```cpp
static void MyReport(const ReportContext& ctx, ReportBuilder& out)
{
    // 讀 ctx，寫 out
}

static const CustomReport CUSTOM_REPORTS[] = {
//   選單標題           函式        快捷鍵（0 = 無）
    { L"My Report",   MyReport,   'R' },
};
```

`CustomReport` 三個欄位：

| 欄位 | 型別 | 說明 |
|---|---|---|
| `title` | `const wchar_t*` | 選單項目文字，同時作為結果視窗標題與產生的 header |
| `fn` | `ReportFn` | `void (const ReportContext&, ReportBuilder&)` |
| `shortcut` | `char` | Ctrl+Alt+`<字母>`，或 `0` 表示不綁 |

函式寫了但沒登記進 `CUSTOM_REPORTS[]` → 不會有選單項目。

### 5.2 讀取文件 — `ReportContext`

| 成員 | 說明 |
|---|---|
| `ctx.Lines()` | `for (auto [lineNo, line] : ctx.Lines())` |
| `ctx.FindAll("kw")` | `for (auto hit : ctx.FindAll("kw"))` |
| `ctx.FindAll({"a","b"})` | 單趟 Aho-Corasick，十個關鍵字和一個同價 |
| `ctx.text` / `ctx.length` | 原始快照位元組 — **沒有進度、沒有取消** |
| `ctx.lineCount` | 總行數 |
| `ctx.fileName` / `ctx.filePath` | `const wchar_t*` |

**`Lines()` 的語意**（規格明訂，逐條都有對應 scenario）：

- 行號 **1-based**，和 Notepad++ 左側行號欄一致
- `line` **不含**換行字元
- CRLF 檔案的尾端 `\r` 已經幫你去掉
- **空行照樣產出**，所以行號不會跟編輯器跑掉
- 最後一行沒有換行結尾時，仍然會產出

**`Hit` 的欄位**：

| 欄位 | 說明 |
|---|---|
| `hit.keyword` | 命中的是哪個關鍵字 |
| `hit.lineNo` | 1-based 行號 |
| `hit.line` | 整行 |
| `hit.after` | 該行從關鍵字後面開始的剩餘部分 |

關鍵字在行尾時，`after` 為空，`line` 仍是完整那一行。

### 5.3 產生輸出 — `ReportBuilder`

| 呼叫 | 輸出 |
|---|---|
| `out.Section("Title")` | `---- Title -----------------`，並重置對齊 |
| `out.Line(text)` | 自由文字，不參與對齊 |
| `out.KV(key, value)` | `key : value`，value 可以是文字或任意數值 |
| `out.KVf(key, fmt, ...)` | value 用 printf 格式描繪 |
| `out.AtLine(n, text)` | `L  142 : text` |
| `out.Blank()` | 空行 |

`KV` 有四個多載：`string_view`、`const char*`、整數（走 `%lld`）、浮點數（走 `%g`）。

### 5.4 字串輔助函式

| 函式 | 結果 |
|---|---|
| `After(s, kw)` | 第一個 `kw` 之後的文字 |
| `Before(s, kw)` | 第一個 `kw` 之前的文字 |
| `Between(s, a, b)` | `a` 與其後第一個 `b` 之間 |
| `Field(s, delim, n)` | 第 n 個欄位，0-based；連續分隔符產生空欄位而不折疊 |
| `Trim(s)` | 去除前後空白 |
| `Contains` / `StartsWith` / `EndsWith` | `bool` |
| `ToInt(s, out)` / `ToDouble(s, out)` | `bool`；失敗時 `out` 不動 |

**核心契約：抽取類函式失敗時回傳空值，絕不越界讀取，絕不丟例外。空輸入給出空輸出。**

這就是它們可以安全巢狀、而不必每層都檢查的原因：

```cpp
auto ip = Field(After(line, "IP: "), ' ', 0);
if (ip.empty()) continue;          // 一次檢查涵蓋兩個步驟
```

這個契約不是方便性設計 —— 它是**沒有 crash guard 的補償機制**（見 9.1）。

---

## 6. 輸出格式規格

這一節描述 `ReportBuilder::Render()` 的確切行為，寫報表時若要預測排版結果可以對照。

### 6.1 引擎自動產生的 header

作者不需要自己寫，引擎一律前置：

```
<title> - <fileName>  (<行數，三位一撇> lines)
<與上一行等長的 = 底線>
<空行>
```

### 6.2 Section 標題列

```
"---- " + title + " " ，然後補 '-' 直到總長度達到 REPORT_SECTION_WIDTH (= 46)
```

### 6.3 對齊規則

**對齊群組以 `Section` 為界。** 每個 Section 到下一個 Section 之間的列自成一組，各組
獨立計算欄寬 —— 所以短 key 的區段不會被後面長 key 的區段拉寬。

群組欄寬的計算方式：

```
keyWidth = max(該組所有 KV/KVf 的 key 長度)
若該組有 AtLine：keyWidth = max(keyWidth, 最大行號位數 + 1)   // 'L' + 數字
```

渲染：

| 列型別 | 輸出 |
|---|---|
| `KV` / `KVf` | `Pad(key, keyWidth)` + `" : "` + value |
| `AtLine` | `Pad("L" + 右對齊行號, keyWidth)` + `" : "` + value |
| `Line` | value（完全不對齊） |
| `Blank` | 空行 |

### 6.4 空輸出

報表函式若一個 `ReportBuilder` 方法都沒呼叫，引擎在內文顯示 `(no output)`
（header 仍然會有）。

### 6.5 編碼

引擎查詢 `SCI_GETCODEPAGE`：code page 65001 用 `CP_UTF8` 轉換，其餘用 `CP_ACP`。
header 直接以 UTF-16 組出且除檔名外均為 ASCII，所以不受文件編碼影響。

---

## 7. 完整範例

以下全部使用同一份輸入。建立 `sample.log`：

```
10:23:40 [ DEBUG ] boot sequence start
10:23:41 Step1 initialise subsystems
10:23:42 IP: 192.168.1.10 connected
10:23:43 [ WARN ] retry 1
10:23:44 IP: 10.0.0.3 connected
10:23:45 [ ERROR ] link down
10:23:46 IP: 192.168.1.10 disconnected
10:23:47 Start test alpha
```

> 檔案共 264 bytes。`lineCount` 是 **9** 而非 8 —— 最後一行的換行字元之後還有一個空行，
> Scintilla 的 `SCI_GETLINECOUNT` 會把它算進去。這正是 `Lines()` 保證「空行照樣產出」
> 的實際後果。

### 7.1 範例一 — 最小的報表

什麼都不掃，`ctx` 本身就知道文件大小。

```cpp
static void LineStats(const ReportContext& ctx, ReportBuilder& out)
{
    out.KV("Lines", ctx.lineCount);
    out.KV("Bytes", ctx.length);
}
```

輸出：

```
Line Stats - sample.log  (9 lines)
==================================

Lines : 9
Bytes : 264
```

沒有任何 `Section`，所以全部列同屬一個對齊群組，`keyWidth` = `max("Lines", "Bytes")` = 5。

### 7.2 範例二 — 逐行走訪、抽欄位、彙總

```cpp
static void IpReport(const ReportContext& ctx, ReportBuilder& out)
{
    std::map<std::string_view, int> hits;        // string_view 當 key：不複製字元
    std::map<std::string_view, int> firstLine;

    for (auto [lineNo, line] : ctx.Lines())
    {
        auto ip = Field(After(line, "IP: "), ' ', 0);
        if (ip.empty()) continue;

        if (++hits[ip] == 1)
            firstLine[ip] = lineNo;
    }

    out.Section("IP addresses");

    if (hits.empty())
    {
        out.Line("none found");
        return;
    }

    for (const auto& [ip, n] : hits)
        out.KVf(ip, "%5d hits   first @ L%d", n, firstLine[ip]);

    out.Blank();
    out.KV("Unique addresses", hits.size());
}
```

抽取過程：

| 步驟 | 值 |
|---|---|
| 輸入 | `10:23:42 IP: 192.168.1.10 connected` |
| `After(line, "IP: ")` | `192.168.1.10 connected` — 行內沒有 `IP: ` 時為空 |
| `Field(..., ' ', 0)` | `192.168.1.10` |

輸出：

```
IP Report - sample.log  (9 lines)
=================================

---- IP addresses ----------------------------
10.0.0.3         :     1 hits   first @ L5
192.168.1.10     :     2 hits   first @ L3

Unique addresses : 2
```

幾個值得對照的細節：

- `Section` 之後的所有列同屬一組，`keyWidth` 被最長的 `"Unique addresses"`（16 字元）
  決定，所以兩個 IP 的冒號被推到第 18 欄
- `%5d` 產生的 4 個前導空白在 value 內部，與 key 對齊無關
- `std::map` 依 `string_view` 字典序排列，所以 `10.0.0.3` 排在 `192.168.1.10` 前面
  （`'0'` < `'9'`），不是依出現順序

### 7.3 範例三 — 單趟掃多個關鍵字

`ctx.FindAll()` 對所有關鍵字跑**單趟** Aho-Corasick，十個關鍵字和一個成本相同。
比起每行呼叫十次 `Contains()` 快得多。

```cpp
static void SeveritySummary(const ReportContext& ctx, ReportBuilder& out)
{
    int errors = 0, warns = 0, debugs = 0;
    int              firstErrLine = 0;
    std::string_view firstErrText;      // 可以安全保留 — 見 9.2

    for (auto hit : ctx.FindAll({ "[ ERROR ]", "[ WARN ]", "[ DEBUG ]" }))
    {
        if (hit.keyword == "[ ERROR ]")
        {
            ++errors;
            if (!firstErrLine)
            {
                firstErrLine = hit.lineNo;
                firstErrText = hit.line;
            }
        }
        else if (hit.keyword == "[ WARN ]") ++warns;
        else                                ++debugs;
    }

    out.Section("Severity");
    out.KV("ERROR", errors);
    out.KV("WARN",  warns);
    out.KV("DEBUG", debugs);

    if (firstErrLine)
    {
        out.Blank();
        out.Section("First error");
        out.AtLine(firstErrLine, Trim(firstErrText));
    }
}
```

輸出：

```
Severity Summary - sample.log  (9 lines)
========================================

---- Severity --------------------------------
ERROR : 1
WARN  : 1
DEBUG : 1

---- First error -----------------------------
L6 : 10:23:45 [ ERROR ] link down
```

這裡可以清楚看到 **6.3 的分組對齊**：第一組 `keyWidth` = 5（`ERROR`/`DEBUG`），
第二組只有一筆 `AtLine(6, ...)`，`keyWidth` = 1 位數 + 1 = 2，所以是 `L6 : ` 而不是被
第一組的 5 拉寬成 `L6    : `。兩組互不影響。

---

## 8. Debug 模式

寫 parser 的時候最快的除錯方式是直接印出每行抽到什麼。

```cpp
// config/CustomReports.h
#define REPORT_DEBUG_MODE       1      // 0 = 關閉（預設）
#define REPORT_DEBUG_MAX_LINES  1000   // 0 = 不限
```

重新編譯後，Notepad++ 啟動時會開一個 console，報表函式裡的 `Debug` / `Debugf` 會印到上面。

```cpp
Debugf("L%-5d raw=[%s] ip=[%s]", lineNo, line, ip);
```

```
==== IP Report  -  sample.log ====
[engine] buf=0x1a2f  stale=true -> caches invalidated
[engine] report cache bypassed (debug mode)
[engine] snapshot 9 lines / 264 bytes
[engine] entering report function
L1     raw=[10:23:40 [ DEBUG ] boot sequence start] ip=[]
L3     raw=[10:23:42 IP: 192.168.1.10 connected] ip=[192.168.1.10]
... debug output suppressed after 1000 lines
[engine] report function returned, 5 rows
[engine] dialog shown
```

### 8.1 `Debugf` 為什麼是 variadic template

這是設計上的關鍵決定，不是實作細節：

- C 的 varargs **根本無法傳遞 `std::string_view`** —— 而那正是每個抽取輔助函式的回傳型別
- `printf("%s", 42)` 會把整數當指標解參考，直接讓 Notepad++ 崩潰

由於這個框架**不提供 crash guard**（9.1），除錯工具本身絕不能成為最容易弄垮編輯器的東西。
所以 `Debugf` 在執行期走訪格式字串，每個轉換只描繪一個模板已知型別的引數：

| 狀況 | 印出 |
|---|---|
| 轉換與引數型別不符 | `<!bad-arg>` |
| 引數不足 | `<!no-arg>` |

支援的轉換：`d i u o x X c`、`e E f F g G a A`、`s`、`%%`，含一般的寬度與精度。
`%s` 直接接受 `std::string_view`、`std::string`、`const char*`，不必拆成指標與長度。

### 8.2 `[engine]` 前綴

開頭是 `[engine]` 的是外掛自己印的 breadcrumb。它們**不計入輸出上限、也不會被上限壓掉** ——
所以「report function returned」這一行即使在作者輸出已被截斷的情況下仍然看得到，這正是
判斷「跑完了還是掛在中間」最需要的那一行。

### 8.3 使用上的注意

- **console 輸出很慢，而報表跑在 UI 執行緒上。** 請用小樣本檔案除錯；
  `REPORT_DEBUG_MAX_LINES` 的存在就是為了避免逐行列印讓 Notepad++ 看起來像當掉
- **debug 模式會繞過報表快取** —— 否則第二次按快捷鍵什麼都不會印，讀起來像除錯壞掉了
- debug 關閉時**引數仍會被求值**，所以出貨版本避免 `Debugf("%s", SomethingExpensive())`
- **console 在 Notepad++ 執行期間無法關閉。** 關閉 console 會送出 `CTRL_CLOSE_EVENT`
  給它的擁有者行程，預設處理會終止 Notepad++ 與所有未存檔分頁 —— 所以關閉鈕被
  `DeleteMenu(SC_CLOSE)` 移除了。請改用最小化
- console 隨行程一起死，所以**看不到崩潰前的最後幾行**

如果需要中斷點而不是印出：編譯 **Debug | x64** → 複製 DLL 到外掛資料夾 → 啟動 Notepad++ →
Visual Studio **Debug > Attach to Process** → `notepad++.exe` → 在報表函式下中斷點。

---

## 9. 安全邊界與陷阱

### 9.1 沒有 crash guard

**引擎直接呼叫報表函式，沒有 SEH 包裹。報表函式裡的野指標會連同 Notepad++ 和所有未存檔
分頁一起帶走。**

這是規格明訂的行為（`custom-report` 的 `No crash guard` 需求），不是疏漏。補償機制是：

> `ReportApi.h` 裡每一個輔助函式都回傳空值而非越界讀取，所以只要作者待在提供的 API 上，
> 就**構造不出**非法存取。

實務上的意思是：**待在 `ctx.Lines()`、`ctx.FindAll()` 和輔助函式上，就不會越界。**
直接操作 `ctx.text` 就是自己承擔。

### 9.2 string_view 的生命週期

引擎在呼叫報表函式前把文件複製進一個連續緩衝區，並保證它**活到函式回傳為止**。
所有從 `ctx` 或輔助函式取得的 `string_view` 都指向那個緩衝區。

| 可以 | 不可以 |
|---|---|
| 收集進 `std::map` / `std::vector`，迴圈結束後再讀 | 存到函式外（static、全域） |
| 當 map 的 key（不複製任何字元） | 期待它在下一次報表執行時還有效 |

即使使用者在進度回呼泵訊息時編輯了文件，報表仍然跑在呼叫當下的快照上，看不到那次編輯。

### 9.3 `FindAll` 的關鍵字必須比迴圈活得久

字串字面量一定符合；**暫時的 `std::string` 不符合**。

```cpp
for (auto hit : ctx.FindAll("[ ERROR ]")) { }          // OK
std::string kw = BuildKeyword();
for (auto hit : ctx.FindAll(kw)) { }                   // OK — kw 活著
for (auto hit : ctx.FindAll(BuildKeyword())) { }       // 危險 — 暫時物件
```

### 9.4 非 ASCII 關鍵字必須存成 UTF-8 with BOM

**沒有 BOM 的話，MSVC 會用系統 ANSI code page 讀原始碼，你的字面量會靜默地永遠不匹配
UTF-8 文件。** 專案本身以 `/utf-8` 編譯。

### 9.5 `CustomReports.h` 只能被 `src/Report.cpp` include

它定義的是**函式**而不只是資料，被 include 第二次就會產生每個報表的第二份私有複本。

這也是為什麼 `Plugin.cpp` 只透過 `Report.h` 的存取函式來驅動選單。Notepad++ 的指令回呼
不吃參數，所以每個報表需要各自的函式指標 —— 由編譯期產生的 `ReportThunk<N>` 提供。

### 9.6 `ctx.text` 直接存取會放棄進度與取消

規格明訂：報表若直接讀 `ctx.text` 而非迭代，進度對話框顯示不確定訊息，Cancel 按下去
沒有作用。

### 9.7 已知數值上限

| 項目 | 上限 | 位置 |
|---|---|---|
| 可註冊的報表數 | 16（`kMaxReports`） | `Plugin.cpp` — 超過就改這裡 |
| `KVf` 單筆 value 長度 | 1024 bytes（含結尾） | `ReportApi.h`，超過會截斷 |
| 進度更新間隔 | 每 500 行（`REPORT_TICK_INTERVAL`） | `ReportApi.h` |
| Section 標題列寬度 | 46（`REPORT_SECTION_WIDTH`） | `ReportApi.h` |

---

## 10. 快速檢查清單

新增一份報表時：

- [ ] 函式簽章是 `void (const ReportContext&, ReportBuilder&)`
- [ ] 已加進 `CUSTOM_REPORTS[]`（否則不會出現在選單）
- [ ] 快捷鍵字母沒有和既有的衝突，或填 `0`
- [ ] 用 `ctx.Lines()` / `ctx.FindAll()` 迭代，而非直接走 `ctx.text`
- [ ] 巢狀輔助函式的結果有做一次 `.empty()` 檢查
- [ ] 保留的 `string_view` 沒有離開函式範圍
- [ ] 多關鍵字用單次 `FindAll({...})` 而非多次 `Contains()`
- [ ] 有非 ASCII 字面量的話，檔案存成 UTF-8 with BOM
- [ ] 重新編譯 **Release | x64**，DLL 複製到
      `%APPDATA%\Notepad++\plugins\log-highlighter\`
- [ ] 出貨前把 `REPORT_DEBUG_MODE` 改回 `0`
