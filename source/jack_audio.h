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


#ifndef __JACK_AUDIO_H
#define __JACK_AUDIO_H


#include <jack/jack.h>
#include "audio_backend.h"
#include "lfqueue.h"


class JackAudio : public AudioBackend
{
public:

    JackAudio (const char *appname, Lfq_u32 *qnote, Lfq_u32 *qcomm);
    virtual ~JackAudio (void);

    // Initialize JACK audio
    void init_jack (const char *server, bool bform, Lfq_u8 *qmidi);
    
    // AudioBackend interface implementation
    void start (void) override;
    int  relpri (void) const override { return _relpri; }

protected:
    
    // Override MIDI processing during synthesis
    void proc_midi_during_synth (int frame_time) override;

private:

    void close_jack (void);
    void jack_shutdown (void);
    int  jack_callback (jack_nframes_t);
    void proc_jmidi (int);

    static void jack_static_shutdown (void *);
    static int  jack_static_callback (jack_nframes_t, void *);

    jack_client_t  *_jack_handle;
    jack_port_t    *_jack_opport [8];
    jack_port_t    *_jack_midipt;
    Lfq_u8         *_qmidi;
    int             _relpri;
    int             _jmidi_count;
    int             _jmidi_index;
    void           *_jmidi_pdata;
};


#endif