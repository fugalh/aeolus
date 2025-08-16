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


#include <cmath>
#include <cstring>
#include "audio_backend.h"
#include "messages.h"


// Static members from original Audio class
const char *AudioBackend::_ports_stereo [2] = { "out.L", "out.R" };
const char *AudioBackend::_ports_ambis1 [4] = { "out.W", "out.X", "out.Y", "out.Z" };


AudioBackend::AudioBackend (const char *appname, Lfq_u32 *qnote, Lfq_u32 *qcomm) :
    A_thread ("Audio"),
    _appname (appname),
    _qnote (qnote),
    _qcomm (qcomm),
    _running (false),
    _policy (SCHED_OTHER),
    _abspri (0),
    _hold (KMAP_ALL),
    _bform (false),
    _nplay (2),
    _fsamp (48000),
    _fsize (1024),
    _nasect (0),
    _ndivis (0),
    _revsize (0.075f),
    _revtime (4.0f)
{
    // Initialize MIDI map and key map
    for (int i = 0; i < 16; i++) _midimap [i] = 0;
    for (int i = 0; i < NNOTES; i++) _keymap [i] = 0;
    for (int i = 0; i < NASECT; i++) _asectp [i] = 0;
    for (int i = 0; i < NDIVIS; i++) _divisp [i] = 0;
    for (int i = 0; i < 8; i++) _outbuf [i] = 0;
}


AudioBackend::~AudioBackend (void)
{
    for (int i = 0; i < _nasect; i++) delete _asectp [i];
    for (int i = 0; i < 8; i++) delete[] _outbuf [i];
}


void AudioBackend::init_audio (void)
{
    int i;

    _audiopar [VOLUME]._val = 0.32f;
    _audiopar [VOLUME]._min = 0.00f;
    _audiopar [VOLUME]._max = 1.00f;
    _audiopar [REVSIZE]._val = _revsize = 0.075f;
    _audiopar [REVSIZE]._min =  0.025f;
    _audiopar [REVSIZE]._max =  0.150f;
    _audiopar [REVTIME]._val = _revtime = 4.0f;
    _audiopar [REVTIME]._min =  2.0f;
    _audiopar [REVTIME]._max =  7.0f;
    _audiopar [STPOSIT]._val =  0.5f;
    _audiopar [STPOSIT]._min = -1.0f;
    _audiopar [STPOSIT]._max =  1.0f;

    _reverb.init (_fsamp);
    _reverb.set_t60mf (_revtime);
    _reverb.set_t60lo (_revtime * 1.50f, 250.0f);
    _reverb.set_t60hi (_revtime * 0.50f, 3e3f);

    _nasect = NASECT;
    for (i = 0; i < NASECT; i++)
    {
        _asectp [i] = new Asection ((float) _fsamp);
        _asectp [i]->set_size (_revsize);
    }
    _hold = KMAP_ALL;
}


void AudioBackend::proc_queue (Lfq_u32 *Q)
{
    int       c, i, j, k, n;
    uint32_t  q;
    uint16_t  m;
    union     { uint32_t i; float f; } u;

    // Execute commands from the model thread (qcomm),
    // or from the midi thread (qnote).

    n = Q->read_avail ();
    while (n > 0)
    {
        q = Q->read (0);
        c = (q >> 24) & 255;  // command
        i = (q >> 16) & 255;  // key, rank or parameter index
        j = (q >>  8) & 255;  // division index
        k = q & 255;          // keyboard index

        switch (c)
        {
        case 0:
            // Single key off.
            key_off_internal (i, 1 << k);
            Q->read_commit (1);
            break;

        case 1:
            // Single key on.
            key_on_internal (i, 1 << k);
            Q->read_commit (1);
            break;

        case 2:
            // All notes off.
            m = (k == NKEYBD) ? KMAP_ALL : (1 << k);
            cond_key_off (m, m);
            Q->read_commit (1);
            break;

        case 3:
            Q->read_commit (1);
            break;

        case 4:
            // Clear bit in division mask.
            _divisp [j]->clr_div_mask (k);
            Q->read_commit (1);
            break;

        case 5:
            // Set bit in division mask.
            _divisp [j]->set_div_mask (k);
            Q->read_commit (1);
            break;

        case 6:
            // Clear bit in rank mask.
            _divisp [j]->clr_rank_mask (i, k);
            Q->read_commit (1);
            break;

        case 7:
            // Set bit in rank mask.
            _divisp [j]->set_rank_mask (i, k);
            Q->read_commit (1);
            break;

        case 8:
            // Hold off.
            // printf ("HOLD OFF %d", k);
            Q->read_commit (1);
            break;

        case 9:
            // Hold on.
            // printf ("HOLD ON  %d", k);
            Q->read_commit (1);
            break;

        case 16:
            // Tremulant on/off.
            if (k) _divisp [j]->trem_on ();
            else   _divisp [j]->trem_off ();
            Q->read_commit (1);
            break;

        case 17:
            // Per-division performance controllers.
            if (n < 2) return;
            u.i = Q->read (1);
            Q->read_commit (2);
            switch (i)
             {
            case 0: _divisp [j]->set_swell (u.f); break;
            case 1: _divisp [j]->set_tfreq (u.f); break;
            case 2: _divisp [j]->set_tmodd (u.f); break;
            }
            break;

        default:
            Q->read_commit (1);
        }
        n = Q->read_avail ();
    }
}


