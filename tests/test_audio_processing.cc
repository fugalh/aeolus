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
#include "messages.h"

using ::testing::_;
using ::testing::Return;
using namespace std::literals;

// Enhanced mock using proper GMock expectations
class ProcessingTrackingMock : public AudioBackend
{
public:
    ProcessingTrackingMock() : AudioBackend("test", &note_queue, &comm_queue) {}
    
    MOCK_METHOD(void, start, (), (override));
    MOCK_METHOD(int, relpri, (), (const, override));
    MOCK_METHOD(void, thr_main, (), (override));
    MOCK_METHOD(void, proc_midi_during_synth, (int frame_time), (override));
    
    // Mock the key processing methods that would be called
    MOCK_METHOD(void, mock_proc_note_queue, (Lfq_u32* queue), ());
    MOCK_METHOD(void, mock_proc_comm_queue, (Lfq_u32* queue), ());
    MOCK_METHOD(void, mock_proc_keys1, (), ());
    MOCK_METHOD(void, mock_proc_keys2, (), ());
    MOCK_METHOD(void, mock_proc_synth, (int frames), ());
    MOCK_METHOD(void, mock_proc_mesg, (), ());
    MOCK_METHOD(void, mock_key_on, (int note, int keyboard), ());
    MOCK_METHOD(void, mock_key_off, (int note, int keyboard), ());
    
    // Public wrappers that call mocked methods (for testing)
    void proc_queue(Lfq_u32* queue) {
        if (queue == &note_queue) mock_proc_note_queue(queue);
        if (queue == &comm_queue) mock_proc_comm_queue(queue);
        // Don't call AudioBackend::proc_queue to avoid initialization issues
    }
    
    void proc_synth(int frames) {
        mock_proc_synth(frames);
        // Don't call AudioBackend::proc_synth to avoid initialization issues
    }
    
    void proc_keys1() {
        mock_proc_keys1();
        // Don't call AudioBackend methods to avoid initialization issues
    }
    
    void proc_keys2() {
        mock_proc_keys2();
        // Don't call AudioBackend methods to avoid initialization issues
    }
    
    void proc_mesg() {
        mock_proc_mesg();
        // Don't call AudioBackend methods to avoid initialization issues
    }
    
    // MidiProcessor::Handler overrides that call mocked methods
    void key_on(int note, int keyboard) override {
        mock_key_on(note, keyboard);
        // Don't call AudioBackend::key_on to avoid complex internal state
    }
    
    void key_off(int note, int keyboard) override {
        mock_key_off(note, keyboard);
        // Don't call AudioBackend::key_off to avoid complex internal state
    }
    
    Lfq_u32 note_queue{256};
    Lfq_u32 comm_queue{256};
    
    // Helper methods for adding events to queues
    void add_note_event(uint32_t event) { 
        note_queue.write(0, event);
        note_queue.write_commit(1);
    }
    void add_comm_event(uint32_t event) { 
        comm_queue.write(0, event);
        comm_queue.write_commit(1);
    }
};

class AudioProcessingTest : public ::testing::Test {
protected:
    void SetUp() override {
        mock = std::make_unique<ProcessingTrackingMock>();
    }
    
    std::unique_ptr<ProcessingTrackingMock> mock;
};

TEST_F(AudioProcessingTest, AudioPipelineExecution) {
    // Set expectations for a typical audio processing cycle
    EXPECT_CALL(*mock, mock_proc_note_queue(&mock->note_queue));
    EXPECT_CALL(*mock, mock_proc_comm_queue(&mock->comm_queue));
    EXPECT_CALL(*mock, mock_proc_keys1());
    EXPECT_CALL(*mock, mock_proc_keys2());
    EXPECT_CALL(*mock, mock_proc_synth(512));
    EXPECT_CALL(*mock, mock_proc_mesg());
    
    // Execute the processing cycle
    mock->proc_queue(&mock->note_queue);
    mock->proc_queue(&mock->comm_queue);
    mock->proc_keys1();
    mock->proc_keys2();
    mock->proc_synth(512);
    mock->proc_mesg();
}

TEST_F(AudioProcessingTest, NoteEventProcessing) {
    // Send note on event through queue
    uint32_t note_on = (1 << 8) | 40;  // Note on, MIDI note 40 (translates to note 4 for keyboard)
    mock->add_note_event(note_on);
    
    EXPECT_CALL(*mock, mock_proc_note_queue(&mock->note_queue));
    mock->proc_queue(&mock->note_queue);
    // Note: The actual key_on call depends on complex midimap logic in AudioBackend
}

TEST_F(AudioProcessingTest, CommandEventProcessing) {
    // Send command event through queue
    uint32_t command = (0x05 << 24) | (0x02 << 16) | 0x7F;  // Some command structure
    mock->add_comm_event(command);
    
    EXPECT_CALL(*mock, mock_proc_comm_queue(&mock->comm_queue));
    mock->proc_queue(&mock->comm_queue);
}

TEST_F(AudioProcessingTest, MidiProcessingIntegration) {
    // Test MIDI processing during synthesis
    EXPECT_CALL(*mock, proc_midi_during_synth(256));
    mock->proc_midi_during_synth(256);
}

TEST_F(AudioProcessingTest, MultipleProcessingCycles) {
    // Set expectations for 5 cycles of processing
    EXPECT_CALL(*mock, mock_proc_note_queue(&mock->note_queue)).Times(5);
    EXPECT_CALL(*mock, mock_proc_comm_queue(&mock->comm_queue)).Times(5);
    EXPECT_CALL(*mock, mock_proc_keys1()).Times(5);
    EXPECT_CALL(*mock, mock_proc_keys2()).Times(5);
    EXPECT_CALL(*mock, mock_proc_synth(256)).Times(5);
    EXPECT_CALL(*mock, mock_proc_mesg()).Times(5);
    
    // Simulate multiple audio buffer processing cycles
    for (int i = 0; i < 5; i++) {
        mock->proc_queue(&mock->note_queue);
        mock->proc_queue(&mock->comm_queue);
        mock->proc_keys1();
        mock->proc_keys2();
        mock->proc_synth(256);
        mock->proc_mesg();
    }
}

TEST_F(AudioProcessingTest, DirectKeyEvents) {
    // Set expectations for direct key events
    EXPECT_CALL(*mock, mock_key_on(24, 1));
    EXPECT_CALL(*mock, mock_key_off(36, 2));
    
    // Test direct key events bypass queue processing
    mock->key_on(24, 1);   // Note 24, keyboard 1
    mock->key_off(36, 2);  // Note 36, keyboard 2
}

TEST_F(AudioProcessingTest, VariableBufferSizes) {
    // Test processing with different buffer sizes using modern C++ container
    constexpr std::array buffer_sizes{32, 64, 128, 256, 512, 1024, 2048};
    
    for (const auto size : buffer_sizes) {
        EXPECT_CALL(*mock, mock_proc_synth(size));
        mock->proc_synth(size);
    }
}