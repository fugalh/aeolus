# Aeolus MIDI Protocol

## Overview
This document describes how Aeolus responds to MIDI messages. Messages are organized by type with hexadecimal formats and Aeolus-specific behaviors.

## Key Concepts

Division: Audio processing unit (Great, Swell, etc.) with synthesis and effects including Tremulant and Expression.

Group: UI container for stops/controls (8 groups × up to 32 buttons each). As in a real organ, a group may mostly map to a division but might include other elements.

Preset: Combination action recalled by Program Change (or notes 24-33).

Stop: Individual voice/sound controlled via groups.

### Channel Configuration

Each MIDI channel has a 16-bit midimap value:
- Bit 0: Keyboard (notes 36-96, hold pedal, all notes off)
- Bit 1: Division (expression, tremulant)
- Bit 2: Control (presets, stops, bank select)

Examples:
- 1 (0b001): Keyboard only
- 3 (0b011): Keyboard + Division
- 4 (0b100): Control only
- 7 (0b111): All

Configuration: Set via Aeolus MIDI dialog at runtime.

## Notation
- `c`: MIDI channel (0x0-F)
- `nn`: Note number (0x00-7F)
- `vv`: Value (0x00-7F)
- `pp`: Program number (0x00-1F)

## Note Messages

### Note On: `9c nn vv`
- Notes 24-33: Preset recall (control channels only)
  - Note 24 → Preset 0, Note 25 → Preset 1, etc.
  - Provides same functionality as Program Change messages
  - Velocity ignored
- Notes 36-96: Keyboard notes (keyboard channels only)  
  - Organ range C2-C7
  - Velocity used as binary trigger (0 = note off, >0 = note on)
- Notes >96: Ignored

### Note Off: `8c nn vv`
- Notes 36-96: Release keyboard note (keyboard channels only)
- Notes 24-33, >96: Ignored

## Controller Messages: `Bc pp vv`

### Keyboard Controls (keyboard channels)
- 64 (0x40) - Hold Pedal: ≥0x40 = sustain on, <0x40 = off
- 123 (0x7B) - All Notes Off: `Bc 7B 00` (preserves held notes)

### Division Controls (division channels)
- 7 (0x07) - Swell: Expression/volume control
- 12 (0x0C) - Tremulant Frequency: 2.0-9.0 Hz range
- 13 (0x0D) - Tremulant Amplitude: 0.0-0.3 range

### General Controls (control channels)
- 32 (0x20) - Bank Select: Select preset bank (0-31)
- 98 (0x62) - Stop Control: Two-message sequence:
  1. Mode: `Bc 62 mm` (bits 5-4: mode, bits 3-0: group)
  2. Button: `Bc 62 bb` (button 0-31)
  3. Clear group: `Bc 62 0g` (g = group 0-7)
- 120 (0x78) - All Sound Off: `Bc 78 00`

## Program Change: `Cc pp`
Recalls preset `pp` from selected bank (control channels only).
Presets are stored via the Aeolus UI.


## Examples

```
# Play middle C
90 3C 64    # Note on
80 3C 00    # Note off

# Recall preset 5  
C0 05       # Program change

# Activate stop: group 2, button 10
B0 62 22    # Mode=on, group=2
B0 62 0A    # Button 10

# Set swell to half
B0 07 40    # Swell = 64
```
