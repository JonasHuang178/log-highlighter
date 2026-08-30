## Context

`config/LogPatterns.h` and `config/OverviewConfig.h` are the plugin's advertised
customization surface: the README tells users to edit them and rebuild. They are
also the files that ship. Nothing separates "the default we committed to" from
"what a developer was trying out last Tuesday", and both directions of leakage
have now happened:

```
2026-06-14  8745972  LogPatterns.h gains 3 commented-out test keywords
2026-06-14  307e17f  the comments are deleted
2026-06-14  5a2a4f8  spec sync writes them down as "LOG_TYPE_RULES (6 entries)"
                     -> spec now describes code that never existed

2026-07-??  ae8195b  { "Step123", ... } added to STEP_TYPE_RULES,
                     unmentioned in the commit message, spec and README
                     -> code now carries an experiment the spec does not know about
```

`OVERVIEW_BG_COLOR` is the same species in a third form: a knob that was declared
and never wired. The spec's constant list omits it, but the list is worded
`SHALL include`, which permits supersets — so the macro was never technically in
violation, and no reader had a reason to question it.

The common root cause is that **nothing in the specs is mechanically comparable
to the config files**. The one thing the spec did pin exactly — the default rule
tables — is precisely the thing that legitimately varies per user, so nobody
treated a mismatch as meaningful.

## Goals / Non-Goals

**Goals:**

- Bring `config/LogPatterns.h` and `config/OverviewConfig.h` back into agreement
  with `openspec/specs/`.
- Move the spec's grip from rule *content* (volatile, user-owned) to rule
  *structure and invariants* (stable, project-owned), so future drift is
  detectable rather than expected.
- Record the panel-background decision as a requirement instead of a README
  apology.
- Leave a specified, un-enforced rule (STEP prefix redundancy) as the seam a
  later change can make compile-time-enforced.

**Non-Goals:**

- No change to plugin behavior, UI or performance beyond removing one duplicate
  indicator fill.
- No `static_assert` / `constexpr` enforcement of the redundancy rule — see
  Decision 4.
- No revisiting which keywords ship. `[ ERROR ]`, `Step`, `Start test` stay as
  they are; they simply stop being spec-mandated.
- No change to `OVERVIEW_VIEWPORT_BG_COLOR` or any other live constant.

## Decisions

### Decision 1 — Delete `Step123` from the code rather than add it to the spec

Two ways to close this gap: teach the spec that two STEP rules ship, or delete
the rule. Deleting wins because the rule is not merely surplus, it is
**strictly redundant** — it can never produce a match that `Step` does not also
produce, and it duplicates every match it does produce.

Derivation, against the validation in `Parser.cpp::ScanBuffer`:

```
let prefix A, prefix B = A + S

B matches at i  <=>  text[i..] = A + S + D + delim      (D = one or more digits)

then, for A at the same position i:
  char after A is S[0]
    S all digits  -> S[0] is a digit  -> A consumes S + D -> hits delim -> A matches
                     and both extend to end-of-line from i
                     => identical byteOffset, identical length, duplicate Match
    S has a non-digit at [0] -> A fails -> B carries independent value
```

So the redundancy condition is exactly `B = A + zero or more digits`; an empty
`S` degenerates to a duplicated prefix, which the same condition covers.

| Prefix pair | Redundant? |
|---|---|
| `Step` / `Step123` | yes — two `Match` entries over the same range |
| `Step` / `Step` | yes |
| `Step` / `Stepabc` | no — `Stepabc1 ` matches only the longer prefix |
| `Step` / `Phase` | no |

Aho-Corasick does not collapse the pair on its own: `Step123`'s failure state has
no `Step` in its output chain, so both patterns report independently at different
scan positions, and both survive validation.

*Alternatives considered.* Keeping the rule and documenting the duplication was
rejected: it costs a redundant `SCI_INDICATORFILLRANGE` per match and a duplicate
`PanelMark`, and it teaches by example that overlapping prefixes are fine.

### Decision 2 — Delete `OVERVIEW_BG_COLOR` instead of wiring it up

The alternative was to make `DrawPanel` honor the constant, following the
`CLR_NONE`-sentinel pattern that `OVERVIEW_VIEWPORT_BG_COLOR` already establishes
a few lines away in the same file. That path is cheap, and it was rejected on two
grounds.

First, a trap: the constant's current value is `RGB(60, 60, 60)`. Wiring it up
as-is would turn every rebuilt panel dark grey against an unchanged scrollbar — a
visible regression delivered by a change whose entire premise is that nothing
user-visible changes. Wiring it up correctly would therefore also require
flipping the default to `CLR_NONE`, i.e. shipping a knob whose only tested value
is "don't use the knob".

Second, and decisive: the panel background matching `COLOR_BTNFACE` is a
*decision*, not an omission. The strip is carved out of Scintilla's non-client
area immediately right of the scrollbar; the two are meant to read as one
surface. A configuration point invites users to break that alignment. A dead knob
is worse than no knob, and a live knob is worse than the current behavior.

So the constant goes, and the position it was silently encoding is promoted from
a README disclaimer to a requirement under `overview-panel`.

