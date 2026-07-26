## Requirements

### Requirement: Next Bookmark command
The plugin SHALL register a "Next Bookmark" command in the Notepad++ Plugins menu under log-highlighter.

#### Scenario: Command appears in menu
- **WHEN** the plugin is loaded
- **THEN** "Next Bookmark" appears as a menu item under Plugins > log-highlighter

### Requirement: Ctrl+Alt+W shortcut
The "Next Bookmark" command SHALL be bound to Ctrl+Alt+W as its default shortcut.

#### Scenario: Shortcut triggers navigation
- **WHEN** the user presses Ctrl+Alt+W
- **THEN** the "Next Bookmark" command executes

### Requirement: Jump to next Bookmark match
When invoked, the command SHALL find the next `BOOKMARK` type match after the current caret line and navigate the editor to that line, centered in the viewport.

#### Scenario: Match exists below caret
- **WHEN** the buffer has been parsed and contains Bookmark matches
- **AND** there is a Bookmark match on a line below the current caret line
- **THEN** the editor navigates to the nearest such line below the caret
- **AND** the caret is placed at the beginning of that line
- **AND** the line is centered vertically in the viewport

#### Scenario: No match below caret but matches exist above
- **WHEN** all Bookmark matches are on lines above or equal to the current caret line
- **THEN** the editor wraps around and navigates to the first Bookmark match in the buffer

#### Scenario: Caret on a Bookmark line
- **WHEN** the caret is on a line that contains a Bookmark match
- **THEN** the command navigates to the next Bookmark match after the current line (not the same line)

### Requirement: No matches feedback
When there are no Bookmark matches in the current buffer, the command SHALL display a status bar message indicating no matches were found.

#### Scenario: Buffer not parsed
- **WHEN** the buffer has not been parsed yet (no matches exist)
- **AND** the user invokes "Next Bookmark"
- **THEN** the status bar displays a message such as "No Bookmark matches. Run Parse Log first."

#### Scenario: Buffer parsed but no Bookmark keywords
- **WHEN** the buffer has been parsed but contains no Bookmark keywords
- **AND** the user invokes "Next Bookmark"
- **THEN** the status bar displays a message such as "No Bookmark matches found."

### Requirement: Navigation scrolling behavior
The command SHALL scroll the editor so the target line is centered vertically in the viewport, using deferred timer navigation to prevent Notepad++ from overriding the scroll position.

#### Scenario: Target line is off-screen
- **WHEN** the next Bookmark match is on a line not currently visible
- **THEN** the editor scrolls to center that line vertically with the caret positioned at the start of the line
