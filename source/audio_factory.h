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


#ifndef __AUDIO_FACTORY_H
#define __AUDIO_FACTORY_H


#include "audio_backend.h"
#include "lfqueue.h"


enum class AudioType
{
    ALSA,
    JACK
};


class AudioFactory
{
public:

    // Create appropriate audio backend based on type
    static AudioBackend* create(AudioType type, const char* appname, 
                               Lfq_u32* note_queue, Lfq_u32* comm_queue);

    // Helper to determine available backend types
    static bool is_available(AudioType type);
};


#endif