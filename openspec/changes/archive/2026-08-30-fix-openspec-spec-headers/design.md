## Context

`openspec/specs/` is the project's record of what the plugin is required to do.
It has never been in a state the tooling accepts:

```
openspec validate --specs  ->  0 passed, 11 failed
```

The two defects have different characters and only one of them has teeth.

**The delta headers are a live defect.** Three files open with
`## ADDED Requirements`, which is valid only inside a change's `specs/`
directory. In a main spec it truncates the parsed requirements section, so every
requirement below it is invisible to `validate`, `list` and `archive`. Nine
requirements in `overview-panel` are affected, plus everything in
`progress-dialog` and `snap-to-nearest-mark`.

Each was written by a past archive run:

```
b8fd849  chore: archive feat-viewport-box-config, sync specs  -> overview-panel
68925d5  chore: archive progress-dialog-two-phase             -> progress-dialog
5341d11  chore: archive snap-to-nearest-mark                  -> snap-to-nearest-mark
```

The pattern is consistent: when a change's delta file consisted solely of
`## ADDED Requirements`, the sync carried that header into the main spec instead
of normalizing it. Every affected file traces back to an archive of a
purely-additive change.

**The missing Purpose sections are an omission**, uniform across all eleven
files, with no runtime or tooling consequence beyond the failing validation.

This surfaced because `fix-spec-config-drift` tried to archive an
`overview-panel` delta into a structurally invalid target. `openspec archive`
refused and changed nothing — the guard worked exactly as intended, and that
change is blocked until this one lands.

## Goals / Non-Goals

**Goals:**

- Make every requirement already in `openspec/specs/` visible to the tooling.
- Take `openspec validate --specs` from 0 passed / 11 failed to 8 passed /
  3 failed, with every remaining failure being a content defect this change
  deliberately does not touch (Decision 6).
- Leave a record of how the bad headers were produced, so the pattern is
  recognizable if it recurs.

**Non-Goals:**

- No requirement is added, removed, reworded or reinterpreted. Not one.
- No change to anything under `log-highlighter/`. No build, no plugin behavior.
- No attempt to fix the archive tooling itself — the defect is in this repo's
  data, and the tool is a dependency this project does not own.
- No review of whether the newly visible requirements are *correct*. They become
  visible; auditing their content is separate work.

## Decisions

### Decision 1 — Write directly into `openspec/specs/`, and archive with `--skip-specs`

Normally only `openspec archive` writes to `openspec/specs/`, and a change
expresses its intent as a delta under `changes/<name>/specs/`. That mechanism
cannot carry this repair, for a structural reason: a delta describes *requirement
changes*, and this change alters no requirement. There is no `ADDED`, `MODIFIED`
or `REMOVED` block that means "line 1 of this file has the wrong header".

Worse, the delta mechanism is precisely what is broken. Routing a fix for
malformed main specs through the machinery that malformed them would mean asking
`archive` to write a valid header into a file it cannot currently parse.

So the repair to the eleven existing files is applied by editing them directly.
Archiving still runs normally rather than with `--skip-specs`, because the one
genuine delta this change does carry — the new `spec-document-structure`
capability — has to be written into `openspec/specs/` like any other. Decision 5
covers what that write is expected to produce.

*Alternative considered.* Reconstructing each affected spec as a full `MODIFIED`
delta containing every requirement verbatim would let archive rewrite the files
through the normal path. Rejected: it would restate hundreds of lines of
requirement text purely to change three characters, and any transcription slip
would silently alter a requirement — the exact failure mode this change exists to
stop.

### Decision 2 — Fix only line 1, and confirm by measurement that nothing else was hiding

The temptation with a file whose contents were invisible to validation is to
review everything in it while you are there. That is a different change.

The bounded question is whether the headers are the *only* structural problem, or
merely the first one that validation could reach. Measured, not assumed: with the
three headers corrected and no other edit, validation reports exactly one issue
per spec — the missing `Purpose` — uniformly across all eleven files.

```
before:  0 passed, 11 failed   3 files with unreachable requirements
                               11 files missing Purpose

after header fix only:
         0 passed, 11 failed   0 files with unreachable requirements
                               11 files missing Purpose   <- the only remaining issue
```

The previously hidden requirements are structurally clean. They were unreachable,
not malformed. So the header fix stays a one-line edit per file, and their
*content* remains unaudited on purpose — see Non-Goals.

A sweep confirmed no delta header appears anywhere other than line 1 of those
three files.

### Decision 3 — Purpose text describes the capability, not this change

Eleven Purpose sections have to be written. They are new prose, and prose in a
spec invites scope creep: a Purpose that starts explaining *how* something works
becomes an unversioned second copy of the requirements, drifting from them
exactly the way the default rule tables drifted in `fix-spec-config-drift`.

Each Purpose is therefore held to one or two sentences answering only "what does
this capability cover, and why does it exist as a separate capability". No
mechanism, no constants, no keyword lists — those live in the requirements below
it, and in the README.

### Decision 4 — Land this before `fix-spec-config-drift`

`fix-spec-config-drift` is committed and pushed on its own branch
(`ed47791`, `8426751`) and is otherwise complete. Two possible orders:

| Order | Consequence |
|---|---|
| This change first (chosen) | `fix-spec-config-drift` archives normally through the documented path |
| That change first, with `--skip-specs` | Its deltas would be silently dropped, leaving the drift it exists to fix still in the specs |

The second is not a real option — it would archive a change while discarding its
entire output. This one goes first.

