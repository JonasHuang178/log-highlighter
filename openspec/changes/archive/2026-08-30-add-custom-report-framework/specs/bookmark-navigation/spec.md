## MODIFIED Requirements

### Requirement: Jump to next Bookmark match
When invoked, the command SHALL find the next Bookmark match after the current caret line and
navigate the editor to that line, centered in the viewport. The command SHALL obtain bookmark
line numbers from its own per-buffer cache, populating that cache by scanning the document when
it is empty or invalidated. The command SHALL NOT require Parse Log to have run.

#### Scenario: Invoked without a prior Parse Log
- **WHEN** the buffer has never been parsed
- **AND** the user invokes "Next Bookmark"
- **THEN** the document is scanned for Bookmark keywords
- **AND** navigation proceeds to the first Bookmark match below the caret

#### Scenario: Match exists below caret
- **WHEN** the bookmark cache is populated
- **AND** there is a Bookmark match on a line below the current caret line
- **THEN** the editor navigates to the nearest such line below the caret
- **AND** the caret is placed at the beginning of that line
- **AND** the line is centered vertically in the viewport

#### Scenario: No match below caret but matches exist above
- **WHEN** all Bookmark matches are on lines above or equal to the current caret line
- **THEN** the editor wraps around and navigates to the first Bookmark match in the buffer

#### Scenario: Caret on a Bookmark line
- **WHEN** the caret is on a line that contains a Bookmark match
- **THEN** the command navigates to the next Bookmark match after the current line (not the same
  line)

#### Scenario: Repeated invocation
- **WHEN** the user presses Ctrl+Alt+W several times without editing the document
- **THEN** the document is scanned at most once and subsequent presses navigate immediately

#### Scenario: Invoked after an edit
- **WHEN** the document has been edited since the bookmark cache was populated
- **THEN** the document is rescanned and navigation uses current line positions

---

### Requirement: No matches feedback
When the current buffer contains no Bookmark matches, the command SHALL display a status bar
message indicating that none were found. The message SHALL NOT instruct the user to run Parse
Log, since Parse Log is no longer a precondition.

#### Scenario: Document contains no Bookmark keywords
- **WHEN** the document is scanned and contains no Bookmark keywords
- **AND** the user invokes "Next Bookmark"
- **THEN** the status bar displays a message such as "No Bookmark matches found."
- **AND** the caret does not move

---

### Requirement: Parse Log populates the bookmark cache
Parse Log SHALL populate the bookmark line cache for the current buffer as a side effect of its
scan. This SHALL NOT create a dependency in either direction: Next Bookmark scans when the cache
is empty, whether or not Parse Log has run.

#### Scenario: Parse Log run first
- **WHEN** the user runs Parse Log and then invokes "Next Bookmark"
- **THEN** navigation occurs without a second scan

#### Scenario: Parse Log cancelled
- **WHEN** the user cancels Parse Log and then invokes "Next Bookmark"
- **THEN** Next Bookmark scans the document itself and navigates normally