void AudioBackend::proc_keys1 (void)
{
    int       d, n;
    uint16_t  m;

    for (n = 0; n < NNOTES; n++)
    {
        m = _keymap [n];
        if (m & KMAP_SET)
        {
            m ^= KMAP_SET;
               _keymap [n] = m;
            for (d = 0; d < _ndivis; d++)
            {
                _divisp [d]->update (n, m & KMAP_ALL);
            }
        }
    }
}


void AudioBackend::proc_keys2 (void)
{
    int d;

    for (d = 0; d < _ndivis; d++)
    {
        _divisp [d]->update (_keymap);
    }
}


void AudioBackend::proc_synth (int nframes)
{
    int           j, k;
    float         W [PERIOD];
    float         X [PERIOD];
    float         Y [PERIOD];
    float         Z [PERIOD];
    float         R [PERIOD];
    float        *out [8];

    if (fabsf (_revsize - _audiopar [REVSIZE]._val) > 0.001f)
    {
        _revsize = _audiopar [REVSIZE]._val;
        _reverb.set_delay (_revsize);
        for (j = 0; j < _nasect; j++) _asectp[j]->set_size (_revsize);
    }
    if (fabsf (_revtime - _audiopar [REVTIME]._val) > 0.1f)
    {
        _revtime = _audiopar [REVTIME]._val;
         _reverb.set_t60mf (_revtime);
         _reverb.set_t60lo (_revtime * 1.50f, 250.0f);
         _reverb.set_t60hi (_revtime * 0.50f, 3e3f);
    }

    for (j = 0; j < _nplay; j++) out [j] = _outbuf [j];
    for (k = 0; k < nframes; k += PERIOD)
    {
        // Allow backend-specific MIDI processing during synthesis
        proc_midi_during_synth (k + PERIOD);
        
        memset (W, 0, PERIOD * sizeof (float));
        memset (X, 0, PERIOD * sizeof (float));
        memset (Y, 0, PERIOD * sizeof (float));
        memset (Z, 0, PERIOD * sizeof (float));
        memset (R, 0, PERIOD * sizeof (float));

        for (j = 0; j < _ndivis; j++) _divisp [j]->process ();
        for (j = 0; j < _nasect; j++) _asectp [j]->process (_audiopar [VOLUME]._val, W, X, Y, R);
        _reverb.process (PERIOD, _audiopar [VOLUME]._val, R, W, X, Y, Z);

        if (_bform)
        {
            for (j = 0; j < PERIOD; j++)
            {
                out [0][j] = W [j];
                out [1][j] = 1.41 * X [j];
                out [2][j] = 1.41 * Y [j];
                out [3][j] = 1.41 * Z [j];
               }
        }
        else
        {
            for (j = 0; j < PERIOD; j++)
            {
                out [0][j] = W [j] + _audiopar [STPOSIT]._val * X [j] + Y [j];
                out [1][j] = W [j] + _audiopar [STPOSIT]._val * X [j] - Y [j];
               }
        }
        for (j = 0; j < _nplay; j++) out [j] += PERIOD;
    }
}


void AudioBackend::proc_mesg (void)
{
    ITC_mesg *M;

    while (get_event_nowait () != EV_TIME)
    {
        M = get_message ();
        if (! M) continue;

        switch (M->type ())
        {
            case MT_NEW_DIVIS:
            {
                M_new_divis  *X = (M_new_divis *) M;
                Division     *D = new Division (_asectp [X->_asect], (float) _fsamp);
                D->set_div_mask (X->_keybd);
                D->set_swell (X->_swell);
                D->set_tfreq (X->_tfreq);
                D->set_tmodd (X->_tmodd);
                _divisp [_ndivis] = D;
                _ndivis++;
                break;
            }
            case MT_CALC_RANK:
            case MT_LOAD_RANK:
            {
                M_def_rank *X = (M_def_rank *) M;
                _divisp [X->_divis]->set_rank (X->_rank, X->_rwave,  X->_synth->_pan, X->_synth->_del);
                send_event (TO_MODEL, M);
                M = 0;
                break;
            }
            case MT_AUDIO_SYNC:
                send_event (TO_MODEL, M);
                M = 0;
                break;
        }
        if (M) M->recover ();
    }
}


// MidiProcessor::Handler implementation
void AudioBackend::key_on(int note, int keyboard)
{
    key_on_internal(note, 1 << keyboard);
}


void AudioBackend::key_off(int note, int keyboard)
{
    key_off_internal(note, 1 << keyboard);
}


void AudioBackend::all_sound_off()
{
    uint16_t m = KMAP_ALL;
    cond_key_off(m, m);
}


void AudioBackend::all_notes_off(int keyboard)
{
    uint16_t m = 1 << keyboard;
    cond_key_off(m, m);
}


void AudioBackend::hold_pedal(int keyboard, bool on)
{
    // Implementation depends on specific backend
    // Base class provides no-op implementation
}