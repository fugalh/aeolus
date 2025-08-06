// ----------------------------------------------------------------------------
//
//  Copyright (C) 2003-2022 Fons Adriaensen <fons@linuxaudio.org>
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 3 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
// ----------------------------------------------------------------------------


#include "midi_processor.h"


void MidiProcessor::process_midi_event(
    uint8_t status, uint8_t data1, uint8_t data2, uint8_t channel,
    uint16_t* midimap, Handler* handler,
    Lfq_u32* qnote, Lfq_u8* qmidi)
{
    uint8_t msg_type = status & 0xF0;

    switch (msg_type)
    {
    case 0x80:  // Note off
    case 0x90:  // Note on
        process_note_event(data1, data2, is_note_on(status, data2), channel, 
                          midimap, handler, qnote, qmidi);
        break;

    case 0xB0:  // Controller
        process_controller_event(data1, data2, channel, midimap, handler, qnote, qmidi);
        break;

    case 0xC0:  // Program change
        {
            uint16_t f = midimap[channel] >> 12;
            if (f & 4)  // Control-enabled channel
            {
                write_midi_queue(qmidi, 0xC0, data1, 0);
            }
        }
        break;
    }
}


void MidiProcessor::process_note_event(
    uint8_t note, uint8_t velocity, bool is_on, uint8_t channel,
    uint16_t* midimap, Handler* handler, Lfq_u32* qnote, Lfq_u8* qmidi)
{
    uint16_t k = midimap[channel] & 15;
    uint16_t f = midimap[channel] >> 12;

    if (is_on)
    {
        // Note on
        if (note < 36)
        {
            if ((f & 4) && (note >= 24) && (note < 34))
            {
                // Preset selection for control-enabled channels
                write_midi_queue(qmidi, 0x90, note, velocity);
            }
        }
        else if (note <= 96)
        {
            if (f & 1)  // Keyboard-enabled channel
            {
                if (handler) handler->key_on(note - 36, k);
                write_note_queue(qnote, 1, note - 36, k);  // Note on command
            }
        }
    }
    else
    {
        // Note off
        if (note >= 36 && note <= 96)
        {
            if (f & 1)  // Keyboard-enabled channel
            {
                if (handler) handler->key_off(note - 36, k);
                write_note_queue(qnote, 0, note - 36, k);  // Note off command
            }
        }
    }
}


void MidiProcessor::process_controller_event(
    uint8_t controller, uint8_t value, uint8_t channel,
    uint16_t* midimap, Handler* handler, Lfq_u32* qnote, Lfq_u8* qmidi)
{
    uint16_t k = midimap[channel] & 15;
    uint16_t f = midimap[channel] >> 12;

    switch (controller)
    {
    case MIDICTL_HOLD:
        // Hold pedal
        if (f & 1)  // Keyboard-enabled channel
        {
            bool hold_on = (value > 63);
            if (handler) handler->hold_pedal(k, hold_on);
            uint8_t cmd = hold_on ? 9 : 8;
            write_note_queue(qnote, cmd, 0, k);
        }
        break;

    case MIDICTL_ASOFF:
        // All sound off - control channels only
        if (f & 4)
        {
            if (handler) handler->all_sound_off();
            write_note_queue(qnote, 2, 0, NKEYBD);  // All keyboards
        }
        break;

    case MIDICTL_ANOFF:
        // All notes off - keyboard channels only
        if (f & 1)
        {
            if (handler) handler->all_notes_off(k);
            write_note_queue(qnote, 2, 0, k);  // Specific keyboard
        }
        break;

    case MIDICTL_BANK:
    case MIDICTL_IFELM:
        // Bank select or interface element control - control channels only
        if (f & 4)
        {
            write_midi_queue(qmidi, 0xB0 | channel, controller, value);
        }
        break;

    case MIDICTL_SWELL:
    case MIDICTL_TFREQ:
    case MIDICTL_TMODD:
        // Division performance controls - division channels only
        if (f & 2)
        {
            write_midi_queue(qmidi, 0xB0 | channel, controller, value);
        }
        break;
    }
}


bool MidiProcessor::is_note_on(uint8_t status, uint8_t velocity)
{
    return ((status & 0xF0) == 0x90) && (velocity > 0);
}


bool MidiProcessor::is_note_off(uint8_t status, uint8_t velocity)
{
    return ((status & 0xF0) == 0x80) || (((status & 0xF0) == 0x90) && (velocity == 0));
}


bool MidiProcessor::is_controller(uint8_t status)
{
    return (status & 0xF0) == 0xB0;
}


bool MidiProcessor::is_program_change(uint8_t status)
{
    return (status & 0xF0) == 0xC0;
}


bool MidiProcessor::write_note_queue(Lfq_u32* queue, uint8_t cmd, uint8_t note, uint8_t keyboard)
{
    if (!queue || queue->write_avail() == 0) return false;
    
    uint32_t message = (cmd << 24) | (note << 16) | keyboard;
    queue->write(0, message);
    queue->write_commit(1);
    return true;
}


bool MidiProcessor::write_midi_queue(Lfq_u8* queue, uint8_t status, uint8_t data1, uint8_t data2)
{
    if (!queue || queue->write_avail() < 3) return false;
    
    queue->write(0, status);
    queue->write(1, data1);
    queue->write(2, data2);
    queue->write_commit(3);
    return true;
}