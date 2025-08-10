# Aeolus Terminal Interface

## Overview

The Aeolus terminal interface provides a text-based console interface for controlling the organ synthesizer. It displays organ stops in a grid layout and allows keyboard navigation and control.

## Screen Layout

The interface displays:
- Title Bar: Shows the current instrument name
- Group Columns: Organ divisions (keyboards) arranged in columns
- Stop Lists: Individual stops within each group
- Cursor: A ">" symbol showing your current position
- Status Line: Shows available controls and current state

## Visual Indicators

- Normal stops: White text
- Tremulants: Green text (if colors supported)
- Couplers: Yellow text (if colors supported)
- Active stops: Highlighted in reverse video
- Tuning stops: Blinking text during retuning

## Controls

### Navigation
- Arrow Keys: Move cursor between stops
  - Up/Down: Navigate within group or move between groups
  - Left/Right: Move between groups

### Stop Control
- Space: Toggle current stop on/off
- 0 or `: General cancel (turn off all stops)

### Presets
- 1-9: Recall preset 1-9
- Shift+1-9: Store current registration as preset 1-9

### Commands
- /: Enter command mode
- quit: Exit program (type after pressing /)
- Ctrl-D: Quit immediately

## Getting Started

1. Launch Aeolus with the terminal interface option
2. Wait for "Aeolus is ready" message in the status line
3. Use arrow keys to navigate between stops
4. Press Space to toggle stops on/off
5. Use number keys to recall saved presets

## Tips

- The interface automatically adjusts to your terminal size
- Controls are disabled during initial loading and tuning
- The cursor wraps between groups when reaching boundaries
- All changes are heard immediately through the audio output