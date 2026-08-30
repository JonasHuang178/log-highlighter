## ADDED Requirements

### Requirement: Debug mode flag
`config/CustomReports.h` SHALL define `REPORT_DEBUG_MODE`, defaulting to `0`. When it is `0` no
console SHALL be created and no debug output SHALL be produced.

#### Scenario: Default build
- **WHEN** the plugin is built with `REPORT_DEBUG_MODE` set to `0`
- **THEN** no console window appears at any point
- **AND** calls to `Debug` and `Debugf` produce no output

#### Scenario: Debug build
- **WHEN** the plugin is built with `REPORT_DEBUG_MODE` set to `1`
- **THEN** a console window is available and debug output is produced

---

### Requirement: Debug flag is readable by the engine
`src/Report.h` SHALL expose whether debug mode is enabled, so that `Plugin.cpp` can act on it
without including `config/CustomReports.h`.

#### Scenario: Single inclusion preserved
- **WHEN** the plugin is compiled
- **THEN** `config/CustomReports.h` is included by `src/Report.cpp` only

---

### Requirement: Console allocated at Notepad++ startup
When debug mode is enabled the console SHALL be allocated while handling `NPPN_READY`, and SHALL
NOT be allocated from `DllMain`.

#### Scenario: Console appears on startup
- **WHEN** Notepad++ starts with a debug-mode build of the plugin
- **THEN** the console window appears without any report having been run

#### Scenario: No report run yet
- **WHEN** the console has been allocated and no report has been invoked
- **THEN** the console is present and empty apart from any startup breadcrumb

---

### Requirement: Console close box disabled
The console window SHALL have `SC_CLOSE` removed from its system menu, and the plugin SHALL
install a console control handler that does not allow `CTRL_CLOSE_EVENT` to terminate the
process.

#### Scenario: User attempts to close the console
- **WHEN** the user clicks the console window close box
- **THEN** the close box is unavailable and Notepad++ continues running

#### Scenario: Console minimised
- **WHEN** the user minimises the console window
- **THEN** it minimises normally and debug output continues to accumulate

---

### Requirement: Debug output API
`src/ReportApi.h` SHALL provide `Debug` and `Debugf` callable from a report function. `Debugf`
SHALL accept a printf-style format string and SHALL be implemented as a variadic template rather
than with C variadic arguments.

#### Scenario: Plain text output
- **WHEN** a report calls `Debug` with text
- **THEN** that text appears in the console as one line

#### Scenario: Formatted output
- **WHEN** a report calls `Debugf` with a format string and arguments
- **THEN** the formatted result appears in the console as one line

#### Scenario: Width and precision
- **WHEN** a numeric conversion carries width or precision, such as `%5d` or `%.2f`
- **THEN** the value is rendered with that width or precision

---

### Requirement: String views print with %s
A `%s` conversion SHALL accept `std::string_view`, `std::string` and `const char*` without the
author decomposing the value.

#### Scenario: string_view argument
- **WHEN** a report passes a `std::string_view` returned by a helper to a `%s` conversion
- **THEN** its characters are printed
- **AND** the author does not have to pass a size and pointer pair

#### Scenario: Empty string view
- **WHEN** a report passes an empty `std::string_view` to a `%s` conversion
- **THEN** nothing is printed for that conversion and the surrounding text is unaffected

---

### Requirement: Debug output cannot crash the editor
A mismatch between a conversion and its argument SHALL NOT cause an invalid memory access. The
implementation SHALL emit a visible marker in place of the value instead.

#### Scenario: Wrong conversion for the argument type
- **WHEN** a report passes an integer to a `%s` conversion
- **THEN** a visible marker is printed in place of the value
- **AND** Notepad++ continues running

#### Scenario: Too few arguments
- **WHEN** a format string contains more conversions than arguments supplied
- **THEN** the surplus conversions print a visible marker
- **AND** no memory outside the supplied arguments is read

---

### Requirement: Output throttling
`config/CustomReports.h` SHALL define `REPORT_DEBUG_MAX_LINES`. Author output SHALL be capped at
that many lines per report run, after which a single suppression notice is printed and further
author output is dropped. A value of `0` SHALL mean unlimited. The counter SHALL reset at the
start of each report run.

#### Scenario: Cap reached
- **WHEN** a report produces more author output lines than the cap
- **THEN** output stops after the cap
- **AND** one notice states that further output was suppressed

#### Scenario: Cap not reached
- **WHEN** a report produces fewer lines than the cap
- **THEN** all lines appear and no notice is printed

#### Scenario: Second run after a capped run
- **WHEN** a report that was capped is invoked again
- **THEN** output resumes from the beginning of the cap for that run

#### Scenario: Unlimited
- **WHEN** `REPORT_DEBUG_MAX_LINES` is `0`
- **THEN** no author output is suppressed

---

### Requirement: Engine breadcrumbs
When debug mode is enabled the engine SHALL print its own progress through a report run,
distinguishable from author output. Breadcrumbs SHALL NOT count toward the output cap and SHALL
NOT be suppressed by it.

#### Scenario: Report run breadcrumbs
- **WHEN** a report runs
- **THEN** the console shows the engine reaching at least: the report starting, the document
  snapshot taken, the report function entered, and the report function returned

#### Scenario: Breadcrumbs survive throttling
- **WHEN** author output has been capped during a run
- **THEN** the breadcrumb recording that the report function returned still appears

#### Scenario: Breadcrumbs are attributable
- **WHEN** any breadcrumb is printed
- **THEN** it is visually distinct from author output

---

### Requirement: Stale flag is observable
Engine breadcrumbs SHALL report the per-buffer `stale` state and buffer identity. The
`SCN_MODIFIED` handler SHALL print one line when `stale` transitions from `false` to `true`, and
SHALL NOT print on subsequent modifications while the flag is already set.

#### Scenario: First edit after a scan
- **WHEN** the document is edited for the first time since its caches were populated
- **THEN** one line is printed identifying the buffer and recording the transition to stale

#### Scenario: Continued typing
- **WHEN** the user keeps typing while the flag is already set
- **THEN** no further transition lines are printed

#### Scenario: Report run after an edit
- **WHEN** a report is invoked after an edit
- **THEN** a breadcrumb reports that the buffer was stale and that its caches were invalidated

---

### Requirement: Console output encoding
Author output SHALL be converted from the active document code page to UTF-16 and written with
`WriteConsoleW`, so that the console code page does not affect rendering.

#### Scenario: Non-ASCII text from a UTF-8 document
- **WHEN** a report prints non-ASCII text extracted from a UTF-8 document
- **THEN** it renders correctly in the console

#### Scenario: Non-ASCII text from an ANSI document
- **WHEN** a report prints non-ASCII text extracted from a document using code page 0
- **THEN** it renders correctly in the console
