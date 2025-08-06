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


#ifndef __MIDI_BACKEND_H
#define __MIDI_BACKEND_H


#include <clthreads.h>
#include "lfqueue.h"
#include "midi_processor.h"
#include "messages.h"


class MidiBackend : public A_thread, public MidiProcessor::Handler
{
public:

    MidiBackend (const char *name, Lfq_u32 *qnote, Lfq_u8 *qmidi, uint16_t *midimap, const char *appname);
    virtual ~MidiBackend (void);

    // Pure virtual interface - must be implemented by subclasses
    virtual void terminate (void) = 0;

    // MidiProcessor::Handler implementation (base class provides no-op)
    void key_on(int note, int keyboard) override;
    void key_off(int note, int keyboard) override;
    void all_sound_off() override;
    void all_notes_off(int keyboard) override;
    void hold_pedal(int keyboard, bool on) override;

protected:

    // Pure virtual methods for platform-specific implementation
    virtual void open_midi (void) = 0;
    virtual void close_midi (void) = 0;
    virtual void proc_midi (void) = 0;

    // Common message processing
    void proc_mesg (ITC_mesg *M);

    // Common member variables
    Lfq_u32        *_qnote;
    Lfq_u8         *_qmidi;
    uint16_t       *_midimap;
    const char     *_appname;
    int             _client;
    int             _ipport;
    int             _opport;

private:

    virtual void thr_main (void) override;
};


#endif