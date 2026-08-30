## Purpose

The framework that lets a user write their own document parser in C++ and see
its output. It is a capability rather than a feature because the plugin
deliberately assumes nothing about log format: what a report extracts is
entirely the author's decision, and the plugin's obligation is to give them a
safe surface to write against.

## Requirements

### Requirement: Custom Report command registration
The plugin SHALL register one Notepad++ command per entry in `CUSTOM_REPORTS[]`, appearing in the
Plugins > log-highlighter menu after the existing commands and before About.

#### Scenario: Reports appear in the menu
- **WHEN** the plugin is loaded
- **THEN** each entry in `CUSTOM_REPORTS[]` appears as a menu item using its `title` field

#### Scenario: Empty report table
- **WHEN** `CUSTOM_REPORTS[]` is empty
- **THEN** the plugin loads normally and no report menu items appear

---

### Requirement: Ctrl+Alt+E shortcut
An entry in `CUSTOM_REPORTS[]` whose `shortcut` field is a letter SHALL be bound to
Ctrl+Alt+`<letter>`. An entry whose `shortcut` field is `0` SHALL be registered without a default
shortcut.

#### Scenario: Shipped default
- **WHEN** the user presses Ctrl+Alt+E
- **THEN** the report registered with shortcut `'E'` runs against the active document

#### Scenario: No default shortcut
- **WHEN** an entry has `shortcut = 0`
- **THEN** the command appears in the menu and is invocable by clicking it

---

### Requirement: Report function signature
A report function SHALL have the signature
`void (const ReportContext& ctx, ReportBuilder& out)` and SHALL be registered by adding a row to
`CUSTOM_REPORTS[]` in `config/CustomReports.h`.

#### Scenario: Function defined but not registered
- **WHEN** a report function exists in `CustomReports.h` but has no row in `CUSTOM_REPORTS[]`
- **THEN** no menu item is created for it

---

### Requirement: CustomReport struct
`CustomReport` in `src/ReportApi.h` SHALL have three fields:

| Field | Type | Description |
|---|---|---|
| `title` | `const wchar_t*` | Menu item text under Plugins > log-highlighter |
| `fn` | `ReportFn` | The report function to invoke |
| `shortcut` | `char` | Letter for Ctrl+Alt+`<letter>`, or `0` for none |

#### Scenario: Title used as dialog caption
- **WHEN** a report runs
- **THEN** its `title` appears in the report dialog caption and in the generated header line

---

### Requirement: Document snapshot and string_view lifetime
Before invoking a report function the engine SHALL copy the active document into a contiguous
buffer, and SHALL keep that buffer alive until the function returns. Every `string_view` obtained
from `ReportContext` or from a string helper SHALL point into that buffer.

#### Scenario: Views collected into a container
- **WHEN** a report function stores `string_view` values in a `std::map` or `std::vector` during
  iteration and reads them after the loop
- **THEN** the values remain valid for the remainder of the function

#### Scenario: Document edited during the report
- **WHEN** the document is modified while the progress callback is pumping messages
- **THEN** the report continues against the snapshot taken at invocation and does not observe the
  edit

---

### Requirement: ReportContext document access
`ReportContext` SHALL expose the document through the following members:

| Member | Type | Description |
|---|---|---|
| `Lines()` | range | Yields `{ int lineNo, std::string_view line }` for every line |
| `FindAll(kw)` | range | Yields a `Hit` for every occurrence of one keyword |
| `FindAll({...})` | range | Yields a `Hit` for every occurrence of any listed keyword |
| `text` | `const char*` | Start of the snapshot |
| `length` | `size_t` | Snapshot size in bytes |
| `lineCount` | `int` | Total number of lines |
| `fileName` | `const wchar_t*` | File name of the active document |
| `filePath` | `const wchar_t*` | Full path of the active document |

#### Scenario: Multi-keyword scan cost
- **WHEN** `FindAll` is called with several keywords
- **THEN** the document is traversed once regardless of how many keywords were supplied

---

### Requirement: Hit fields
A `Hit` produced by `FindAll` SHALL expose `keyword`, `lineNo`, `line` and `after`, where `after`
is the remainder of the line beginning immediately past the matched keyword.

