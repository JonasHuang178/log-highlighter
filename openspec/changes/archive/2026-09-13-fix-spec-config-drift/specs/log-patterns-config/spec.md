## ADDED Requirements

### Requirement: Shipped rule table invariants
The rule tables shipped in `config/LogPatterns.h` SHALL satisfy the invariants
below, so that a freshly built plugin demonstrates every rule type and visibly
exercises the Overview Panel on first use. The entries themselves are user-owned
content and are NOT normatively fixed.

1. `LOG_TYPE_RULES[]`, `STEP_TYPE_RULES[]` and `BOOKMARK_RULES[]` SHALL each
   contain at least one entry.
2. At least one entry across all three tables SHALL have `showInPanel = true`.

The specific keywords, colors and `showInPanel` values that ship are illustrative
rather than normative. They are documented in `README.md`; adding, removing or
editing them is the intended use of the file and SHALL NOT be treated as a
violation of this specification.

#### Scenario: Shipped configuration demonstrates every rule type
- **WHEN** the plugin is built from the committed `config/LogPatterns.h`
- **THEN** each of the three rule tables contains at least one entry

#### Scenario: Shipped configuration exercises the Overview Panel
- **WHEN** the plugin is built from the committed `config/LogPatterns.h`
- **AND** the user presses Ctrl+Alt+Q on a document containing a matching keyword
- **THEN** at least one rule has `showInPanel = true`, so at least one mark can appear in the Overview Panel

#### Scenario: User adds a keyword
- **WHEN** a user adds a new entry such as `{ "[ INFO ]", MAKE_BGR(0,180,0), false }` to `LOG_TYPE_RULES[]`
- **THEN** the configuration remains conforming, because table contents are not normatively fixed

#### Scenario: User removes a shipped keyword
- **WHEN** a user removes a shipped entry and at least one entry remains in each table
- **THEN** the configuration remains conforming

---

### Requirement: No redundant STEP_TYPE prefix
`STEP_TYPE_RULES[]` SHALL NOT contain a prefix `B` that equals another prefix `A`
followed by zero or more ASCII digits.

Such a `B` is strictly redundant. Given the STEP_TYPE match rule
(`<prefix>` + one or more digits + space or end-of-line), any text matching `B`
has the form `A` + digits + digits + delimiter, which `A` also matches, producing
a `Match` with an identical `byteOffset` and an identical `length` — both ranges
extend from the prefix start to the end of the line. The scanner therefore emits
two identical matches, and the renderer performs two `SCI_INDICATORFILLRANGE`
calls over the same bytes.

A longer prefix whose extra characters are not all digits is NOT redundant and is
permitted: `Stepabc` matches `Stepabc1 `, where `Step` fails because the character
following it is not a digit.

Aho-Corasick does not collapse a redundant pair on its own: the longer prefix's
state does not carry the shorter pattern in its output chain, so both patterns
report independently and both pass validation.

Enforcement is by review in this revision. Compile-time enforcement is out of
scope here.

#### Scenario: Redundant prefix rejected
- **WHEN** `STEP_TYPE_RULES[]` contains both `"Step"` and `"Step123"`
- **THEN** the configuration is non-conforming, because `Step123` is `Step` followed only by digits

#### Scenario: Duplicated prefix rejected
- **WHEN** `STEP_TYPE_RULES[]` contains `"Step"` twice
- **THEN** the configuration is non-conforming, because the second entry is the first followed by zero digits

#### Scenario: Longer prefix with non-digit suffix permitted
- **WHEN** `STEP_TYPE_RULES[]` contains both `"Step"` and `"Stepabc"`
- **THEN** the configuration is conforming, because `Stepabc1 ` matches only the longer prefix

#### Scenario: Unrelated prefixes permitted
- **WHEN** `STEP_TYPE_RULES[]` contains both `"Step"` and `"Phase"`
- **THEN** the configuration is conforming

---

### Requirement: LogPatterns.h structural inventory
`config/LogPatterns.h` SHALL define exactly the following, and nothing else:

- the `MAKE_BGR(r, g, b)` macro
- the struct types `LogTypeRule`, `StepTypeRule` and `BookmarkRule`
- the rule tables `LOG_TYPE_RULES[]`, `STEP_TYPE_RULES[]` and `BOOKMARK_RULES[]`

This constrains the file's structure only. The number and content of entries
within each table are governed by "Shipped rule table invariants" and are
deliberately unconstrained.

Introducing a new struct type, table or macro SHALL require updating this
specification in the same change.

#### Scenario: File matches the declared inventory
- **WHEN** `config/LogPatterns.h` is compared against this list
- **THEN** every item in the list is present and no additional macro, struct type or rule table is defined

#### Scenario: Undeclared definition added
- **WHEN** a macro, struct type or rule table not named in this list is added to `config/LogPatterns.h`
- **THEN** the configuration is non-conforming until this specification is updated to include it

