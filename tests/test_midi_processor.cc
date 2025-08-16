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
#include <array>
#include <vector>
#include <memory>
#include "midi_processor.h"
#include "lfqueue.h"

using ::testing::_;
using ::testing::StrictMock;
using namespace std::literals;

// Modern C++20 MIDI event structure for testing
struct MidiEvent {
    uint8_t status;
    uint8_t data1;
    uint8_t data2;
    uint8_t channel;
    
    constexpr MidiEvent(uint8_t s, uint8_t d1, uint8_t d2, uint8_t ch) 
        : status(s), data1(d1), data2(d2), channel(ch) {}
    
    // Named constructors for common MIDI events
    static constexpr MidiEvent note_on(uint8_t channel, uint8_t note, uint8_t velocity = 64) {
        return {static_cast<uint8_t>(0x90 | channel), note, velocity, channel};
    }
    
    static constexpr MidiEvent note_off(uint8_t channel, uint8_t note, uint8_t velocity = 0) {
        return {static_cast<uint8_t>(0x80 | channel), note, velocity, channel};
    }
    
    static constexpr MidiEvent controller(uint8_t channel, uint8_t controller, uint8_t value) {
        return {static_cast<uint8_t>(0xB0 | channel), controller, value, channel};
    }
};

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

// RAII helper class for MIDI processing tests
class MidiTestEnvironment {
public:
    MidiTestEnvironment() {
        // Initialize midimap with proper flags according to MIDI.md:
        // Bits 0-3: Keyboard number (channel)
        // Bits 12-15: Channel flags (bit 0=keyboard, bit 1=division, bit 2=control)
        for (auto i = 0u; i < midimap.size(); ++i) {
            midimap[i] = static_cast<uint16_t>(i | (7u << 12u));  // All capabilities + keyboard number
        }
    }
    
    void configureMidimap(int channel, bool keyboard, bool division, bool control) {
        auto flags = 0u;
        if (keyboard) flags |= 1u;   // Bit 0 in high nibble
        if (division) flags |= 2u;   // Bit 1 in high nibble
        if (control) flags |= 4u;    // Bit 2 in high nibble
        midimap[channel] = static_cast<uint16_t>(channel | (flags << 12u));  // Channel number + flags
    }
    
    void processMidiEvent(const MidiEvent& event, MockMidiHandler& handler) {
        MidiProcessor::process_midi_event(event.status, event.data1, event.data2, event.channel,
                                        midimap.data(), &handler, &note_queue, &midi_queue);
    }
    
    std::array<uint16_t, 16> midimap{};
    Lfq_u32 note_queue{256};
    Lfq_u8 midi_queue{1024};
};

class MidiProcessorTest : public ::testing::Test {
protected:
    StrictMock<MockMidiHandler> handler;
    MidiTestEnvironment midi_env;
};

TEST_F(MidiProcessorTest, NoteOnMessage) {
    // Test modern MIDI event processing with RAII helper
    constexpr auto event = MidiEvent::note_on(0, 60, 64);  // Channel 0, middle C, velocity 64
    
    EXPECT_CALL(handler, key_on(24, 0));  // MIDI note 60 - 36 = 24, keyboard 0
    
    midi_env.processMidiEvent(event, handler);
}

TEST_F(MidiProcessorTest, NoteOffMessage) {
    // Note Off: 0x80 (channel 0), note 60, velocity 0
    uint8_t status = 0x80;
    uint8_t note = 60;
    uint8_t velocity = 0;
    uint8_t channel = 0;
    
    EXPECT_CALL(handler, key_off(24, 0));  // MIDI note 60 - 36 = 24, keyboard 0
    
    midi_env.processMidiEvent(MidiEvent{status, note, velocity, channel,
                                     }, handler);
}

TEST_F(MidiProcessorTest, AllSoundOffController) {
    // Control Change: 0xB0 (channel 0), controller 120 (All Sound Off), value 0
    uint8_t status = 0xB0;
    uint8_t controller = 120;
    uint8_t value = 0;
    uint8_t channel = 0;
    
    EXPECT_CALL(handler, all_sound_off());
    
    midi_env.processMidiEvent(MidiEvent{status, controller, value, channel,
                                     }, handler);
}

TEST_F(MidiProcessorTest, AllNotesOffController) {
    // Control Change: 0xB1 (channel 1), controller 123 (All Notes Off), value 0
    uint8_t status = 0xB1;
    uint8_t controller = 123;
    uint8_t value = 0;
    uint8_t channel = 1;
    
    EXPECT_CALL(handler, all_notes_off(1));  // keyboard 1
    
    midi_env.processMidiEvent(MidiEvent{status, controller, value, channel,
                                     }, handler);
}

