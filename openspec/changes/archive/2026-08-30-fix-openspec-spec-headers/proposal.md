## Why

`openspec/specs/` has never passed validation:

```
openspec validate --specs  ->  0 passed, 11 failed
```

Two independent structural defects, neither of which touches what the plugin
does — but one of which is actively hiding requirements from the tooling.

**1. Three main specs begin with a leftover delta header.**
`overview-panel`, `progress-dialog` and `snap-to-nearest-mark` open with
`## ADDED Requirements` instead of `## Requirements`. That header is only valid
inside `openspec/changes/<name>/specs/`, and in a main spec it truncates the
parsed requirements section. The consequence is not cosmetic:

> Requirement headers appear outside the main `## Requirements` section, so they
> are currently invisible to validate, list, and archive.

Nine requirements in `overview-panel` alone — the NCA panel, track-aligned
drawing, click navigation, the viewport box, subclass migration — are not seen by
the tooling at all, and the same applies to every requirement in the other two
files. Each defect was written by a **past `openspec archive` run**
(`b8fd849`, `68925d5`, `5341d11`), so it has been accumulating silently.

**2. All eleven main specs are missing the required `## Purpose` section.**
The schema expects `## Purpose` followed by `## Requirements`; no spec has one.

This surfaced when the `fix-spec-config-drift` change tried to archive: its
`overview-panel` delta could not be applied because the target spec is
structurally invalid. `openspec archive` refused and changed nothing, which is
the correct behavior. That change is now blocked until this one lands.

## What Changes

- Replace the line-1 `## ADDED Requirements` header with `## Requirements` in
  `openspec/specs/overview-panel/spec.md`,
  `openspec/specs/progress-dialog/spec.md` and
  `openspec/specs/snap-to-nearest-mark/spec.md`. This is a one-line edit per
  file; a sweep confirmed no delta header appears anywhere else in any main spec,
  and no requirement text is added, removed or reworded.
- Add a `## Purpose` section to all eleven specs in `openspec/specs/`, stating
  what each capability covers. Purpose text is new prose; it introduces no
  requirement and changes no behavior.
- Take `openspec validate --specs` from **0 passed / 11 failed** to
  **8 passed / 3 failed**, the whole of the improvement being structural.

**Deferred (not in this change)**

Repairing the three specs whose requirements are malformed in *substance* rather
than in framing. Once the structural defects were fixed, validation could reach
the requirement bodies for the first time and found:

| Spec | Defect |
|---|---|
| `log-patterns-config` | 3 requirements with no scenario |
| `scanner` | 2 requirements with no scenario and no SHALL/MUST |
| `status-bar-timing` | 2 with no scenario, 2 with no SHALL/MUST |

Fixing these means writing new requirement text and inventing scenarios —
deciding what `Match struct` obliges the system to do and what a passing test for
it looks like. That is authoring specification content, not repairing structure,
and it cannot be done mechanically: a scenario asserts intended behavior, so it
would be inferred from the implementation — the same direction of inference that
produced the phantom rule table `fix-spec-config-drift` had to remove. It gets its
own change, `fix-spec-requirement-scenarios`, and its own review.

Consequence, stated plainly: `openspec/specs/` is still not fully valid at the end
of this change, so **`fix-spec-config-drift` remains blocked** until the follow-up
lands. `openspec archive` refuses on any invalid target spec, and
`log-patterns-config` is one of its targets.

The `spec-document-structure` requirement that validation report zero failures is
therefore adopted here as the target state while three specs are knowingly
non-conforming. That deviation is tracked by the follow-up change, not hidden.

**This change writes directly into `openspec/specs/`**, which is otherwise
written only by `openspec archive`. That is deliberate and is the subject of a
design decision: the target of the repair *is* the specs directory, so there is
no capability delta that could carry it.

## Capabilities

### New Capabilities

- `spec-document-structure`: the structural rules a file under `openspec/specs/`
  must satisfy — it carries `## Purpose` and `## Requirements`, it never carries a
  delta header, and `openspec validate --specs` is run after every archive.

  This capability exists because the defect being repaired here has occurred
  three times, always written by an archive run, and is silent by nature: a
  truncated spec reads perfectly well to a human while its requirements are
  invisible to the tooling. Repairing the three files without recording the rule
  would leave the next occurrence just as undetectable. This follows the approach
  taken in `fix-spec-config-drift`, where the exact-set config inventories turned
  a one-time cleanup into a checkable constraint.

### Modified Capabilities

None. No existing requirement is added to, removed, reworded or reinterpreted.
Every requirement in `openspec/specs/` today continues to exist with identical
text; three files merely stop hiding theirs from the tooling, and eleven gain a
Purpose section that carries no normative content.

Because no existing capability has a delta, the repair itself is applied by
editing `openspec/specs/` directly. Archiving then runs normally — **not** with
`--skip-specs` — so that the new `spec-document-structure` capability is written
into `openspec/specs/`. See design Decision 1, and Decision 5 for what to expect
when it does.

## Impact

**Files**

- `openspec/specs/overview-panel/spec.md` — line 1, plus Purpose
- `openspec/specs/progress-dialog/spec.md` — line 1, plus Purpose
- `openspec/specs/snap-to-nearest-mark/spec.md` — line 1, plus Purpose
- The remaining eight specs under `openspec/specs/` — Purpose only

**Not affected**

- No file under `log-highlighter/` changes. No build, no plugin behavior, no
  configuration.
- No requirement text is edited.

**Downstream**

- `fix-spec-config-drift` (branch `fix/spec-config-drift`, commits `ed47791`,
  `8426751`) can archive once this lands.

**Risk**

Low, and the one open worry has been measured rather than assumed. Making the
previously unreachable requirements visible could have surfaced further
complaints in them. It does not: with the three headers corrected and no other
edit, validation reports exactly one issue per spec — the missing `Purpose`
section — uniformly across all eleven. The requirements that were hidden are
structurally clean; they were only ever unreachable, never malformed.
