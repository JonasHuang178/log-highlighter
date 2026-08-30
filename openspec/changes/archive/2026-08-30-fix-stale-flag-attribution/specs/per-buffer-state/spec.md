## MODIFIED Requirements

### Requirement: Stale flag driven by document modification
`BufferState` SHALL carry a `stale` flag. The plugin SHALL handle `SCN_MODIFIED` for
`SC_MOD_INSERTTEXT` and `SC_MOD_DELETETEXT` and set `stale` on **every tracked buffer**, not only
the active one. Handling this notification SHALL NOT trigger any scan, parse, highlight or
report, and SHALL NOT create state entries for buffers that have none.

#### Scenario: User types in the document
- **WHEN** text is inserted or deleted in a buffer
- **THEN** every tracked buffer's `stale` flag is set
- **AND** no parse, scan or report is started

#### Scenario: Modification reaches a buffer that is not active
- **WHEN** a document is modified without being the active buffer, as with Replace All in All
  Opened Documents
- **THEN** that buffer's caches are still invalidated
- **AND** no command serves a result built before the modification

#### Scenario: Unrelated modifications
- **WHEN** a modification occurs that is neither an insertion nor a deletion of text
- **THEN** no `stale` flag is set

#### Scenario: Buffer with no cached state
- **WHEN** a modification occurs and some open buffer has no `BufferState` entry
- **THEN** no entry is created for it
- **AND** it scans fresh on first use

---

### Requirement: Stale invalidates every cached scan
When `stale` is set, the bookmark line cache and the report cache SHALL be treated as invalid.
The next invocation of a command SHALL rescan before using its cache, and SHALL clear `stale` for
that buffer once its cache is repopulated. Clearing SHALL apply only to the buffer being used, so
that other buffers remain invalidated until they are next used.

#### Scenario: Report after an edit
- **WHEN** the document is edited after a report was displayed
- **AND** the user invokes the same report again
- **THEN** the document is rescanned and the report describes the current content

#### Scenario: Navigation after an edit
- **WHEN** the document is edited after Next Bookmark has cached bookmark lines
- **AND** the user presses Ctrl+Alt+W
- **THEN** the document is rescanned and navigation targets current line positions

#### Scenario: Another buffer used later
- **WHEN** one buffer has rescanned and cleared its own flag after an edit
- **AND** the user switches to a different buffer and invokes a command
- **THEN** that buffer also rescans rather than serving its pre-edit cache

#### Scenario: Highlights after an edit
- **WHEN** the document is edited after Parse Log has run
- **THEN** existing highlights remain on screen and are not refreshed automatically
- **AND** the next Parse Log rescans rather than reusing the cached match list
