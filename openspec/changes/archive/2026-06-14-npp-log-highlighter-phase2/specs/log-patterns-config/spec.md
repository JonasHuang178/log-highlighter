## MODIFIED Requirements

### Requirement: LogTypeRule struct
`LogTypeRule` in `config/LogPatterns.h` SHALL contain a `showInPanel` boolean
field that controls whether matches for this rule are shown as marks in the
Overview Panel. The field SHALL default to `false`. The `[ ERROR ]` rule
SHALL ship with `showInPanel = true`.

#### Scenario: showInPanel default false
- **WHEN** a new LogTypeRule is defined without specifying showInPanel
- **THEN** the rule's matches do not appear in the Overview Panel

#### Scenario: ERROR shown in panel by default
- **WHEN** the plugin is used with the default LogPatterns.h
- **THEN** only [ ERROR ] matches produce marks in the Overview Panel

### Requirement: StepTypeRule struct
`StepTypeRule` in `config/LogPatterns.h` SHALL contain a `showInPanel` boolean
field that controls whether matches for this rule are shown in the Overview Panel.
The field SHALL default to `false`.

#### Scenario: Step not shown in panel by default
- **WHEN** the plugin is used with the default LogPatterns.h
- **THEN** Step matches do not produce marks in the Overview Panel

#### Scenario: User enables Step in panel
- **WHEN** the user sets showInPanel = true for the Step rule and rebuilds
- **THEN** Step matches appear as green marks in the Overview Panel
