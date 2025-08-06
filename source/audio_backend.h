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


#ifndef __AUDIO_BACKEND_H
#define __AUDIO_BACKEND_H


#include <stdlib.h>
#include <clthreads.h>
#include "asection.h"
#include "division.h"
#include "lfqueue.h"
#include "reverb.h"
#include "global.h"
#include "midi_processor.h"


class AudioBackend : public A_thread, public MidiProcessor::Handler
{
public:

    AudioBackend (const char *appname, Lfq_u32 *qnote, Lfq_u32 *qcomm);
    virtual ~AudioBackend (void);
    
    // Pure virtual methods - must be implemented by subclasses
    virtual void start (void) = 0;
    virtual int  relpri (void) const = 0;

    // Common interface methods
    const char  *appname (void) const { return _appname; }
    uint16_t    *midimap (void) const { return (uint16_t *) _midimap; }
    int  policy (void) const { return _policy; }
    int  abspri (void) const { return _abspri; }

    // MidiProcessor::Handler implementation
    void key_on(int note, int keyboard) override;
    void key_off(int note, int keyboard) override;
    void all_sound_off() override;
    void all_notes_off(int keyboard) override;
    void hold_pedal(int keyboard, bool on) override;

protected:

    enum { VOLUME, REVSIZE, REVTIME, STPOSIT };

    // Common initialization shared by all backends
    void init_audio (void);
    
    // Common audio processing methods - now MIDI-agnostic
    void proc_queue (Lfq_u32 *);
    void proc_synth (int);
    void proc_keys1 (void);
    void proc_keys2 (void);
    void proc_mesg (void);

    // Virtual method for backend-specific MIDI processing during synthesis
    virtual void proc_midi_during_synth (int frame_time) {}

    // Common member variables
    const char     *_appname;
    uint16_t        _midimap [16];
    Lfq_u32        *_qnote;
    Lfq_u32        *_qcomm;
    volatile bool   _running;
    int             _policy;
    int             _abspri;
    int             _hold;
    bool            _bform;
    int             _nplay;
    unsigned int    _fsamp;
    unsigned int    _fsize;
    int             _nasect;
    int             _ndivis;
    Asection       *_asectp [NASECT];
    Division       *_divisp [NDIVIS];
    Reverb          _reverb;
    float          *_outbuf [8];
    uint16_t        _keymap [NNOTES];
    Fparm           _audiopar [4];
    float           _revsize;
    float           _revtime;

    static const char *_ports_stereo [2];
    static const char *_ports_ambis1 [4];

private:

    // Common key handling methods
    void key_off_internal (int i, int b)
    {
        _keymap [i] &= ~b;
        _keymap [i] |= KMAP_SET;
    }

    void key_on_internal (int i, int b)
    {
        _keymap [i] |= b | KMAP_SET;
    }

    void cond_key_off (int m, int b)
    {
        int       i;
        uint16_t  *p;

        for (i = 0, p = _keymap; i < NNOTES; i++, p++)
        {
            if (*p & m)
            {
                *p &= ~b;
                *p |= KMAP_SET;
            }
        }
    }

    void cond_key_on (int m, int b)
    {
        int       i;
        uint16_t  *p;

        for (i = 0, p = _keymap; i < NNOTES; i++, p++)
        {
            if (*p & m)
            {
                *p |= b | KMAP_SET;
            }
        }
    }
};


#endif