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


#include "alsa_audio.h"
#include "messages.h"


AlsaAudio::AlsaAudio (const char *appname, Lfq_u32 *qnote, Lfq_u32 *qcomm) :
    AudioBackend (appname, qnote, qcomm),
    _alsa_handle (0),
    _relpri (0)
{
}


AlsaAudio::~AlsaAudio (void)
{
    if (_alsa_handle) close_alsa ();
    // Clean up _outbuf arrays that were allocated with new[]
    for (int i = 0; i < 8; i++) delete[] _outbuf [i];
}


void AlsaAudio::init_alsa (const char *device, int fsamp, int fsize, int nfrag)
{
    _alsa_handle = new Alsa_pcmi (device, 0, 0, fsamp, fsize, nfrag);
    if (_alsa_handle->state () < 0)
    {
        fprintf (stderr, "Error: can't connect to ALSA.\n");
        exit (1);
    }
    _nplay = _alsa_handle->nplay ();
    _fsize = fsize;
    _fsamp = fsamp;
    if (_nplay > 2) _nplay = 2;
    init_audio ();
    for (int i = 0; i < _nplay; i++) _outbuf [i] = new float [fsize];
    _running = true;
    if (thr_start (_policy = SCHED_FIFO, _relpri = -20, 0))
    {
        fprintf (stderr, "Warning: can't run ALSA thread in RT mode.\n");
        if (thr_start (_policy = SCHED_OTHER, _relpri = 0, 0))
        {
            fprintf (stderr, "Error: can't create ALSA thread.\n");
            exit (1);
        }
    }
}


void AlsaAudio::start (void)
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
}


void AlsaAudio::close_alsa (void)
{
    _running = false;
    get_event (1 << EV_EXIT);
    for (int i = 0; i < _nplay; i++) delete[] _outbuf [i];
    delete _alsa_handle;
    _alsa_handle = 0;
}


void AlsaAudio::thr_main (void)
{
    unsigned long k;

    _alsa_handle->pcm_start ();

    while (_running)
    {
        k = _alsa_handle->pcm_wait ();
        proc_queue (_qnote);
        proc_queue (_qcomm);
        proc_keys1 ();
        proc_keys2 ();
        while (k >= _fsize)
               {
            proc_synth (_fsize);
            _alsa_handle->play_init (_fsize);
            for (int i = 0; i < _nplay; i++) _alsa_handle->play_chan (i, _outbuf [i], _fsize);
            _alsa_handle->play_done (_fsize);
            k -= _fsize;
        }
        proc_mesg ();
    }

    _alsa_handle->pcm_stop ();
    put_event (EV_EXIT);
}