### Decision 5 — Expect the archive of this change to reproduce the defect, and handle it

`spec-document-structure` is a new capability, so its delta consists solely of
`## ADDED Requirements`. That is precisely the input shape that produced all three
malformed specs:

```
b8fd849 / 68925d5 / 5341d11
   purely-additive delta   --archive-->   main spec starting "## ADDED Requirements"
```

There is therefore a well-founded expectation that archiving this change writes
`openspec/specs/spec-document-structure/spec.md` with a delta header on line 1
and no `## Purpose` — a file that violates, on the day it is created, the two
requirements written inside it.

This is treated as an expected outcome rather than an embarrassment to avoid, for
two reasons. First, avoiding it would mean not declaring the capability at all,
which leaves the recurrence undetectable — the thing the capability exists to
prevent. Second, it is the requirement working: "run `openspec validate --specs`
after every archive" catches this within seconds of it happening, which is exactly
the loop that was missing while three malformed specs accumulated over two months.

The task list therefore treats post-archive validation as mandatory, and repairing
the generated file as an anticipated step rather than a failure. If archive turns
out to normalize the header, the validation step simply passes and nothing is
done.

*Alternative considered.* Dropping the capability and satisfying the schema's
`specs` artifact with an empty or placeholder file. Rejected: it would state
something untrue about the change, and it would trade a durable rule for a
one-time cleanup that silently re-breaks on the next purely-additive archive.

**Outcome — the prediction was wrong.** Archiving this change wrote
`openspec/specs/spec-document-structure/spec.md` with a correct `## Requirements`
header and an inserted `## Purpose` placeholder. The current tooling normalizes a
purely-additive delta rather than copying its header through, so the failure mode
behind `b8fd849`, `68925d5` and `5341d11` does not reproduce today — those three
came from an older code path. Only the placeholder text
(`TBD - created by archiving change ...`) needed replacing, since the capability's
own first requirement forbids a Purpose that does not describe the capability.

This does not make the capability redundant. What was missing was never the
tool's correctness on any single day; it was the absence of any check that would
notice if that changed. The post-archive validation requirement stands, and it is
what turned this from an assumption into an observation.

### Decision 6 — Stop at the structural boundary, and accept an unclean validation result

Fixing the headers let validation reach the requirement bodies for the first time,
which immediately surfaced a third defect of a different kind: seven requirements
across `log-patterns-config`, `scanner` and `status-bar-timing` have no scenario,
and several are written as descriptive prose with no SHALL or MUST.

This is where the change stops, and the stopping point is chosen rather than
convenient. Decision 2 measured whether the headers were the only *structural*
problem and found they were. These are not structural: they are missing
specification content. Writing them means deciding what each requirement obliges
the system to do and what a passing test looks like, inferred from the
implementation — the same inference that wrote three commented-out test keywords
into `log-patterns-config` as shipped defaults. That work needs review, and it
cannot get review inside a change whose reviewable property is "the diff contains
no requirement text".

The cost is that this change ends with validation unclean:

```
0 passed / 11 failed   ->   8 passed / 3 failed
                            all 3 remaining are content defects,
                            enumerated in the proposal's Deferred section
```

and `fix-spec-config-drift` therefore stays blocked, because `archive` refuses on
any invalid target spec and `log-patterns-config` is one of its targets. Chaining
a third change is the price of keeping each one reviewable. The alternative —
folding the content work in here — buys one branch at the cost of making this
change's central claim ("not one requirement reworded") false.

*On the new capability's zero-failures requirement.* `spec-document-structure`
requires that `openspec validate --specs` report no failures, and this change ends
three short of that. A spec states the intended state, not the current one, and
the gap is tracked by the follow-up change rather than papered over by weakening
the requirement or annotating it with a status that would go stale. The deviation
is recorded in the proposal.

## Risks / Trade-offs

**[Editing `openspec/specs/` by hand sets a precedent]** → Mitigated by keeping
the edit mechanical and provably content-free: three header lines plus eleven
additive Purpose sections, with no requirement text touched. Decision 1 records
why the normal path could not carry it, so the precedent reads as "when the spec
directory itself is malformed", not "when a delta feels inconvenient".

**[A hand edit could silently corrupt a requirement]** → The header change is
`sed`-scoped to line 1 and matched exactly; Purpose sections are inserted above
`## Requirements` and add no normative text. Verification is
`openspec validate --specs` reaching 11/11 plus a diff review confirming no line
inside any `### Requirement:` block changed.

**[The underlying archive behavior is not fixed]** → Accepted. The defect lives
in tooling this project consumes rather than owns, and three occurrences over
three archives is a recognizable pattern rather than a mystery. The mitigation is
that this repair makes `validate --specs` meaningful, so a recurrence is caught
by running it after the next archive instead of two months later. That check is a
task here.

**[Newly visible requirements may be wrong]** → Out of scope by Decision 2 and
explicitly a Non-Goal. Nine `overview-panel` requirements have not been validated
against the implementation since `b8fd849`. This change makes them visible; it
does not claim they are accurate.

## Migration Plan

None. Documentation-only, on a branch, reversible with `git revert`. No build
artifact, no persisted state, nothing to deploy.

## Open Questions

**Should the newly visible requirements be audited against the code?** Nine
`overview-panel` requirements have gone unchecked by tooling since `b8fd849`, and
`fix-spec-config-drift` demonstrated that specs in this repo do drift from the
implementation. Deliberately not answered here — it is a review task, not a
structural repair, and folding it in would make this change unreviewable. Worth
raising once both changes have landed.
