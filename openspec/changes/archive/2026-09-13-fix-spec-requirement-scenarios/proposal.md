## Why

`openspec validate --specs` reports 2 failures. `spec-document-structure`
requires zero, so the specs directory is knowingly non-conforming.

This is the follow-up that `fix-openspec-spec-headers` named and deferred. That
change repaired structure — delta headers and missing Purpose sections — and
recorded why it stopped short of the requirement bodies:

> Fixing these means writing new requirement text and inventing scenarios […]
> That is authoring specification content, not repairing structure, and it
> cannot be done mechanically: a scenario asserts intended behavior, so it would
> be inferred from the implementation — the same direction of inference that
> produced the phantom rule table `fix-spec-config-drift` had to remove.

Its deferred list covered three specs. `log-patterns-config` has since been
repaired: archiving `fix-spec-config-drift` was blocked by its scenario-less
`MAKE_BGR macro` requirement, which was fixed there because the archive could
not proceed otherwise. Two remain.

| Spec | Requirement | Missing |
|---|---|---|
| `scanner` | `Match struct` | SHALL, scenario |
| `scanner` | `Progress callback` | SHALL, scenario |
| `status-bar-timing` | `Parse time displayed in Notepad++ status bar` | scenario |
| `status-bar-timing` | `Timing scope` | SHALL |
| `status-bar-timing` | `Adaptive time format` | SHALL |
| `status-bar-timing` | `Timing source` | scenario |

The common cause is mood. These six were written to *describe what the code
does* — "Each match produces one `Match` entry", "`t0` is captured immediately
after `InitStyles`", "The format depends on the magnitude of `elapsed`" — rather
than to state what the system is obliged to do. A description has nothing to
test against, which is why the validator rejects it.

Unlike the delta-header defect, this one is not silent: validation has reported
it on every run since the structural repair landed. It has simply been carried.

## What Changes

Six requirements are rewritten in the normative mood and given scenarios.

**How much of this is genuinely new content.** Decision 6 of the prior change
worried that scenarios would be reverse-engineered from the implementation. In
the event, almost nothing had to be invented: five of the six requirements
already stated their obligations in prose and were merely failing to say SHALL,
or stated them normatively and were merely missing a scenario. The scenarios
assert what the existing text already claims.

Two additions go beyond restating, and are called out for review:

- **`Match struct` gains `BOOKMARK`.** The field table lists `type` as
  `LOG_TYPE` or `STEP_TYPE` and `ruleIndex` as indexing one of two rule tables.
  `MatchType::BOOKMARK` was added to `src/Parser.h` in `ae8195b` and the
  requirement was never updated. This is a factual correction, not an inference:
  the bookmark rule type is independently specified in `log-patterns-config` and
  `bookmark-navigation`, so the requirement is being brought in line with
  specifications that already exist, not with the code alone. The stale comment
  in `src/Parser.h` is corrected to match.
- **The sort-order guarantee gains its consumer.** `Match struct` already
  required ascending `byteOffset` order. The rewrite records *who depends on it*
  — `BookmarkLinesFrom` deduplicates by comparing against the previous entry
  only, which is correct only for a sorted vector. This turns an incidental
  property of left-to-right scanning into a guarantee with a stated reason to
  hold.

**Two obligations are made explicit that the prose only implied:**

- A cancelled scan returns an empty vector *and therefore discards matches it had
  already collected*. The old text said "returns an empty vector"; the rewrite
  says why that matters — a caller must not be able to mistake a partial result
  for a complete one.
- The timing interval must not stop at the end of the scan. Phase 2 dominates on
  a match-dense document, so a figure excluding it would understate the cost of
  the command the user invoked.

Neither changes behavior; both state the intent the current code already
implements.

## Capabilities

### New Capabilities

None.

### Modified Capabilities

- `scanner`: `Match struct` and `Progress callback` rewritten normatively, given
  scenarios, and corrected to include the `BOOKMARK` match type.
- `status-bar-timing`: all four requirements rewritten normatively and/or given
  scenarios.

No requirement is added or removed. No obligation is weakened.

## Impact

**Specs**

- `openspec/specs/scanner/spec.md`
- `openspec/specs/status-bar-timing/spec.md`

Applied by `openspec archive`, not by hand. Unlike `fix-openspec-spec-headers`,
this change repairs requirement *content* belonging to two capabilities, so it
has a normal capability delta and needs no direct write into `openspec/specs/`.

**Code**

- `log-highlighter/src/Parser.h` — one comment corrected to list `BOOKMARK`
  alongside the other two match types. No executable line changes; the enum
  itself already has all three members.

**Outcome**

`openspec validate --specs` reaches 12 passed / 0 failed, satisfying
`spec-document-structure` for the first time.

**Risk**

Low, but not zero in the way the other spec repairs were. Those changed framing
only; this one asserts behavior. If a scenario here is wrong, it is wrong in the
direction of claiming the plugin guarantees something it does not — which is
why the two additions and two made-explicit obligations above are listed
individually rather than folded into a diff.
