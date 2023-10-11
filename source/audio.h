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


#ifndef __AUDIO_H
#define __AUDIO_H


#include <stdlib.h>
#include <clthreads.h>
#ifdef __linux__
#include <zita-alsa-pcmi.h>
#endif
#include <jack/jack.h>
#include "asection.h"
#include "division.h"
#include "lfqueue.h"
#include "reverb.h"
#include "global.h"


class Audio : public A_thread
{
public:

    Audio (const char *jname, Lfq_u32 *qnote, Lfq_u32 *qcomm);
    virtual ~Audio (void);
#ifdef __linux__
    void  init_alsa (const char *device, int fsamp, int fsize, int nfrag);
#endif
    void  init_jack (const char *server, bool bform, Lfq_u8 *qmidi);
    void  start (void);

    const char  *appname (void) const { return _appname; }
    uint16_t    *midimap (void) const { return (uint16_t *) _midimap; }
    int  policy (void) const { return _policy; }
    int  abspri (void) const { return _abspri; }
    int  relpri (void) const { return _relpri; }

private:
   
    enum { VOLUME, REVSIZE, REVTIME, STPOSIT };

    void init_audio (void);
#ifdef __linux__
    void close_alsa (void);
#endif
    void close_jack (void);
    virtual void thr_main (void);
    void jack_shutdown (void);
    int  jack_callback (jack_nframes_t);
    void proc_jmidi (int);
    void proc_queue (Lfq_u32 *);
    void proc_synth (int);
    void proc_keys1 (void);
    void proc_keys2 (void);
    void proc_mesg (void);

    void key_off (int i, int b)
    {
        _keymap [i] &= ~b;
        _keymap [i] |= KMAP_SET;
    }

    void key_on (int i, int b)
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

    static void jack_static_shutdown (void *);
    static int  jack_static_callback (jack_nframes_t, void *);

    const char     *_appname;
    uint16_t        _midimap [16];
    Lfq_u32        *_qnote; 
    Lfq_u32        *_qcomm; 
    Lfq_u8         *_qmidi;
    volatile bool   _running;
#ifdef __linux__
    Alsa_pcmi      *_alsa_handle;
#endif
    jack_client_t  *_jack_handle;
    jack_port_t    *_jack_opport [8];
    jack_port_t    *_jack_midipt;
    int             _policy;
    int             _abspri;
    int             _relpri;
    int             _jmidi_count;
    int             _jmidi_index;
    void           *_jmidi_pdata;
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
};


#endif

