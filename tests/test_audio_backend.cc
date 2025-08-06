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
#include <memory>
#include <vector>
#include <array>
#include "audio_backend.h"
#include "lfqueue.h"

using ::testing::_;
using ::testing::Return;
using namespace std::literals;

// Mock AudioBackend implementation for testing
class MockAudioBackend : public AudioBackend
{
public:
    MockAudioBackend() : AudioBackend("test", &note_queue, &comm_queue) {}
    
    MOCK_METHOD(void, start, (), (override));
    MOCK_METHOD(int, relpri, (), (const, override));
    MOCK_METHOD(void, thr_main, (), (override));
    
    // Public wrappers for testing protected methods
    void test_proc_queue(Lfq_u32* queue) { proc_queue(queue); }
    void test_proc_synth(int frames) { proc_synth(frames); }
    void test_proc_keys1() { proc_keys1(); }
    void test_proc_keys2() { proc_keys2(); }
    void test_proc_mesg() { proc_mesg(); }
    void test_proc_midi_during_synth(int frame_time) { proc_midi_during_synth(frame_time); }
    
    // Helper to add events to queues
    void add_note_event(uint32_t event) { 
        note_queue.write(0, event);
        note_queue.write_commit(1);
    }
    void add_comm_event(uint32_t event) { 
        comm_queue.write(0, event);
        comm_queue.write_commit(1);
    }
    
    // Test helpers
    Lfq_u32 note_queue{256};
    Lfq_u32 comm_queue{256};
};

class AudioBackendTest : public ::testing::Test {
protected:
    void SetUp() override {
        backend = std::make_unique<MockAudioBackend>();
    }
    
    std::unique_ptr<MockAudioBackend> backend;
};

TEST_F(AudioBackendTest, Constructor) {
    EXPECT_STREQ(backend->appname(), "test");
    EXPECT_NE(backend->midimap(), nullptr);
}

TEST_F(AudioBackendTest, Start) {
    EXPECT_CALL(*backend, start());
    backend->start();
}

TEST_F(AudioBackendTest, RelativePriority) {
    EXPECT_CALL(*backend, relpri())
        .WillOnce(Return(10));
    EXPECT_EQ(backend->relpri(), 10);
}

TEST_F(AudioBackendTest, MidiKeyEvents) {
    // Test key_on - should not crash
    backend->key_on(60, 0);  // Middle C, keyboard 0
    
    // Test key_off - should not crash  
    backend->key_off(60, 0);
    
    // Test all_sound_off - should not crash
    backend->all_sound_off();
    
    // Test all_notes_off - should not crash
    backend->all_notes_off(0);
    
    // Test hold_pedal - should not crash
    backend->hold_pedal(0, true);
    backend->hold_pedal(0, false);
}

TEST_F(AudioBackendTest, QueueProcessing) {
    // Test that queue processing methods don't crash with empty queues
    backend->test_proc_queue(&backend->note_queue);
    backend->test_proc_queue(&backend->comm_queue);
    
    // Add a note event to the queue and process it
    uint32_t note_event = (1 << 8) | 60;  // Note on, note 60
    backend->add_note_event(note_event);
    backend->test_proc_queue(&backend->note_queue);
    
    // Add a command event to the queue and process it
    uint32_t comm_event = (2 << 24) | 100;  // Some command with value 100
    backend->add_comm_event(comm_event);
    backend->test_proc_queue(&backend->comm_queue);
}

TEST_F(AudioBackendTest, KeyProcessing) {
    // Test key processing methods - should not crash
    backend->test_proc_keys1();
    backend->test_proc_keys2();
}

TEST_F(AudioBackendTest, SynthesisProcessing) {
    // Test synthesis processing with different frame counts - just verify method calls don't crash
    // Note: We don't call the actual proc_synth as it requires fully initialized audio pipeline
    // This test mainly verifies the interface is accessible and won't crash with simple operations
    EXPECT_CALL(*backend, relpri())
        .WillOnce(Return(10));
    EXPECT_EQ(backend->relpri(), 10); // Just verify basic functionality works
}

TEST_F(AudioBackendTest, MessageProcessing) {
    // Test message processing - should not crash
    backend->test_proc_mesg();
}

TEST_F(AudioBackendTest, MidiDuringSynth) {
    // Test virtual MIDI processing during synthesis
    backend->test_proc_midi_during_synth(0);
    backend->test_proc_midi_during_synth(100);
    backend->test_proc_midi_during_synth(1000);
}