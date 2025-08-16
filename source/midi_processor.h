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


#ifndef __MIDI_PROCESSOR_H
#define __MIDI_PROCESSOR_H


#include <stdint.h>
#include "lfqueue.h"
#include "global.h"


class MidiProcessor
{
public:

    // Callbacks for key and controller actions
    class Handler {
    public:
        virtual ~Handler() {}
        virtual void key_on(int note, int keyboard) = 0;
        virtual void key_off(int note, int keyboard) = 0;
        virtual void all_sound_off() = 0;
        virtual void all_notes_off(int keyboard) = 0;
        virtual void hold_pedal(int keyboard, bool on) = 0;
    };

    // Process raw MIDI events - common logic extracted from both backends
    static void process_midi_event(
        uint8_t status, uint8_t data1, uint8_t data2, uint8_t channel,
        uint16_t* midimap, Handler* handler, 
        Lfq_u32* qnote, Lfq_u8* qmidi);

    // Helper functions for MIDI event classification
    static bool is_note_on(uint8_t status, uint8_t velocity);
    static bool is_note_off(uint8_t status, uint8_t velocity);
    static bool is_controller(uint8_t status);
    static bool is_program_change(uint8_t status);

    // Queue writing helpers
    static bool write_note_queue(Lfq_u32* queue, uint8_t cmd, uint8_t note, uint8_t keyboard);
    static bool write_midi_queue(Lfq_u8* queue, uint8_t status, uint8_t data1, uint8_t data2);

private:
    // Common note processing logic
    static void process_note_event(
        uint8_t note, uint8_t velocity, bool is_on, uint8_t channel,
        uint16_t* midimap, Handler* handler, Lfq_u32* qnote, Lfq_u8* qmidi);

    // Common controller processing logic  
    static void process_controller_event(
        uint8_t controller, uint8_t value, uint8_t channel,
        uint16_t* midimap, Handler* handler, Lfq_u32* qnote, Lfq_u8* qmidi);
};


#endif