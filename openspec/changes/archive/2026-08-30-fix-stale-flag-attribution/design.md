## Context

`BufferState` (in `Plugin.h`) holds three per-buffer caches keyed by Notepad++ buffer id: the
Parse Log match list, the Next Bookmark line list, and the last rendered Custom Report. A single
`stale` flag, set from `SCN_MODIFIED`, invalidates the offset-dependent ones via
`InvalidateIfStale`, which each command calls before trusting its cache.

`SCN_MODIFIED` is a Scintilla notification. It reports what changed and where, but not which
Notepad++ buffer the change belongs to, so the handler has been resolving the buffer with
`NPPM_GETCURRENTBUFFERID` — the active one.

## Goals / Non-Goals

**Goals:**

- A cache is never served for a buffer whose content has changed since it was built
- No dependence on whether Notepad++ activates a buffer before modifying it
- The behaviour stays visible in the debug console

**Non-Goals:**

- Correct per-buffer attribution of modifications
- Any reduction in what is cached, or any change to cache rebuild timing

## Decisions

### Decision 1: Mark every tracked buffer, not the active one

**Choice**: on `SC_MOD_INSERTTEXT` / `SC_MOD_DELETETEXT`, iterate `g_bufferStates` and set
`stale` on every entry.

**Rationale**: the notification cannot identify its buffer, so the handler must either guess or
cover everything. Guessing is wrong exactly when it matters — a modification that did not go
through the active buffer — and wrong silently. Covering everything is wrong only in the sense of
doing surplus work, and that work is bounded and cheap.

An accurate alternative would be to map the notification's `hwndFrom` to a buffer, but Notepad++
swaps documents through two Scintilla views, so a view handle does not identify a buffer either.
Recovering true attribution would mean tracking activation and modification together — a great
deal of machinery to avoid a rescan.

**Cost**: editing any document invalidates the bookmark and report caches of every other open
document. Each is rebuilt by a single scan on next use, which is the cheap half of a parse — no
indicator fill, no highlight application — and paid once per edit rather than per keypress. A
buffer that is never used again pays nothing.

Entries are not created for buffers that have none. A buffer with no entry has no cache to
invalidate and will scan fresh on first use.

### Decision 2: `matches` and `highlightActive` stay untouched

**Choice**: unchanged — `InvalidateIfStale` still leaves the Parse Log match list alone.

**Rationale**: Parse Log rescans unconditionally, so nothing stale is ever reused from it. The
list survives only to restore Overview Panel marks on tab switch, where slightly out-of-date
marks beat a blank panel. This is the same trade the editor's own indicators make, and this
change does not disturb it.

### Decision 3: The breadcrumb reports the count

**Choice**: log one line when at least one buffer transitions from not-stale to stale, naming the
document that triggered it and how many buffers were marked.

**Rationale**: preserves the existing rate-limiting — after the first edit every buffer is
already stale, so continued typing produces no further lines — while making the wider effect
visible. The count is what distinguishes this behaviour from the old one at a glance.

## Risks / Trade-offs

- **[Unnecessary rescans]** → Accepted, and bounded: one scan per buffer per edit, on next use,
  with no indicator work. The alternative is serving wrong data.
- **[Large documents rescan after an unrelated edit]** → The scan is the cheap half of Parse Log.
  If this ever becomes measurable, the answer is accurate attribution, not narrower invalidation.
- **[The original hole may not have been reachable]** → Then this change costs some redundant
  scanning and buys certainty. That trade was chosen deliberately over a fifth verification
  attempt.
