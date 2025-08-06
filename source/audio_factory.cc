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


#include "audio_factory.h"

#ifdef __linux__
#include "alsa_audio.h"
#endif

#ifdef HAVE_JACK
#include "jack_audio.h"
#endif


AudioBackend* AudioFactory::create(AudioType type, const char* appname, 
                                   Lfq_u32* note_queue, Lfq_u32* comm_queue)
{
    switch (type)
    {
#ifdef __linux__
    case AudioType::ALSA:
        return new AlsaAudio(appname, note_queue, comm_queue);
#endif

#ifdef HAVE_JACK
    case AudioType::JACK:
        return new JackAudio(appname, note_queue, comm_queue);
#endif

    default:
        return nullptr;
    }
}


bool AudioFactory::is_available(AudioType type)
{
    switch (type)
    {
#ifdef __linux__
    case AudioType::ALSA:
        return true;
#endif

#ifdef HAVE_JACK
    case AudioType::JACK:
        return true;
#endif

    default:
        return false;
    }
}