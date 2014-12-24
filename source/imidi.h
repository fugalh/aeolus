// ----------------------------------------------------------------------------
//
//  Copyright (C) 2003-2013 Fons Adriaensen <fons@linuxaudio.org>
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


#ifndef __IMIDI_H
#define __IMIDI_H


#include <stdlib.h>
#include <stdio.h>
#include <clthreads.h>
#ifdef __linux__
#include <alsa/asoundlib.h>
#endif
#ifdef __APPLE__
#include <CoreMIDI/MIDIServices.h>
#endif
#include "lfqueue.h"
#include "messages.h"



class Imidi : public A_thread
{
public:

    Imidi (Lfq_u32 *qnote, Lfq_u8 *qmidi, uint16_t *midimap, const char *appname);
    virtual ~Imidi (void);

    void terminate (void);
#ifdef __APPLE__
    void coremidi_proc (const MIDIPacketList *pktlist, void *refCon, void *connRefCon);
#endif

private:

    virtual void thr_main (void);

    void open_midi (void);
    void close_midi (void);
    void proc_midi (void);
    void proc_mesg (ITC_mesg *M);

    Lfq_u32        *_qnote; 
    Lfq_u8         *_qmidi; 
    uint16_t       *_midimap;
    const char     *_appname;
#ifdef __linux__
    snd_seq_t      *_handle;
#endif
#ifdef __APPLE__
    MIDIClientRef   _handle;
#endif
    int             _client;
    int             _ipport;
    int             _opport;
};


#endif
