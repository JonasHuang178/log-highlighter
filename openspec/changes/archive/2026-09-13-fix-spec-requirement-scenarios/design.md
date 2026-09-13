## Context

Six requirements across two specs fail `openspec validate --specs`. The prior
structural repair (`fix-openspec-spec-headers`) deliberately left them, naming
this change and stating the hazard: writing a scenario means asserting intended
behavior, and the only evidence at hand is the implementation. Inferring the
specification from the code is the exact mistake that produced the phantom
six-entry rule table `fix-spec-config-drift` had to delete.

So the governing question is not "what do these functions do" but "what was
already promised, and what am I adding".

## Decision 1 — Restate obligations, do not discover them

Every scenario must trace to a claim the requirement already makes in prose. The
requirements are descriptive, but they are not empty: "The callback is invoked
every 500 completed lines", "Matches are sorted by `byteOffset`", "`t1` is
captured after the Overview Panel `Update()` call" are all assertions about
intended behavior that happen to be phrased in the indicative. Converting those
to SHALL and attaching a WHEN/THEN adds testability, not content.

Anything that does *not* trace back this way is listed individually in the
proposal rather than absorbed into the rewrite. Four items qualified; they are
named there, so a reviewer can accept or reject each on its own.

**Rejected alternative:** read `Parser.cpp` and `Plugin.cpp` and specify what
they do. That would produce a more complete spec and a worthless one — it could
never disagree with the code, so it could never catch a defect, which is the
entire function of a specification. The `Step123` rule survived two months
precisely because nothing in the specs was capable of contradicting the config.

## Decision 2 — Correct `Match struct` rather than specify around it

The field table predates `MatchType::BOOKMARK` (`ae8195b`) and lists two match
types where the code has three.

Leaving it would mean writing scenarios for a requirement known to be wrong.
Correcting it is an inference from the implementation, which Decision 1 treats
as suspect — except that here the conclusion is independently supported:
`log-patterns-config` specifies `BookmarkRule`, and `bookmark-navigation`
specifies the whole Ctrl+Alt+W capability that consumes these matches. The
bookmark match type is already normative elsewhere. `scanner` is the one spec
that failed to hear about it.

That makes this a sync between specs, not a reading of the code. It is called
out in the proposal anyway, because the reasoning is the kind that deserves to
be checked by someone else.

## Decision 3 — Record the consumer of the sort-order guarantee

`Match struct` already required ascending `byteOffset` order, justified by
"because `ScanBuffer` scans left-to-right" — which explains why the order happens
to hold, not why anything may rely on it. An implementation detail offered as a
reason is an invitation to break it.

`BookmarkLinesFrom` in `Plugin.cpp` deduplicates bookmark lines by comparing each
against `lines.back()` only. Unsorted input would silently produce duplicate jump
targets in the Ctrl+Alt+W cycle. Naming that dependency converts an incidental
property into a guarantee with a cost attached to breaking it.

This is additive and low-risk: the ordering requirement already existed and is
already met.

## Decision 4 — Apply through a capability delta, not a direct write

`fix-openspec-spec-headers` wrote straight into `openspec/specs/` and justified
it at length: its target *was* the specs directory's structure, so no capability
delta could carry it.

That argument does not extend here. These are requirement bodies belonging to
`scanner` and `status-bar-timing` — ordinary capability content with an ordinary
`## MODIFIED Requirements` delta. The normal route applies, and `openspec
archive` performs the write.

This matters beyond tidiness: the direct-write precedent is the more dangerous
habit of the two, and it should stay attached to the one situation that actually
required it.

## Decision 5 — Expect archive to validate the rebuilt spec, and treat that as the check

`openspec archive` validates the *rebuilt* spec before writing, and aborts
without touching anything if it fails. Archiving `fix-spec-config-drift` hit
exactly that: `log-patterns-config` rebuilt with a scenario-less `MAKE_BGR macro`
requirement, and the archive refused.

Here that behavior is the acceptance test. If these deltas are complete, the
rebuilt `scanner` and `status-bar-timing` validate and the archive proceeds. If a
requirement was missed, the archive refuses and nothing is written. There is no
need for a separate pre-check, and no reason to reach for `--no-validate`.

## Risks

- **A scenario asserts a guarantee the plugin does not actually provide.** The
  mitigation is Decision 1 plus the itemised list in the proposal: four
  statements go beyond restatement and each is individually reviewable. The
  remainder cannot introduce a new claim, because they assert only what the
  existing prose already asserted.
- **The `BOOKMARK` correction is right for the wrong reason.** Decision 2 rests
  on other specs already specifying bookmarks. If that reading is wrong, the
  correction becomes a code-derived inference and should be rejected.