## MODIFIED Requirements

### Requirement: Rebuild-only customization
Adding, removing, or editing rules SHALL require only editing `config/LogPatterns.h` and rebuilding. Overview Panel appearance SHALL require only editing `config/OverviewConfig.h` and rebuilding. No other source files need modification. The scanner and renderer derive all rule counts at compile time via `sizeof` array division.

`OverviewConfig.h` SHALL define exactly the following constants, and no others:
- `OVERVIEW_PANEL_WIDTH` — panel strip width in pixels
- `OVERVIEW_MARK_MIN_H` — minimum mark height in pixels
- `OVERVIEW_SNAP_RADIUS` — click snap radius in document lines
- `OVERVIEW_VIEWPORT_COLOR` — viewport box border color
- `OVERVIEW_VIEWPORT_BG_COLOR` — viewport box fill color
- `OVERVIEW_VIEWPORT_BORDER_WIDTH` — viewport box border pen width (default: 1)
- `OVERVIEW_VIEWPORT_BORDER_VISIBLE` — viewport box visibility toggle (default: true); when false, both fill and border are hidden

This list is the complete inventory, not a minimum. Every constant defined in the
file SHALL be read by the implementation; a constant that nothing reads is a
defect, because it presents itself as a working setting and silently does
nothing. Introducing a new constant SHALL require updating this list in the same
change.

#### Scenario: Adding a rule requires no source change
- **WHEN** a user adds an entry to any rule table in `config/LogPatterns.h` and rebuilds
- **THEN** the new rule is scanned and rendered with no modification to any file under `src/`

#### Scenario: File matches the declared constant inventory
- **WHEN** `config/OverviewConfig.h` is compared against this list
- **THEN** every listed constant is present and no additional constant is defined

#### Scenario: Constant defined but never read
- **WHEN** a constant is defined in `config/OverviewConfig.h` that no implementation file reads
- **THEN** the configuration is non-conforming, and the constant is either wired up or removed

---

### Requirement: BookmarkRule struct
`BookmarkRule` in `config/LogPatterns.h` SHALL have three fields:

| Field | Type | Description |
|---|---|---|
| `keyword` | `const char*` | Exact UTF-8 string to match (case-sensitive) |
| `textColor` | `COLORREF` | Foreground color applied to the matched keyword, expressed as `MAKE_BGR(r,g,b)` |
| `showInPanel` | `bool` | `true` = show a colored tick mark in the Overview Panel |

Matching is exact and case-sensitive, and applies to any keyword configured in
`BOOKMARK_RULES[]` — these semantics do not depend on which keywords ship.

#### Scenario: BookmarkRule matched
- **WHEN** a line contains the exact string defined in a `BookmarkRule` keyword
- **THEN** the keyword text is highlighted with the specified foreground color
- **AND** if `showInPanel` is `true`, a colored mark appears in the Overview Panel

#### Scenario: Bookmark matching is case-sensitive
- **WHEN** a `BookmarkRule` keyword is configured
- **AND** a line contains the same text in different casing
- **THEN** no match is recorded

#### Scenario: Bookmark keyword is a jump target
- **WHEN** a line matches any entry in `BOOKMARK_RULES[]`
- **THEN** that line is a Ctrl+Alt+W navigation target

## REMOVED Requirements

### Requirement: Default rules
**Reason**: The requirement pinned the exact contents of `LOG_TYPE_RULES[]` and
`STEP_TYPE_RULES[]`, which is the wrong thing to fix normatively — editing those
tables is the file's advertised purpose, so any user customization made the
configuration non-conforming. Because everyone was expected to violate it, nobody
compared it against the code, and it drifted twice without detection: it recorded
`LOG_TYPE_RULES` as shipping 6 entries, three of which were only ever
commented-out test keywords deleted before the requirement was written, while the
code shipped 3; and it recorded 1 STEP entry after a second, redundant `Step123`
entry had been added to the code.

**Migration**: Replaced by "Shipped rule table invariants", which constrains the
properties that would genuinely be defects (each table non-empty, at least one
rule visible in the panel) and defers the specific shipped keywords to `README.md`
as non-normative documentation. The redundancy defect the old requirement failed
to catch is now covered by "No redundant STEP_TYPE prefix".

---

### Requirement: Default bookmark rules
**Reason**: Same defect as "Default rules" — it pinned the contents of
`BOOKMARK_RULES[]`. Its two scenarios were nominally about the shipped
`Start test` keyword but actually tested matching semantics (exact match, case
sensitivity), which hold for any configured keyword.

**Migration**: The table is dropped; the non-empty invariant is covered by
"Shipped rule table invariants". Both scenarios are preserved, generalized away
from the `Start test` keyword, under the "BookmarkRule struct" requirement.
