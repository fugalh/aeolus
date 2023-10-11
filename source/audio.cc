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


#include <math.h>
#include <jack/midiport.h>
#include "audio.h"
#include "messages.h"



Audio::Audio (const char *name, Lfq_u32 *qnote, Lfq_u32 *qcomm) :
    A_thread ("Audio"), 
    _appname (name),
    _qnote (qnote),
    _qcomm (qcomm),
    _qmidi (0),
    _running (false),
#ifdef __linux__
    _alsa_handle (0),
#endif
    _jack_handle (0),
    _abspri (0),
    _relpri (0),
    _bform (0),
    _nplay (0),
    _fsamp (0),
    _fsize (0),
    _nasect (0),
    _ndivis (0)
{
}


Audio::~Audio (void)
{
    int i;

#ifdef __linux__
    if (_alsa_handle) close_alsa ();
#endif
    if (_jack_handle) close_jack ();
    for (i = 0; i < _nasect; i++) delete _asectp [i];
    for (i = 0; i < _ndivis; i++) delete _divisp [i];
    _reverb.fini ();
}


void Audio::init_audio (void)
{
    int i;

    _jmidi_pdata = 0;
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


void Audio::start (void)
{
    M_audio_info  *M;
    int           i;

    M = new M_audio_info ();
    M->_nasect = _nasect;
    M->_fsamp  = _fsamp;
    M->_fsize  = _fsize;
    M->_instrpar = _audiopar;
    for (i = 0; i < _nasect; i++) M->_asectpar [i] = _asectp [i]->get_apar ();
    send_event (TO_MODEL, M);
}


#ifdef __linux__
void Audio::init_alsa (const char *device, int fsamp, int fsize, int nfrag)
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
#endif


#ifdef __linux__
void Audio::close_alsa ()
{
    _running = false;
    get_event (1 << EV_EXIT);
    for (int i = 0; i < _nplay; i++) delete[] _outbuf [i];
    delete _alsa_handle;
}
#endif


void Audio::thr_main (void) 
{
#ifdef __linux__
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
#endif
}


void Audio::init_jack (const char *server, bool bform, Lfq_u8 *qmidi)
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


void Audio::close_jack ()
{
    jack_deactivate (_jack_handle);
    for (int i = 0; i < _nplay; i++) jack_port_unregister(_jack_handle, _jack_opport [i]);
    jack_client_close (_jack_handle);
}


void Audio::jack_static_shutdown (void *arg)
{
    return ((Audio *) arg)->jack_shutdown ();
}


void Audio::jack_shutdown (void)
{
    _running = false;
    send_event (EV_EXIT, 1);
}


int Audio::jack_static_callback (jack_nframes_t nframes, void *arg)
{
    return ((Audio *) arg)->jack_callback (nframes);
}


int Audio::jack_callback (jack_nframes_t nframes)
{
    int i;

    proc_queue (_qnote);
    proc_queue (_qcomm);
    proc_keys1 ();
    proc_keys2 ();
    for (i = 0; i < _nplay; i++) _outbuf [i] = (float *)(jack_port_get_buffer (_jack_opport [i], nframes));
    _jmidi_pdata = jack_port_get_buffer (_jack_midipt, nframes);
    _jmidi_count = jack_midi_get_event_count (_jmidi_pdata);
    _jmidi_index = 0;
    proc_synth (nframes);
    proc_mesg ();
    return 0;
}


void Audio::proc_jmidi (int tmax) 
{
    int                 c, f, k, m, n, t, v;
    jack_midi_event_t   E;

    // Read and process MIDI commands from the JACK port.
    // Events related to keyboard state are dealt with
    // locally. All the rest is sent as raw MIDI to the
    // model thread via qmidi.

    while (   (jack_midi_event_get (&E, _jmidi_pdata, _jmidi_index) == 0)
           && (E.time < (jack_nframes_t) tmax))
    {
	t = E.buffer [0];
        n = E.buffer [1];
	v = E.buffer [2];
	c = t & 0x0F;
        k = _midimap [c] & 15;
        f = _midimap [c] >> 12;
        
	switch (t & 0xF0)
	{
	case 0x80:
	case 0x90:
	    // Note on or off.
	    if (v && (t & 0x10))
	    {
		// Note on.
		if (n < 36)
		{
		    if ((f & 4) && (n >= 24) && (n < 34))
		    {
			// Preset selection, sent to model thread
			// if on control-enabled channel.
			if (_qmidi->write_avail () >= 3)
			{
			    _qmidi->write (0, t);
			    _qmidi->write (1, n);
			    _qmidi->write (2, v);
			    _qmidi->write_commit (3);
			}
		    }
		}
		else if (n <= 96)
		{
		    if (f & 1) key_on (n - 36, 1 << k);
		}
	    }
	    else
	    {
		// Note off.
		if (n < 36)
		{
		}
		else if (n <= 96)
		{
		    if (f & 1) key_off (n - 36, 1 << k);
		}
	    }
	    break;

	case 0xB0: // Controller
	    switch (n)
	    {
	    case MIDICTL_ASOFF:
		// All sound off, accepted on control channels only.
		// Clears all keyboards.
                if (f & 4)
                {
                    m = KMAP_ALL;
                    cond_key_off (m, m);
                }
		break;

	    case MIDICTL_ANOFF:
		// All notes off, accepted on channels controlling
		// a keyboard.
                if (f & 1)
                {
                    m = 1 << k;
                    cond_key_off (m, m);
                }
		break;
	
	    case MIDICTL_BANK:	
	    case MIDICTL_IFELM:	
                // Program bank selection or stop control, sent
                // to model thread if on control-enabled channel.
		if (f & 4)
		{
		    if (_qmidi->write_avail () >= 3)
		    {
			_qmidi->write (0, t);
			_qmidi->write (1, n);
			_qmidi->write (2, v);
			_qmidi->write_commit (3);
		    }
		}
	    case MIDICTL_SWELL:
	    case MIDICTL_TFREQ:
	    case MIDICTL_TMODD:
		// Per-division performance controls, sent to model
                // thread if on a channel that controls a division.
		if (f & 2)
		{
		    if (_qmidi->write_avail () >= 3)
		    {
			_qmidi->write (0, t);
			_qmidi->write (1, n);
			_qmidi->write (2, v);
			_qmidi->write_commit (3);
		    }
		}
		break;
	    }
	    break;

	case 0xC0:
            // Program change sent to model thread
            // if on control-enabled channel.
            if (f & 4)
   	    {
	        if (_qmidi->write_avail () >= 3)
	        {
		    _qmidi->write (0, t);
		    _qmidi->write (1, n);
		    _qmidi->write (2, 0);
		    _qmidi->write_commit (3);
		}
	    }
	    break;
	}	
	_jmidi_index++;
    }
}


void Audio::proc_queue (Lfq_u32 *Q) 
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
            key_off (i, 1 << k);
	    Q->read_commit (1);
	    break;

	case 1:
	    // Single key on.
            key_on (i, 1 << k);
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
            printf ("HOLD OFF %d", k);
//            _hold = KMAP_ALL;
//             cond_key_off (KMAP_HLD, KMAP_HLD);
            Q->read_commit (1);
	    break;

        case 9:
	    // Hold on.
            printf ("HOLD ON  %d", k);
//            _hold = KMAP_ALL | KMAP_HLD;
//            cond_key_on (, KMAP_HLD);
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


void Audio::proc_keys1 (void)
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


void Audio::proc_keys2 (void)
{    
    int d;

    for (d = 0; d < _ndivis; d++)
    {
        _divisp [d]->update (_keymap);
    }
}


void Audio::proc_synth (int nframes) 
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
	if (_jmidi_pdata)
	{
	    proc_jmidi (k + PERIOD);
	    proc_keys1 ();
	}

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


void Audio::proc_mesg (void) 
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


const char *Audio::_ports_stereo [2] = { "out.L", "out.R" };
const char *Audio::_ports_ambis1 [4] = { "out.W", "out.X", "out.Y", "out.Z" };
