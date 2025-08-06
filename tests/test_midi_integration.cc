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

#include <gtest/gtest.h>
#include "midi_processor.h"
#include "midi_backend.h"
#include "lfqueue.h"
#include "global.h"

// Mock MIDI backend for integration testing
class MockMidiBackend : public MidiBackend
{
public:
    MockMidiBackend() : MidiBackend("test", &note_queue, &midi_queue, midimap, "test_app") {}
    
    // Pure virtual implementations
    void terminate() override {}
    
    // Test interface methods
    int open() { 
        opened = true;
        return 0; 
    }
    
    void close() { 
        opened = false;
    }
    
private:
    // Pure virtual implementations for protected methods
    void open_midi() override {}
    void close_midi() override {}
    void proc_midi() override {}
    
public:
    // Test state
    bool opened = false;
    Lfq_u32 note_queue{256};
    Lfq_u8 midi_queue{1024};
    uint16_t midimap[16];
};

// Enhanced MIDI handler for integration testing
class IntegrationMidiHandler : public MidiProcessor::Handler
{
public:
    void key_on(int note, int keyboard) override {
        key_events.push_back({true, note, keyboard});
        last_note_on = note;
        last_keyboard_on = keyboard;
    }
    
    void key_off(int note, int keyboard) override {
        key_events.push_back({false, note, keyboard});
        last_note_off = note;
        last_keyboard_off = keyboard;
    }
    
    void all_sound_off() override { 
        all_sound_off_count++;
    }
    
    void all_notes_off(int keyboard) override { 
        all_notes_off_count++;
        all_notes_off_keyboard = keyboard;
    }
    
    void hold_pedal(int keyboard, bool on) override { 
        hold_events.push_back({keyboard, on});
        last_hold_keyboard = keyboard;
        last_hold_state = on;
    }
    
    // Test state
    struct KeyEvent {
        bool on;
        int note;
        int keyboard;
    };
    
    struct HoldEvent {
        int keyboard;
        bool on;
    };
    
    std::vector<KeyEvent> key_events;
    std::vector<HoldEvent> hold_events;
    
    int last_note_on = -1;
    int last_note_off = -1;
    int last_keyboard_on = -1;
    int last_keyboard_off = -1;
    int all_sound_off_count = 0;
    int all_notes_off_count = 0;
    int all_notes_off_keyboard = -1;
    int last_hold_keyboard = -1;
    bool last_hold_state = false;
    
    void reset() {
        key_events.clear();
        hold_events.clear();
        last_note_on = last_note_off = -1;
        last_keyboard_on = last_keyboard_off = -1;
        all_sound_off_count = all_notes_off_count = 0;
        all_notes_off_keyboard = last_hold_keyboard = -1;
        last_hold_state = false;
    }
};

class MidiIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        backend = new MockMidiBackend();
        handler.reset();
        
        // Initialize midimap with proper flags for all channels
        for (int i = 0; i < 16; i++) {
            backend->midimap[i] = i | (1 << 12) | (4 << 12);  // Enable keyboard and control flags
        }
    }
    
    void TearDown() override {
        delete backend;
    }
    
    MockMidiBackend* backend;
    IntegrationMidiHandler handler;
};

TEST_F(MidiIntegrationTest, BackendLifecycle) {
    EXPECT_FALSE(backend->opened);
    
    backend->open();
    EXPECT_TRUE(backend->opened);
    
    backend->close();
    EXPECT_FALSE(backend->opened);
}

TEST_F(MidiIntegrationTest, ComplexMidiSequence) {
    // Simulate a complex MIDI sequence: chord, sustain, release
    std::vector<std::tuple<uint8_t, uint8_t, uint8_t, uint8_t>> midi_events = {
        // Play C major chord
        {0x90, 60, 64, 0},  // Note on C4
        {0x90, 64, 64, 0},  // Note on E4  
        {0x90, 67, 64, 0},  // Note on G4
        
        // Hold sustain pedal
        {0xB0, 64, 127, 0}, // Sustain pedal on
        
        // Release chord notes
        {0x80, 60, 0, 0},   // Note off C4
        {0x80, 64, 0, 0},   // Note off E4
        {0x80, 67, 0, 0},   // Note off G4
        
        // Release sustain pedal
        {0xB0, 64, 0, 0},   // Sustain pedal off
    };
    
    for (auto& event : midi_events) {
        MidiProcessor::process_midi_event(
            std::get<0>(event), std::get<1>(event), std::get<2>(event), std::get<3>(event),
            backend->midimap, &handler, &backend->note_queue, &backend->midi_queue);
    }
    
    // Verify we got the expected key events
    EXPECT_EQ(handler.key_events.size(), 6);  // 3 note on + 3 note off
    EXPECT_EQ(handler.hold_events.size(), 2); // Sustain on + off
    
    // Check chord notes were processed (MIDI notes 60,64,67 -> notes 24,28,31)
    EXPECT_EQ(handler.key_events[0].note, 24); // C4 -> note 24
    EXPECT_EQ(handler.key_events[1].note, 28); // E4 -> note 28
    EXPECT_EQ(handler.key_events[2].note, 31); // G4 -> note 31
    
    // Check sustain pedal events
    EXPECT_TRUE(handler.hold_events[0].on);   // Sustain on
    EXPECT_FALSE(handler.hold_events[1].on);  // Sustain off
}