### Decision 3 — Specs constrain rule *structure*, not rule *content*

The `Default rules` and `Default bookmark rules` requirements currently pin the
exact shipped keyword tables. This is the wrong grip on the wrong thing: the
README's headline feature is that users edit these tables, so any user who adds
`[ INFO ]` immediately violates the spec. A requirement that everyone is expected
to break is a requirement nobody checks — which is how a phantom 6-entry table
survived two months.

Note what the existing scenarios under those requirements actually test:

```
Requirement: Default bookmark rules
  |- BOOKMARK_RULES (1 entry) table            <- content, drifts
  |- Scenario: "Start test" matched            <- really tests matching semantics
  '- Scenario: casing "start test" no match    <- really tests case sensitivity
```

Both scenarios are worth keeping and neither actually depends on the keyword
being `Start test`. Re-attaching them to the matching-semantics requirement
preserves all their value while dropping the brittleness.

What replaces the tables are the properties that would genuinely be bugs if
violated:

| Invariant | Why it matters |
|---|---|
| Each of the three tables is non-empty | The shipped config demonstrates all three rule types |
| At least one rule has `showInPanel = true` | The first Ctrl+Alt+Q visibly exercises the Overview Panel |
| No redundant STEP prefix (Decision 1's condition) | The exact defect this change removes |

The specific shipped keywords survive as a non-normative note pointing at the
README, which is already the authoritative, worked-through description of them.

*Alternative considered.* Keeping the tables but labelling them "illustrative,
not normative" was rejected as a half-measure: the text still drifts, and a
reader still cannot tell whether a mismatch is a defect.

### Decision 4 — Specify the redundancy rule now, enforce it mechanically later

The redundancy prohibition is written into the spec by this change, but stays
enforced by review. A `constexpr` predicate over `STEP_TYPE_RULES` plus a
`static_assert` is entirely feasible in C++17 and would make the next `Step99`
fail the build instead of shipping.

It is deferred because it is a *new safety mechanism*, not an alignment fix, and
folding it in would turn a change whose thesis is "make specs and code agree"
into two changes wearing one name. The rule is specified here precisely so the
follow-up has something to implement against.

The honest counter-argument is recorded: `Step123` is direct evidence that a
prose requirement in this project does not survive contact with a config file
edit. Decision 4 accepts that risk for one change's duration.

### Decision 5 — Constant lists become exact sets, and that is what justifies Decision 2

`SHALL include` permits supersets. Under that wording `OVERVIEW_BG_COLOR` is
*not* a violation, which is the mechanical reason it survived review. Changing
the wording to an exact, complete set changes the causal chain:

```
   "exactly these constants, no others"
                |
                v
   OVERVIEW_BG_COLOR becomes a spec violation
                |
                v
   deleting it is a correction, not a preference
```

This also gives the file the property it was missing: an inventory a reader can
diff against the header in ten seconds.

The same constraint is applied to `LogPatterns.h`, but at the structural layer
only — `MAKE_BGR`, the three rule structs, the three rule tables, exactly those.
Together with Decision 3 the two layers separate cleanly:

```
LogPatterns.h
  structural layer  (exact set, project-owned)  MAKE_BGR + 3 structs + 3 tables
  content layer     (free, user-owned)          keywords, colors, showInPanel

OverviewConfig.h
  structural layer only (exact set)             7 constants
```

## Risks / Trade-offs

**[Removing a STEP rule shifts indicator and pattern index bases]** →
`INDIC_BOOKMARK_BASE` moves 16 → 15 and `BOOKMARK_PATTERN_BASE` shifts likewise.
Both are `constexpr` expressions derived from `sizeof(...)/sizeof(...[0])` in
`log-highlighter.cpp` and `Parser.cpp`; no index is hard-coded anywhere. Verified
by inspection that the only literal in the chain is the base `11`. No source file
under `src/` needs to change.

**[Users who deliberately added their own redundant prefix]** → The new
requirement makes their config non-conforming without warning them. Accepted: the
config is rebuilt from source by each user, the condition is easy to state, and
the behavior of a redundant prefix (silent duplicate fills) was never desirable.
No migration is possible or needed for a header-only, compile-time configuration.

**[Dropping the default tables loses the record of what ships]** → Mitigated by
the non-normative note and by the README, which documents every shipped rule with
worked examples. The trade is deliberate: a description that is allowed to be
approximate and is kept in one place beats a requirement that is mandatory,
duplicated, and wrong.

**[Deferring `static_assert` leaves the redundancy rule unenforced]** → Accepted
for this change; recorded in the proposal's Deferred section so the follow-up is
not lost to the git log. Decision 4 states the counter-argument.

**[Exact-set constraints add friction to adding a constant]** → Intended. Adding
a knob now requires touching the spec, which is the mechanism that would have
caught `OVERVIEW_BG_COLOR` at birth.

## Migration Plan

None required. Both edits remove entries from compile-time configuration headers
in a plugin that users build from source. Rollback is a `git revert`; there is no
persisted state, no on-disk format and no API surface involved.

## Open Questions

None outstanding. The one live question during exploration — whether to include
compile-time enforcement of the redundancy rule — was resolved as deferred
(Decision 4).
