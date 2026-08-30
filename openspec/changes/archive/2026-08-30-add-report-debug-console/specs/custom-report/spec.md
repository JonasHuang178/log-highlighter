## MODIFIED Requirements

### Requirement: Report result caching and invalidation
A rendered report SHALL be cached per buffer and reused on subsequent invocations of the same
report until the cache is invalidated. When debug mode is enabled the cache SHALL be bypassed and
the report function SHALL be executed on every invocation.

#### Scenario: Repeated invocation
- **WHEN** the same report is invoked twice with no intervening document edit
- **AND** debug mode is disabled
- **THEN** the second invocation displays the cached result without re-traversing the document

#### Scenario: Invocation after an edit
- **WHEN** the document is edited and the report is invoked again
- **THEN** the document is re-traversed and the report reflects the current content

#### Scenario: Repeated invocation in debug mode
- **WHEN** the same report is invoked twice with no intervening document edit
- **AND** debug mode is enabled
- **THEN** the report function is executed both times
- **AND** debug output is produced on both invocations
