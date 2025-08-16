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


#include <pthread.h>
#include <jack/midiport.h>
#include "jack_audio.h"
#include "messages.h"
#include "midi_processor.h"


JackAudio::JackAudio (const char *appname, Lfq_u32 *qnote, Lfq_u32 *qcomm) :
    AudioBackend (appname, qnote, qcomm),
    _jack_handle (0),
    _jack_midipt (0),
    _qmidi (0),
    _relpri (0),
    _jmidi_count (0),
    _jmidi_index (0),
    _jmidi_pdata (0)
{
    for (int i = 0; i < 8; i++) _jack_opport [i] = 0;
}


JackAudio::~JackAudio (void)
{
    if (_jack_handle) close_jack ();
}


void JackAudio::init_jack (const char *server, bool bform, Lfq_u8 *qmidi)
{
    int                 i;
    int                 opts;
    jack_status_t       stat;
    struct sched_param  spar;
    const char          **p;

    _bform = bform;
    _qmidi = qmidi;

    opts = JackNoStartServer;
    if (server) opts |= JackServerName;
    _jack_handle = jack_client_open (_appname, (jack_options_t) opts, &stat, server);
    if (!_jack_handle)
    {
        fprintf (stderr, "Error: can't connect to JACK\n");
        exit (1);
    }
    _appname = jack_get_client_name (_jack_handle);

    jack_set_process_callback (_jack_handle, jack_static_callback, (void *)this);
    jack_on_shutdown (_jack_handle, jack_static_shutdown, (void *)this);

    if (_bform)
    {
        _nplay = 4;
        p = _ports_ambis1;
    }
    else
    {
        _nplay = 2;
        p = _ports_stereo;
    }

    for (i = 0; i < _nplay; i++)
    {
        _jack_opport [i] = jack_port_register (_jack_handle, p [i], JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
        if (!_jack_opport [i])
        {
            fprintf (stderr, "Error: can't create the '%s' jack port\n", p [i]);
            exit (1);
        }
    }

    if (_qmidi)
    {
        _jack_midipt = jack_port_register (_jack_handle, "Midi/in", JACK_DEFAULT_MIDI_TYPE, JackPortIsInput, 0);
        if (!_jack_midipt)
        {
            fprintf (stderr, "Error: can't create the 'Midi/in' jack port\n");
            exit (1);
        }
    }

    _fsamp = jack_get_sample_rate (_jack_handle);
    _fsize = jack_get_buffer_size (_jack_handle);
    init_audio ();

    if (jack_activate (_jack_handle))
    {
        fprintf(stderr, "Error: can't activate JACK.");
        exit (1);
    }

    pthread_getschedparam (jack_client_thread_id (_jack_handle), &_policy, &spar);
    _abspri = spar.sched_priority;
    _relpri = spar.sched_priority -  sched_get_priority_max (_policy);
}


void JackAudio::start (void)
{
    M_audio_info  *M;
    int           i;

    M = new M_audio_info ();
    M->_nasect = _nasect;
    M->_fsamp  = _fsamp;
    M->_fsize  = _fsize;
    M->_instrpar = _audiopar;
    for (i = 0; i < _nasect; i++) M->_asectpar [i] = _asectp [i]->get_apar ();
    send_event(TO_MODEL, M);

    // Send MIDI info to model thread since JACK handles MIDI in audio thread
    if (_qmidi)
    {
        M_midi_info *MI = new M_midi_info ();
        MI->_client = 0;  // JACK doesn't use client ID like ALSA
        MI->_ipport = 0;  // JACK doesn't use port ID like ALSA
        memcpy (MI->_chbits, _midimap, 16 * sizeof (uint16_t));
        send_event (TO_MODEL, MI);
    }
}


void JackAudio::close_jack (void)
{
    jack_deactivate (_jack_handle);
    for (int i = 0; i < _nplay; i++) 
    {
        if (_jack_opport [i]) jack_port_unregister(_jack_handle, _jack_opport [i]);
    }
    if (_jack_midipt) jack_port_unregister(_jack_handle, _jack_midipt);
    jack_client_close (_jack_handle);
    _jack_handle = 0;
}


void JackAudio::jack_shutdown (void)
{
    _running = false;
    send_event (EV_EXIT, 1);
}


int JackAudio::jack_callback (jack_nframes_t nframes)
{
    int i;

    proc_queue (_qnote);
    proc_queue (_qcomm);
    proc_keys1 ();
    proc_keys2 ();
    for (i = 0; i < _nplay; i++) _outbuf [i] = (float *)(jack_port_get_buffer (_jack_opport [i], nframes));
    
    if (_qmidi)
    {
        _jmidi_pdata = jack_port_get_buffer (_jack_midipt, nframes);
        _jmidi_count = jack_midi_get_event_count (_jmidi_pdata);
        _jmidi_index = 0;
    }
    else
    {
        _jmidi_pdata = 0;
    }
    
    proc_synth (nframes);
    
    proc_mesg ();
    return 0;
}


void JackAudio::proc_midi_during_synth (int frame_time)
{
    if (_jmidi_pdata)
    {
        proc_jmidi (frame_time);
        proc_keys1 (); // Update keys after MIDI processing
    }
}


void JackAudio::proc_jmidi (int tmax)
{
    jack_midi_event_t   E;

    // Read and process MIDI commands from the JACK port using MidiProcessor
    while (   (jack_midi_event_get (&E, _jmidi_pdata, _jmidi_index) == 0)
           && (E.time < (jack_nframes_t) tmax))
    {
        if (E.size >= 1)
        {
            uint8_t status = E.buffer[0];
            uint8_t data1 = E.size >= 2 ? E.buffer[1] : 0;
            uint8_t data2 = E.size >= 3 ? E.buffer[2] : 0;
            uint8_t channel = status & 0x0F;

            // Use MidiProcessor for common MIDI logic
            MidiProcessor::process_midi_event(status, data1, data2, channel,
                                            _midimap, this, _qnote, _qmidi);
        }
        _jmidi_index++;
    }
}


void JackAudio::jack_static_shutdown (void *arg)
{
    return ((JackAudio *) arg)->jack_shutdown ();
}


int JackAudio::jack_static_callback (jack_nframes_t nframes, void *arg)
{
    return ((JackAudio *) arg)->jack_callback (nframes);
}


void JackAudio::thr_main (void)
{
    // JACK uses callback-based processing, no thread main needed
    // This method should not be called for JACK backends
}