TEST_F(MidiProcessorTest, SustainPedal) {
    // Control Change: 0xB2 (channel 2), controller 64 (Sustain), value 127 (on)
    uint8_t status = 0xB2;
    uint8_t controller = 64;
    uint8_t value = 127;
    uint8_t channel = 2;
    
    EXPECT_CALL(handler, hold_pedal(2, true));  // keyboard 2, on
    
    midi_env.processMidiEvent(MidiEvent{status, controller, value, channel,
                                     }, handler);
    
    // Test sustain off
    value = 0;  // value 0 (off)
    EXPECT_CALL(handler, hold_pedal(2, false));  // keyboard 2, off
    
    midi_env.processMidiEvent(MidiEvent{status, controller, value, channel,
                                     }, handler);
}

// Test midimap behavior according to MIDI.md
TEST_F(MidiProcessorTest, MidimapKeyboardOnlyChannel) {
    midi_env.configureMidimap(0, true, false, false);  // Keyboard only (bit 0)
    
    // Should accept keyboard notes (36-96)
    EXPECT_CALL(handler, key_on(24, 0));  // Note 60 - 36 = 24
    midi_env.processMidiEvent(MidiEvent::note_on(0, 60, 64), handler);
    
    // Should accept hold pedal (controller 64)
    EXPECT_CALL(handler, hold_pedal(0, true));
    midi_env.processMidiEvent(MidiEvent::controller(0, 64, 127), handler);
    
    // Should accept all notes off (controller 123)
    EXPECT_CALL(handler, all_notes_off(0));
    midi_env.processMidiEvent(MidiEvent::controller(0, 123, 0), handler);
}

TEST_F(MidiProcessorTest, MidimapControlOnlyChannel) {
    midi_env.configureMidimap(1, false, false, true);  // Control only (bit 2)
    
    // Should accept program change (presets)
    // Note: Program change handling would need to be tested when implemented
    
    // Should NOT accept keyboard notes - no expectation = StrictMock will fail if called
    midi_env.processMidiEvent(MidiEvent{0x91, 60, 64, 1, }, handler);
}

TEST_F(MidiProcessorTest, MidimapDisabledChannel) {
    midi_env.configureMidimap(2, false, false, false);  // No capabilities
    
    // Should reject all messages - no expectations = StrictMock will fail if called
    midi_env.processMidiEvent(MidiEvent{0x92, 60, 64, 2, }, handler);
    midi_env.processMidiEvent(MidiEvent{0xB2, 64, 127, 2, }, handler);
    midi_env.processMidiEvent(MidiEvent{0xB2, 120, 0, 2, }, handler);
}

TEST_F(MidiProcessorTest, MidimapKeyboardAndControl) {
    midi_env.configureMidimap(3, true, false, true);  // Keyboard + Control (bits 0,2)
    
    // Should accept both keyboard notes and control messages
    EXPECT_CALL(handler, key_on(24, 3));
    midi_env.processMidiEvent(MidiEvent{0x93, 60, 64, 3, }, handler);
    
    EXPECT_CALL(handler, all_sound_off());
    midi_env.processMidiEvent(MidiEvent{0xB3, 120, 0, 3, }, handler);
}

TEST_F(MidiProcessorTest, NoteRangeHandling) {
    // Test preset notes (24-33) - should be handled differently from keyboard notes
    // Note: This would require program change functionality to be implemented
    
    // Test keyboard notes (36-96)
    EXPECT_CALL(handler, key_on(0, 0));  // Note 36 - 36 = 0 (lowest)
    midi_env.processMidiEvent(MidiEvent{0x90, 36, 64, 0, }, handler);
    
    EXPECT_CALL(handler, key_on(60, 0)); // Note 96 - 36 = 60 (highest)
    midi_env.processMidiEvent(MidiEvent{0x90, 96, 64, 0, }, handler);
    
    // Test notes outside range (>96) - should be ignored (no expectation)
    midi_env.processMidiEvent(MidiEvent{0x90, 127, 64, 0, }, handler);
}

TEST_F(MidiProcessorTest, VelocityAsNoteOnOff) {
    // Test velocity-based note on/off as described in MIDI.md
    // "Velocity used as binary trigger (0 = note off, >0 = note on)"
    
    // Note on with velocity > 0
    EXPECT_CALL(handler, key_on(24, 0));
    midi_env.processMidiEvent(MidiEvent{0x90, 60, 1, 0, }, handler);
    
    // Note on with velocity = 0 should be treated as note off
    EXPECT_CALL(handler, key_off(24, 0));
    midi_env.processMidiEvent(MidiEvent{0x90, 60, 0, 0, }, handler);
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