#### Scenario: Keyword at end of line
- **WHEN** a keyword matches with nothing following it on that line
- **THEN** `after` is empty and `line` still holds the full line

---

### Requirement: Line iteration semantics
`ctx.Lines()` SHALL yield 1-based line numbers, SHALL exclude the line ending from `line`,
SHALL strip a trailing `\r`, SHALL yield empty lines, and SHALL yield a final line that is not
terminated by a newline.

#### Scenario: First line
- **WHEN** iteration begins
- **THEN** the first yielded `lineNo` is `1`, matching the Notepad++ margin

#### Scenario: CRLF document
- **WHEN** the document uses `\r\n` line endings
- **THEN** no yielded `line` ends with `\r`

#### Scenario: Blank line in the middle
- **WHEN** a document contains an empty line
- **THEN** that line is yielded with an empty `line` value and subsequent line numbers stay
  aligned with the editor

#### Scenario: No trailing newline
- **WHEN** the last line of the document has no line ending
- **THEN** it is still yielded

---

### Requirement: String helpers
`src/ReportApi.h` SHALL provide `After`, `Before`, `Between`, `Field`, `Trim`, `Contains`,
`StartsWith`, `EndsWith`, `ToInt` and `ToDouble`.

#### Scenario: Extraction helper succeeds
- **WHEN** `After(line, "IP: ")` is called on a line containing `IP: 192.168.1.10 connected`
- **THEN** it returns `192.168.1.10 connected`

#### Scenario: Field extraction
- **WHEN** `Field("192.168.1.10 connected", ' ', 0)` is called
- **THEN** it returns `192.168.1.10`

---

### Requirement: Helpers fail to empty and never read out of range
Extraction helpers SHALL return an empty `string_view` when they cannot produce a result, SHALL
return an empty result for empty input, and SHALL NOT read outside the snapshot or throw.
`ToInt` and `ToDouble` SHALL return `bool` and SHALL leave their out-parameter unmodified on
failure.

#### Scenario: Keyword absent
- **WHEN** `After(line, "IP: ")` is called on a line that does not contain `IP: `
- **THEN** it returns an empty `string_view`

#### Scenario: Nested helpers with a failing inner call
- **WHEN** `Field(After(line, "IP: "), ' ', 0)` is evaluated on a line without `IP: `
- **THEN** the result is empty and no out-of-range access occurs

#### Scenario: Field index beyond the available fields
- **WHEN** `Field` is called with an index greater than the number of fields present
- **THEN** it returns an empty `string_view`

#### Scenario: Unterminated delimiter pair
- **WHEN** `Between(line, "[", "]")` is called on a line containing `[` but no following `]`
- **THEN** it returns an empty `string_view`

#### Scenario: Numeric conversion failure
- **WHEN** `ToInt` is called on text that is not a number
- **THEN** it returns `false` and the out-parameter is unchanged

---

### Requirement: ReportBuilder output API
`ReportBuilder` SHALL provide `Section`, `Line`, `KV`, `KVf`, `AtLine` and `Blank`. `KV` SHALL
accept `string_view`, integer and floating-point values.

#### Scenario: Key-value output
- **WHEN** a report calls `out.KV("ERROR", 3)`
- **THEN** the output contains a line pairing the key `ERROR` with the value `3`

#### Scenario: Formatted value
- **WHEN** a report calls `out.KVf(key, "%5d hits", n)`
- **THEN** the value portion is rendered with the supplied printf format

#### Scenario: Line reference output
- **WHEN** a report calls `out.AtLine(142, text)`
- **THEN** the output contains a line identifying line 142 alongside `text`

---

### Requirement: Per-section column alignment
`KV`, `KVf` and `AtLine` entries SHALL be aligned on the key column, and alignment SHALL be
computed independently for each `Section`.

#### Scenario: Two sections with different key widths
- **WHEN** a report emits a section with short keys followed by a section with long keys
- **THEN** each section is aligned to its own widest key and neither affects the other

---

### Requirement: Generated report header
The engine SHALL prepend a header containing the report title, the file name and the document
line count to every report, without the report function emitting it.

