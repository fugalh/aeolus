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
#include "audio_backend.h"
#include "lfqueue.h"

// Mock AudioBackend implementation for testing
class MockAudioBackend : public AudioBackend
{
public:
    MockAudioBackend() : AudioBackend("test", &note_queue, &comm_queue) {}
    
    void start() override { started = true; }
    int relpri() const override { return 10; }
    
    // A_thread pure virtual method - empty implementation for testing
    void thr_main() override {}
    
    // Test helpers
    bool started = false;
    Lfq_u32 note_queue{256};
    Lfq_u32 comm_queue{256};
};

class AudioBackendTest : public ::testing::Test {
protected:
    void SetUp() override {
        backend = new MockAudioBackend();
    }
    
    void TearDown() override {
        delete backend;
    }
    
    MockAudioBackend* backend;
};

TEST_F(AudioBackendTest, Constructor) {
    EXPECT_STREQ(backend->appname(), "test");
    EXPECT_NE(backend->midimap(), nullptr);
    EXPECT_FALSE(backend->started);
}

TEST_F(AudioBackendTest, Start) {
    backend->start();
    EXPECT_TRUE(backend->started);
}

TEST_F(AudioBackendTest, RelativePriority) {
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