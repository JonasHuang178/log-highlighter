# spec-document-structure Specification

## Purpose

The structural rules a file under `openspec/specs/` must satisfy to be readable
by the tooling at all. It exists as a capability because a spec that cannot be
parsed fails silently: it reads perfectly well to a human while its requirements
are invisible to validate, list and archive, and so go unverified without anyone
noticing.

## Requirements
### Requirement: Main specs carry Purpose and Requirements sections
Every `openspec/specs/<capability>/spec.md` SHALL contain a `## Purpose` section
followed by a `## Requirements` section, in that order.

`## Purpose` SHALL state what the capability covers and why it exists as a
separate capability, in one or two sentences. It SHALL NOT restate mechanism,
constants, keyword lists or any other detail that belongs in the requirements
below it or in `README.md`; a Purpose that describes behavior becomes an
unversioned second copy of the requirements and drifts from them.

#### Scenario: Spec has both sections
- **WHEN** any file matching `openspec/specs/*/spec.md` is inspected
- **THEN** it contains a `## Purpose` section
- **AND** it contains a `## Requirements` section appearing after it

#### Scenario: New capability added
- **WHEN** a change introduces a new capability spec
- **THEN** that spec carries a Purpose section before its requirements

---

### Requirement: Main specs contain no delta headers
A file under `openspec/specs/` SHALL NOT contain the delta headers
`## ADDED Requirements`, `## MODIFIED Requirements`, `## REMOVED Requirements` or
`## RENAMED Requirements`.

Those headers are valid only inside `openspec/changes/<name>/specs/<capability>/spec.md`.
In a main spec a delta header truncates the parsed `## Requirements` section, so
every requirement below it becomes invisible to `validate`, `list` and `archive` —
present in the file, absent from the tooling, and therefore unverifiable without
anyone noticing.

This condition has occurred three times, each time written by an `openspec archive`
run syncing a change whose delta consisted solely of `## ADDED Requirements`
(`b8fd849`, `68925d5`, `5341d11`). It is a recognizable pattern, not an accident,
and it is silent by nature: the affected requirements read perfectly well to a
human.

#### Scenario: Delta header in a main spec
- **WHEN** a file under `openspec/specs/` begins with `## ADDED Requirements`
- **THEN** the specs directory is non-conforming
- **AND** the requirements below that header are invisible to the tooling

#### Scenario: Delta header in a change directory
- **WHEN** `openspec/changes/<name>/specs/<capability>/spec.md` begins with `## ADDED Requirements`
- **THEN** that is correct and required, because delta headers belong there

---

### Requirement: Specs directory validates after every archive
`openspec validate --specs` SHALL report zero failures. It SHALL be run after
every `openspec archive`, because archive is the only operation that writes to
`openspec/specs/` and is the operation that has introduced every occurrence of a
malformed main spec so far.

#### Scenario: Validation passes
- **WHEN** `openspec validate --specs` is run against a conforming specs directory
- **THEN** every spec passes and the command reports zero failures

#### Scenario: Archive damages a main spec
- **WHEN** an archive run writes a delta header into a main spec
- **THEN** the validation run immediately afterwards fails
- **AND** the damage is repaired before further changes are archived, rather than accumulating undetected

