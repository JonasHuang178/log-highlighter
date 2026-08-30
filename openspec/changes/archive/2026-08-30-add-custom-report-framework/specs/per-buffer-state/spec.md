## ADDED Requirements

### Requirement: Per-buffer scan caches
`BufferState` SHALL hold, in addition to the existing match list, a bookmark line cache used by
Next Bookmark and a rendered report cache used by Custom Report commands.

#### Scenario: Independent population
- **WHEN** any one of Parse Log, Next Bookmark or Custom Report runs
- **THEN** only the cache belonging to that command is required to be populated for it to succeed

---

### Requirement: Stale flag driven by document modification
`BufferState` SHALL carry a `stale` flag. The plugin SHALL handle `SCN_MODIFIED` for
`SC_MOD_INSERTTEXT` and `SC_MOD_DELETETEXT` and set `stale` on the affected buffer. Handling this
notification SHALL NOT trigger any scan, parse, highlight or report.

#### Scenario: User types in the document
- **WHEN** text is inserted or deleted in a buffer
- **THEN** that buffer's `stale` flag is set
- **AND** no parse, scan or report is started

#### Scenario: Unrelated modifications
- **WHEN** a modification occurs that is neither an insertion nor a deletion of text
- **THEN** the `stale` flag is not set

---

### Requirement: Stale invalidates every cached scan
When `stale` is set, the match list, the bookmark line cache and the report cache SHALL all be
treated as invalid. The next invocation of a command SHALL rescan before using its cache, and
SHALL clear `stale` once its cache is repopulated.

#### Scenario: Report after an edit
- **WHEN** the document is edited after a report was displayed
- **AND** the user invokes the same report again
- **THEN** the document is rescanned and the report describes the current content

#### Scenario: Navigation after an edit
- **WHEN** the document is edited after Next Bookmark has cached bookmark lines
- **AND** the user presses Ctrl+Alt+W
- **THEN** the document is rescanned and navigation targets current line positions

#### Scenario: Highlights after an edit
- **WHEN** the document is edited after Parse Log has run
- **THEN** existing highlights remain on screen and are not refreshed automatically
- **AND** the next Parse Log rescans rather than reusing the cached match list

---

### Requirement: Cache lifetime follows the buffer
All caches and the `stale` flag SHALL be released when the buffer is closed, via the existing
`NPPN_FILEBEFORECLOSE` handler.

#### Scenario: Tab closed
- **WHEN** a buffer with populated caches is closed
- **THEN** all of its cached state is released

#### Scenario: Tab switch
- **WHEN** the user switches between tabs
- **THEN** each buffer retains its own caches and stale flag independently