#### Scenario: Header present
- **WHEN** any report runs
- **THEN** its output begins with the report title, the active file name and the line count

---

### Requirement: Empty report output
When a report function produces no output the engine SHALL display `(no output)` in the body.

#### Scenario: Report finds nothing
- **WHEN** a report function returns without calling any `ReportBuilder` method
- **THEN** the dialog opens showing the generated header and `(no output)`

---

### Requirement: Report result caching and invalidation
A rendered report SHALL be cached per buffer and reused on subsequent invocations of the same
report until the cache is invalidated. When debug mode is enabled the cache SHALL be bypassed and
the report function SHALL be executed on every invocation.

#### Scenario: Repeated invocation
- **WHEN** the same report is invoked twice with no intervening document edit
- **AND** debug mode is disabled
- **THEN** the second invocation displays the cached result without re-traversing the document

#### Scenario: Invocation after an edit
- **WHEN** the document is edited and the report is invoked again
- **THEN** the document is re-traversed and the report reflects the current content

#### Scenario: Repeated invocation in debug mode
- **WHEN** the same report is invoked twice with no intervening document edit
- **AND** debug mode is enabled
- **THEN** the report function is executed both times
- **AND** debug output is produced on both invocations

---

### Requirement: Progress and cancellation during a report
While a report iterates via `ctx.Lines()` or `ctx.FindAll()` the engine SHALL display the
progress dialog, update it periodically, and pump messages so that Cancel remains clickable.
Cancelling SHALL terminate iteration, discard the output, and open no dialog.

#### Scenario: Progress during a long report
- **WHEN** a report iterates a large document
- **THEN** the progress dialog updates and remains responsive

#### Scenario: User cancels
- **WHEN** the user clicks Cancel during iteration
- **THEN** iteration ends, no report dialog is shown, and the document is unchanged

#### Scenario: Report uses raw text access
- **WHEN** a report reads `ctx.text` directly instead of iterating
- **THEN** the progress dialog shows an indeterminate message and Cancel has no effect

---

### Requirement: Report output encoding
The engine SHALL query `SCI_GETCODEPAGE` and convert rendered report text to UTF-16 using
`CP_UTF8` for code page 65001 and `CP_ACP` otherwise, before displaying it.

#### Scenario: UTF-8 document with non-ASCII content
- **WHEN** a report extracts non-ASCII text from a UTF-8 document
- **THEN** that text renders correctly in the dialog

#### Scenario: ANSI document
- **WHEN** the active document uses code page 0
- **THEN** report text is converted using the system ANSI code page

---

### Requirement: Source encoding for non-ASCII keywords
The project SHALL compile with `/utf-8`, and `config/CustomReports.h` SHALL be saved as UTF-8
with BOM.

#### Scenario: Non-ASCII keyword literal
- **WHEN** a report function searches for a non-ASCII keyword literal
- **THEN** the literal's bytes are UTF-8 and match the corresponding text in a UTF-8 document

---

### Requirement: Shipped example reports
`config/CustomReports.h` SHALL ship with worked examples covering the minimal form, line
iteration with aggregation, and multi-keyword scanning, together with a header comment stating
the lifetime guarantee, the empty-on-failure contract, 1-based line numbering, the absence of a
crash guard with the debugger attach procedure, the UTF-8 BOM requirement, and the
single-inclusion rule.

#### Scenario: New author opens the file
- **WHEN** a user opens `config/CustomReports.h`
- **THEN** they find runnable examples and the documented guarantees without consulting other
  files

---

### Requirement: Report functions are isolated from plugin internals
`src/ReportApi.h` SHALL NOT expose Win32, Scintilla or Notepad++ types, and SHALL NOT require
`config/CustomReports.h` to include any other plugin header.

#### Scenario: Author file includes
- **WHEN** `config/CustomReports.h` is compiled
- **THEN** its only plugin include is `src/ReportApi.h`

---

### Requirement: No crash guard
The engine SHALL invoke report functions directly, without structured exception handling.

#### Scenario: Report function faults
- **WHEN** a report function performs an invalid memory access
- **THEN** the fault is not caught by the plugin
