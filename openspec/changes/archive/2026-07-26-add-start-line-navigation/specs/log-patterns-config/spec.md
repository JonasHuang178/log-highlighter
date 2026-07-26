## ADDED Requirements

### Requirement: BookmarkRule struct
`BookmarkRule` in `config/LogPatterns.h` SHALL have three fields:

| Field | Type | Description |
|---|---|---|
| `keyword` | `const char*` | Exact UTF-8 string to match (case-sensitive) |
| `textColor` | `COLORREF` | Foreground color applied to the matched keyword, expressed as `MAKE_BGR(r,g,b)` |
| `showInPanel` | `bool` | `true` = show a colored tick mark in the Overview Panel |

#### Scenario: BookmarkRule matched
- **WHEN** a line contains the exact string defined in a `BookmarkRule` keyword
- **THEN** the keyword text is highlighted with the specified foreground color
- **AND** if `showInPanel` is `true`, a colored mark appears in the Overview Panel

### Requirement: Default bookmark rules
The default `LogPatterns.h` SHALL ship with these bookmark rules:

**BOOKMARK_RULES** (1 entry):

| Keyword | Color | showInPanel |
|---|---|---|
| `Start test` | Magenta `MAKE_BGR(200,0,180)` | `true` |

#### Scenario: Start test keyword matched
- **WHEN** a line contains the exact string `Start test`
- **THEN** the keyword text is highlighted with magenta foreground color
- **AND** a colored mark appears in the Overview Panel

#### Scenario: Start test case sensitivity
- **WHEN** a line contains `start test` or `START TEST` (different casing)
- **THEN** no match is recorded (matching is case-sensitive)
