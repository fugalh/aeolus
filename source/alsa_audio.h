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


#ifndef __ALSA_AUDIO_H
#define __ALSA_AUDIO_H


#include <zita-alsa-pcmi.h>
#include "audio_backend.h"


class AlsaAudio : public AudioBackend
{
public:

    AlsaAudio (const char *appname, Lfq_u32 *qnote, Lfq_u32 *qcomm);
    virtual ~AlsaAudio (void);

    // Initialize ALSA audio
    void init_alsa (const char *device, int fsamp, int fsize, int nfrag);
    
    // AudioBackend interface implementation
    void start (void) override;
    int  relpri (void) const override { return _relpri; }

private:

    void close_alsa (void);
    virtual void thr_main (void) override;

    Alsa_pcmi      *_alsa_handle;
    int             _relpri;
};


#endif