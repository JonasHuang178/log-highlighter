## Why

`SCN_MODIFIED` carries no buffer id, so the handler flags `CurrentBuffer()` — the *active*
buffer. When a modification reaches a buffer that is not the active one, the wrong `BufferState`
is marked and the edited buffer keeps a cache that no longer matches its content. Next Bookmark
then navigates to stale line numbers, and Custom Report serves a report describing the previous
version of the document. Neither shows any error; the results simply look plausible and are wrong.

The realistic trigger is Replace All in All Opened Documents, which log users do reach for.

Four verification attempts failed to establish whether Notepad++ actually produces this
situation — each for a different reason: both measurements landing on one tab, an unread dialog,
a replace that never reached the second buffer, and a Notepad++ restart that cleared the state
table. The question is still open.

It does not need to be answered. The defensive fix is three lines and removes the risk whether or
not the condition can occur. Continuing to investigate costs more than the fix.

## What Changes

- **Change** the `SCN_MODIFIED` handler to mark every tracked buffer stale rather than only the
  active one
- **Change** the debug breadcrumb to report how many buffers were marked, so the behaviour stays
  observable

## Capabilities

### Modified Capabilities

- `per-buffer-state`: stale marking becomes global across tracked buffers instead of
  active-buffer-only

## Impact

- `src/Plugin.cpp`: the `SCN_MODIFIED` handler and its breadcrumb
- No change to `BufferState`, to what each command caches, or to when caches are rebuilt

## Non-Goals

- **Determining whether the hole is reachable.** This change makes the answer irrelevant.
- **Attributing modifications to the correct buffer.** That would need a buffer id the
  notification does not carry. Over-invalidating is accepted instead.
- Any change to Overview Panel behaviour: `matches` and `highlightActive` remain untouched by
  invalidation, exactly as before.
