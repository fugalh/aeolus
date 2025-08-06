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

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "midi_processor.h"
#include "lfqueue.h"

using ::testing::_;
using ::testing::StrictMock;

// Mock handler to capture MIDI events
class MockMidiHandler : public MidiProcessor::Handler
{
public:
    MOCK_METHOD(void, key_on, (int note, int keyboard), (override));
    MOCK_METHOD(void, key_off, (int note, int keyboard), (override));
    MOCK_METHOD(void, all_sound_off, (), (override));
    MOCK_METHOD(void, all_notes_off, (int keyboard), (override));
    MOCK_METHOD(void, hold_pedal, (int keyboard, bool on), (override));
};

class MidiProcessorTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize midimap with proper flags:
        // Bit 0: keyboard-enabled (for note events)
        // Bit 2: control-enabled (for controllers)
        // Lower 4 bits: keyboard number
        for (int i = 0; i < 16; i++) {
            midimap[i] = i | (1 << 12) | (4 << 12);  // Enable keyboard and control flags
        }
    }
    
    StrictMock<MockMidiHandler> handler;
    Lfq_u32 note_queue{256};
    Lfq_u8 midi_queue{1024};
    uint16_t midimap[16];
};

TEST_F(MidiProcessorTest, NoteOnMessage) {
    // Note On: 0x90 (channel 0), note 60 (middle C), velocity 64
    uint8_t status = 0x90;
    uint8_t note = 60;
    uint8_t velocity = 64;
    uint8_t channel = 0;
    
    EXPECT_CALL(handler, key_on(24, 0));  // MIDI note 60 - 36 = 24, keyboard 0
    
    MidiProcessor::process_midi_event(status, note, velocity, channel,
                                     midimap, &handler, &note_queue, &midi_queue);
}

TEST_F(MidiProcessorTest, NoteOffMessage) {
    // Note Off: 0x80 (channel 0), note 60, velocity 0
    uint8_t status = 0x80;
    uint8_t note = 60;
    uint8_t velocity = 0;
    uint8_t channel = 0;
    
    EXPECT_CALL(handler, key_off(24, 0));  // MIDI note 60 - 36 = 24, keyboard 0
    
    MidiProcessor::process_midi_event(status, note, velocity, channel,
                                     midimap, &handler, &note_queue, &midi_queue);
}

TEST_F(MidiProcessorTest, AllSoundOffController) {
    // Control Change: 0xB0 (channel 0), controller 120 (All Sound Off), value 0
    uint8_t status = 0xB0;
    uint8_t controller = 120;
    uint8_t value = 0;
    uint8_t channel = 0;
    
    EXPECT_CALL(handler, all_sound_off());
    
    MidiProcessor::process_midi_event(status, controller, value, channel,
                                     midimap, &handler, &note_queue, &midi_queue);
}

TEST_F(MidiProcessorTest, AllNotesOffController) {
    // Control Change: 0xB1 (channel 1), controller 123 (All Notes Off), value 0
    uint8_t status = 0xB1;
    uint8_t controller = 123;
    uint8_t value = 0;
    uint8_t channel = 1;
    
    EXPECT_CALL(handler, all_notes_off(1));  // keyboard 1
    
    MidiProcessor::process_midi_event(status, controller, value, channel,
                                     midimap, &handler, &note_queue, &midi_queue);
}

TEST_F(MidiProcessorTest, SustainPedal) {
    // Control Change: 0xB2 (channel 2), controller 64 (Sustain), value 127 (on)
    uint8_t status = 0xB2;
    uint8_t controller = 64;
    uint8_t value = 127;
    uint8_t channel = 2;
    
    EXPECT_CALL(handler, hold_pedal(2, true));  // keyboard 2, on
    
    MidiProcessor::process_midi_event(status, controller, value, channel,
                                     midimap, &handler, &note_queue, &midi_queue);
    
    // Test sustain off
    value = 0;  // value 0 (off)
    EXPECT_CALL(handler, hold_pedal(2, false));  // keyboard 2, off
    
    MidiProcessor::process_midi_event(status, controller, value, channel,
                                     midimap, &handler, &note_queue, &midi_queue);
}

TEST_F(MidiProcessorTest, HelperFunctions) {
    // Test MIDI event classification helpers
    EXPECT_TRUE(MidiProcessor::is_note_on(0x90, 64));  // Note on with velocity
    EXPECT_FALSE(MidiProcessor::is_note_on(0x90, 0)); // Note on with zero velocity
    EXPECT_TRUE(MidiProcessor::is_note_off(0x80, 0));  // Note off
    EXPECT_TRUE(MidiProcessor::is_note_off(0x90, 0));  // Note on with zero velocity
    EXPECT_TRUE(MidiProcessor::is_controller(0xB0));   // Control change
    EXPECT_FALSE(MidiProcessor::is_controller(0x90));  // Note on
    EXPECT_TRUE(MidiProcessor::is_program_change(0xC0)); // Program change
    EXPECT_FALSE(MidiProcessor::is_program_change(0x90)); // Note on
}