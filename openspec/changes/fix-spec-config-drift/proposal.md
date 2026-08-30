## Why

Three places where the specs and the shipped configuration files disagree, all
traceable to the same failure mode: `config/` serves both as the shipped default
and as the user's own experiment scratchpad, with no boundary between the two.
Experiment residue leaked into the spec in one direction and stayed in the code
in the other.

- `specs/log-patterns-config` records `LOG_TYPE_RULES` as shipping **6 entries**.
  Three of those (`[  ERROR  ]`, `[ WARNING ]`, `[  MSG    ]`) were only ever
  **commented-out** test keywords, deleted in `307e17f`; the Phase 3 spec sync
  (`5a2a4f8`) transcribed them as live defaults. The code ships 3.
- `STEP_TYPE_RULES` gained a second entry, `Step123`, in `ae8195b` with no
  mention in any commit message, spec or README. It is strictly redundant and
  produces a duplicate `Match` for every text it matches. The spec still says 1.
- `OVERVIEW_BG_COLOR` is defined in `config/OverviewConfig.h` and read by
  nothing. The spec's constant list does not mention it, but the list is worded
  `SHALL include`, which permits supersets — so the dead macro is not currently
  a violation, which is exactly why it survived.

Left alone, the specs keep drifting because nothing in them is checkable against
the config files.

## What Changes

**Config corrections**

- Remove `{ "Step123", ... }` from `STEP_TYPE_RULES` in `config/LogPatterns.h`.
  It is redundant: a `STEP` prefix `B` equal to another prefix `A` followed only
  by digits can never match text that `A` does not also match, so it only ever
  duplicates `A`'s match over an identical byte range.
- Remove `OVERVIEW_BG_COLOR` from `config/OverviewConfig.h`.

**Spec restructuring — separate normative semantics from illustrative content**

- Replace the pinned default rule tables in `log-patterns-config` with the
  invariants that actually matter (each table non-empty; at least one rule with
  `showInPanel = true`; no redundant `STEP` prefix). The specific shipped
  keywords become a non-normative note pointing at the README.
  This stops an ordinary user customization — adding `[ INFO ]` — from reading
  as a spec violation, which is why nobody compared spec to config for two
  months.
- Preserve the case-sensitivity scenario currently attached to the bookmark
  default table by re-attaching it to the matching-semantics requirement, where
  it does not depend on the keyword happening to be `Start test`.
- Add the `STEP` prefix redundancy prohibition as a requirement. Enforcement
  stays manual in this change — see Deferred.

**Spec inventory constraints**

- Change the `OverviewConfig.h` constant list from `SHALL include` to an exact,
  complete set. This is what makes removing `OVERVIEW_BG_COLOR` a correction
  rather than a housekeeping preference, and makes the next stray macro
  detectable.
- Apply the same exact-set constraint to `LogPatterns.h` at the **structural**
  level only (`MAKE_BGR`, three rule structs, three rule tables). Rule
  *contents* stay the user's territory.

**Design position recorded**

- The Overview Panel background uses the system color `COLOR_BTNFACE` so the
  strip matches the adjacent scrollbar. Today this is an apology in the README
  ("nothing reads it"); it becomes a requirement in `overview-panel`.

**Deferred (not in this change)**

- A `constexpr` / `static_assert` check in `Parser.cpp` enforcing the `STEP`
  prefix redundancy rule at compile time. The rule itself is specified here; only
  machine enforcement is deferred, because adding a new safety mechanism is a
  separate concern from aligning specs with code. `Step123` is the evidence that
  prose alone does not hold.

No behavior visible to users changes. Removing `Step123` means one fewer
indicator fill over an identical range in an identical color.

## Capabilities

### New Capabilities

None.

### Modified Capabilities

- `log-patterns-config`: default rule tables cease to be normative content and
  are replaced by invariants; a `STEP` prefix redundancy prohibition is added;
  the `OverviewConfig.h` constant list becomes an exact set; `LogPatterns.h`
  gains a structural exact-set constraint.
- `overview-panel`: adds a requirement fixing the panel background to
  `COLOR_BTNFACE` and stating that it is not configurable.

## Impact

**Code**

- `log-highlighter/config/LogPatterns.h` — one rule row removed.
- `log-highlighter/config/OverviewConfig.h` — one macro definition removed.
- No `.cpp` or `.h` under `src/` changes. All rule counts and index bases
  (`STEP_RULE_COUNT`, `INDIC_BOOKMARK_BASE`, `BOOKMARK_PATTERN_BASE`) are derived
  at compile time via `sizeof`, so removing a rule shifts the later ranges
  automatically.

**Docs**

- `README.md` — remove the `Step123` row from the Step Type table and the
  blockquote below it; remove the `OVERVIEW_BG_COLOR` paragraph.

**Specs**

- `openspec/specs/log-patterns-config/spec.md`
- `openspec/specs/overview-panel/spec.md`

**Risk**

Low. Both config edits remove rules rather than adding them; no new behavior is
introduced. Indicator indices shift down by one for `BOOKMARK_RULES` (16 → 15),
which is derived and unreferenced elsewhere.
