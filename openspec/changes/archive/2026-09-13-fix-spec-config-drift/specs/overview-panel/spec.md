## ADDED Requirements

### Requirement: Panel background matches the adjacent scrollbar
The Overview Panel background SHALL be painted with the system color
`COLOR_BTNFACE`, obtained at runtime via `GetSysColor`, and SHALL NOT be
configurable.

The panel strip is carved out of Scintilla's non-client area immediately to the
right of the vertical scrollbar. The two are intended to read as a single
surface, so the background follows the system scrollbar background rather than a
fixed color of the plugin's own. Using the system color also means the strip
tracks the user's Windows theme without any setting.

Because this is a deliberate design position rather than an unimplemented
feature, `config/OverviewConfig.h` SHALL NOT define a background-color constant.
A constant that presents itself as a working setting while nothing reads it is a
defect — see the constant inventory in the `log-patterns-config` capability.

The viewport indicator box fill is unaffected by this requirement and remains
configurable through `OVERVIEW_VIEWPORT_BG_COLOR`.

#### Scenario: Panel background follows the system color
- **WHEN** the Overview Panel is painted
- **THEN** the full panel strip is filled with `GetSysColor(COLOR_BTNFACE)`

#### Scenario: Panel strip is visually continuous with the scrollbar
- **WHEN** the user views the right edge of the editor
- **THEN** the panel background and the adjacent scrollbar background are the same color

#### Scenario: Windows theme changed
- **WHEN** the user changes the Windows color scheme and the panel repaints
- **THEN** the panel background follows the new system color with no rebuild and no configuration change

#### Scenario: No background color constant is offered
- **WHEN** `config/OverviewConfig.h` is inspected
- **THEN** it defines no panel background color constant

#### Scenario: Viewport box fill remains configurable
- **WHEN** `OVERVIEW_VIEWPORT_BG_COLOR` is set to an explicit `RGB()` value and the plugin is rebuilt
- **THEN** the viewport indicator box is filled with that color while the panel background still uses `COLOR_BTNFACE`
