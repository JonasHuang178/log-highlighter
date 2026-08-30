## ADDED Requirements

### Requirement: Modal report window
Report output SHALL be displayed in a modal window that runs its own message loop, so that
Notepad++'s accelerator table does not intercept keyboard input destined for the window.

#### Scenario: Dialog is open
- **WHEN** a report dialog is displayed
- **THEN** the Notepad++ main window does not process input until the dialog closes

---

### Requirement: Read-only multiline text area
The dialog SHALL display report text in an edit control created with `ES_MULTILINE`,
`ES_READONLY` and `WS_VSCROLL`, and SHALL send `EM_SETLIMITTEXT` with `wParam = 0` to remove the
default character limit.

#### Scenario: Report longer than the window
- **WHEN** report text exceeds the visible height
- **THEN** a vertical scrollbar allows the remainder to be read

#### Scenario: Report exceeding the default edit limit
- **WHEN** report text is longer than 32767 characters
- **THEN** it is displayed in full and is not truncated

#### Scenario: User attempts to edit
- **WHEN** the user types into the text area
- **THEN** the content is unchanged

---

### Requirement: Monospace rendering
The dialog SHALL render report text in a fixed-pitch font so that aligned key columns line up.

#### Scenario: Aligned output
- **WHEN** a report emits several `KV` entries in one section
- **THEN** their value columns line up vertically

---

### Requirement: Copying report text
The user SHALL be able to copy report text by selection followed by the edit control's
right-click Copy or Select All commands, and by Ctrl+C.

#### Scenario: Copy a selected portion
- **WHEN** the user selects part of the report and chooses Copy from the right-click menu
- **THEN** the selected text is placed on the clipboard

#### Scenario: Copy everything
- **WHEN** the user chooses Select All from the right-click menu and then Copy
- **THEN** the entire report is placed on the clipboard

---

### Requirement: Line endings in displayed text
Report text SHALL use `\r\n` line endings when written to the edit control.

#### Scenario: Multi-line report
- **WHEN** a report emits several lines
- **THEN** they appear on separate lines in the dialog

---

### Requirement: Closing the dialog
The dialog SHALL close on ESC, on the close button, and on a Close control.

#### Scenario: ESC pressed
- **WHEN** the user presses ESC
- **THEN** the dialog closes and focus returns to the editor

---

### Requirement: Dialog caption and sizing
The dialog caption SHALL identify the plugin and the report title. The window SHALL be resizable,
with the text area filling the client area at all sizes.

#### Scenario: Window resized
- **WHEN** the user resizes the dialog
- **THEN** the text area resizes to fill the new client area