TEST_F(MidiIntegrationTest, MultiChannelProcessing) {
    // Test MIDI events on different channels
    handler.reset();
    
    // Channel 0: Note on
    MidiProcessor::process_midi_event(0x90, 60, 64, 0, backend->midimap, &handler, &backend->note_queue, &backend->midi_queue);
    
    // Channel 5: Note on  
    MidiProcessor::process_midi_event(0x95, 72, 80, 5, backend->midimap, &handler, &backend->note_queue, &backend->midi_queue);
    
    // Channel 15: Note on
    MidiProcessor::process_midi_event(0x9F, 48, 100, 15, backend->midimap, &handler, &backend->note_queue, &backend->midi_queue);
    
    EXPECT_EQ(handler.key_events.size(), 3);
    
    // Verify different keyboards based on midimap
    EXPECT_EQ(handler.key_events[0].keyboard, 0);   // Channel 0 -> keyboard 0
    EXPECT_EQ(handler.key_events[1].keyboard, 5);   // Channel 5 -> keyboard 5  
    EXPECT_EQ(handler.key_events[2].keyboard, 15);  // Channel 15 -> keyboard 15
}

TEST_F(MidiIntegrationTest, ControllerMessages) {
    handler.reset();
    
    // Test different controllers on different channels
    std::vector<std::tuple<uint8_t, uint8_t, uint8_t, uint8_t>> controller_tests = {
        {0xB0, 64, 127, 0},  // Channel 0: Sustain on
        {0xB1, 64, 0, 1},    // Channel 1: Sustain off
        {0xB2, 120, 0, 2},   // Channel 2: All sound off
        {0xB3, 123, 0, 3},   // Channel 3: All notes off
        {0xB4, 1, 50, 4},    // Channel 4: Modulation wheel (should be ignored)
    };
    
    for (auto& event : controller_tests) {
        MidiProcessor::process_midi_event(
            std::get<0>(event), std::get<1>(event), std::get<2>(event), std::get<3>(event),
            backend->midimap, &handler, &backend->note_queue, &backend->midi_queue);
    }
    
    EXPECT_EQ(handler.hold_events.size(), 2);  // 2 sustain events
    EXPECT_EQ(handler.all_sound_off_count, 1);  // 1 all sound off
    EXPECT_EQ(handler.all_notes_off_count, 1);  // 1 all notes off
    
    // Check sustain pedal states
    EXPECT_TRUE(handler.hold_events[0].on);   // First sustain: on
    EXPECT_FALSE(handler.hold_events[1].on);  // Second sustain: off
    
    // Check keyboards
    EXPECT_EQ(handler.hold_events[0].keyboard, 0);
    EXPECT_EQ(handler.hold_events[1].keyboard, 1);
    EXPECT_EQ(handler.all_notes_off_keyboard, 3);
}

TEST_F(MidiIntegrationTest, EdgeCaseNoteNumbers) {
    handler.reset();
    
    // Test edge case note numbers
    std::vector<std::tuple<uint8_t, uint8_t>> test_notes = {
        {35, false},  // Below range (< 36) - should be ignored
        {36, true},   // Minimum valid note
        {60, true},   // Middle C
        {96, true},   // Maximum valid note  
        {97, false},  // Above range (> 96) - should be ignored
        {127, false}, // Maximum MIDI note - should be ignored
    };
    
    for (auto& note_test : test_notes) {
        uint8_t note = std::get<0>(note_test);
        bool should_process = std::get<1>(note_test);
        
        handler.reset();
        MidiProcessor::process_midi_event(0x90, note, 64, 0, backend->midimap, &handler, &backend->note_queue, &backend->midi_queue);
        
        if (should_process) {
            EXPECT_EQ(handler.key_events.size(), 1) << "Note " << (int)note << " should be processed";
            EXPECT_EQ(handler.key_events[0].note, note - 36) << "Note " << (int)note << " should map to " << (note - 36);
        } else {
            EXPECT_EQ(handler.key_events.size(), 0) << "Note " << (int)note << " should be ignored";
        }
    }
}

TEST_F(MidiIntegrationTest, VelocityHandling) {
    handler.reset();
    
    // Test different velocity values
    std::vector<uint8_t> velocities = {1, 64, 100, 127};
    
    for (uint8_t vel : velocities) {
        handler.reset();
        MidiProcessor::process_midi_event(0x90, 60, vel, 0, backend->midimap, &handler, &backend->note_queue, &backend->midi_queue);
        
        EXPECT_EQ(handler.key_events.size(), 1) << "Velocity " << (int)vel << " should produce key event";
        EXPECT_TRUE(handler.key_events[0].on) << "Velocity " << (int)vel << " should be note on";
    }
    
    // Test velocity 0 (should be note off)
    handler.reset();
    MidiProcessor::process_midi_event(0x90, 60, 0, 0, backend->midimap, &handler, &backend->note_queue, &backend->midi_queue);
    
    EXPECT_EQ(handler.key_events.size(), 1);
    EXPECT_FALSE(handler.key_events[0].on); // Velocity 0 should be note off
}

TEST_F(MidiIntegrationTest, QueueWriteOperations) {
    // Test that MIDI events are properly written to queues
    int initial_note_queue_avail = backend->note_queue.read_avail();
    int initial_midi_queue_avail = backend->midi_queue.read_avail();
    
    // Send a note event
    MidiProcessor::process_midi_event(0x90, 60, 64, 0, backend->midimap, &handler, &backend->note_queue, &backend->midi_queue);
    
    // Check that note queue has new data
    EXPECT_GT(backend->note_queue.read_avail(), initial_note_queue_avail);
    
    // Send a program change (should go to MIDI queue)
    MidiProcessor::process_midi_event(0xC0, 42, 0, 0, backend->midimap, &handler, &backend->note_queue, &backend->midi_queue);
    
    // Check that MIDI queue has new data
    EXPECT_GT(backend->midi_queue.read_avail(), initial_midi_queue_avail);
}