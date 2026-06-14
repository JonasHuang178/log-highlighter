## Requirements

### Requirement: LogTypeRule struct
`LogTypeRule` in `config/LogPatterns.h` SHALL have three fields:

| Field | Type | Description |
|---|---|---|
| `keyword` | `const char*` | Exact UTF-8 string to match (case-sensitive) |
| `textColor` | `COLORREF` | Foreground color applied to the matched keyword, expressed as `MAKE_BGR(r,g,b)` |
| `showInPanel` | `bool` | `true` = show a colored tick mark in the Overview Panel |

#### Scenario: showInPanel false
- **WHEN** a `LogTypeRule` has `showInPanel = false`
- **THEN** matches for that rule produce no mark in the Overview Panel

#### Scenario: showInPanel true
- **WHEN** a `LogTypeRule` has `showInPanel = true`
- **THEN** every match for that rule produces a colored tick mark in the Overview Panel

---

### Requirement: StepTypeRule struct
`StepTypeRule` in `config/LogPatterns.h` SHALL have three fields:

| Field | Type | Description |
|---|---|---|
| `prefix` | `const char*` | Literal text before the digit sequence, e.g. `"Step"` |
| `bgColor` | `COLORREF` | Background color applied from prefix start to end-of-line, expressed as `MAKE_BGR(r,g,b)` |
| `showInPanel` | `bool` | `true` = show a colored tick mark in the Overview Panel |

Match rule: `<prefix>` + one or more ASCII digits + (space character OR end-of-line).
The highlighted range runs from the first character of the prefix to the last character of the line (excluding the newline).

Invalid: `Step ` (no digits), `Stepname` (letter after prefix), `Step1init` (non-space/EOL after digits).

#### Scenario: Step not matched on non-digit suffix
- **WHEN** the text is `Step1init`
- **THEN** no STEP_TYPE match is recorded

---

### Requirement: Default rules
The default `LogPatterns.h` SHALL ship with these rules:

**LOG_TYPE_RULES** (6 entries):

| Keyword | Color | showInPanel |
|---|---|---|
| `[ ERROR ]` | Red `MAKE_BGR(220,0,0)` | `true` |
| `[ WARN ]` | Golden yellow `MAKE_BGR(200,160,0)` | `false` |
| `[ DEBUG ]` | Cornflower blue `MAKE_BGR(70,150,255)` | `false` |
| `[  ERROR  ]` | Red `MAKE_BGR(220,0,0)` | `true` |
| `[ WARNING ]` | Golden yellow `MAKE_BGR(200,160,0)` | `false` |
| `[  MSG    ]` | Cornflower blue `MAKE_BGR(70,150,255)` | `false` |

**STEP_TYPE_RULES** (1 entry):

| Prefix | Color | showInPanel |
|---|---|---|
| `Step` | Light green `MAKE_BGR(180,230,180)` | `false` |

---

### Requirement: MAKE_BGR macro
`MAKE_BGR(r, g, b)` SHALL convert a standard RGB triple to Scintilla's BGR `COLORREF` format:
```cpp
#define MAKE_BGR(r,g,b) ( ((COLORREF)(b) << 16) | ((COLORREF)(g) << 8) | (COLORREF)(r) )
```

---

### Requirement: Rebuild-only customization
Adding, removing, or editing rules SHALL require only editing `config/LogPatterns.h` and rebuilding. No other source files need modification. The scanner and renderer derive all rule counts at compile time via `sizeof` array division.
