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


#include "midi_backend.h"


MidiBackend::MidiBackend (const char *name, Lfq_u32 *qnote, Lfq_u8 *qmidi, uint16_t *midimap, const char *appname) :
    A_thread (name),
    _qnote (qnote),
    _qmidi (qmidi),
    _midimap (midimap),
    _appname (appname),
    _client (0),
    _ipport (0),
    _opport (0)
{
}


MidiBackend::~MidiBackend (void)
{
}


void MidiBackend::thr_main (void)
{
    open_midi ();
    proc_midi ();
    close_midi ();
    send_event (EV_EXIT, 1);
}


void MidiBackend::proc_mesg (ITC_mesg *M)
{
    // Base implementation - subclasses can override
    // Default behavior is to ignore all messages
    M->recover ();
}


// MidiProcessor::Handler implementation - queue-based actions only
// Direct key manipulation is handled by audio backend
void MidiBackend::key_on(int note, int keyboard)
{
    // Send note on command to audio thread via queue
    if (_qnote && _qnote->write_avail() > 0)
    {
        _qnote->write(0, (1 << 24) | (note << 16) | keyboard);
        _qnote->write_commit(1);
    }
}


void MidiBackend::key_off(int note, int keyboard)
{
    // Send note off command to audio thread via queue
    if (_qnote && _qnote->write_avail() > 0)
    {
        _qnote->write(0, (0 << 24) | (note << 16) | keyboard);
        _qnote->write_commit(1);
    }
}


void MidiBackend::all_sound_off()
{
    // Send all sound off command to audio thread
    if (_qnote && _qnote->write_avail() > 0)
    {
        _qnote->write(0, (2 << 24) | NKEYBD);  // All keyboards
        _qnote->write_commit(1);
    }
}


void MidiBackend::all_notes_off(int keyboard)
{
    // Send all notes off command for specific keyboard
    if (_qnote && _qnote->write_avail() > 0)
    {
        _qnote->write(0, (2 << 24) | keyboard);
        _qnote->write_commit(1);
    }
}


void MidiBackend::hold_pedal(int keyboard, bool on)
{
    // Send hold pedal command to audio thread
    if (_qnote && _qnote->write_avail() > 0)
    {
        uint8_t cmd = on ? 9 : 8;
        _qnote->write(0, (cmd << 24) | keyboard);
        _qnote->write_commit(1);
    }
}