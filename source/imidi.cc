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


#include "imidi.h"



Imidi::Imidi (Lfq_u32 *qnote, Lfq_u8 *qmidi, uint16_t *midimap, const char *appname) :
    A_thread ("Imidi"),
    _qnote (qnote),
    _qmidi (qmidi),
    _midimap (midimap),
    _appname (appname)
{
}


Imidi::~Imidi (void)
{
}


void Imidi::terminate (void)
{
    snd_seq_event_t E;

    if (_handle)
    {   
	snd_seq_ev_clear (&E);
	snd_seq_ev_set_direct (&E);
	E.type = SND_SEQ_EVENT_USR0;
	E.source.port = _opport;
	E.dest.client = _client;
	E.dest.port   = _ipport;
	snd_seq_event_output_direct (_handle, &E);
    }
}


void Imidi::thr_main (void)
{
    open_midi ();
    proc_midi ();
    close_midi ();
    send_event (EV_EXIT, 1);
}


void Imidi::open_midi (void)
{
    snd_seq_client_info_t *C;
    M_midi_info *M;

    if (snd_seq_open (&_handle, "hw", SND_SEQ_OPEN_DUPLEX, 0) < 0)
    {
        fprintf(stderr, "Error opening ALSA sequencer.\n");
        exit(1);
    }

    snd_seq_client_info_alloca (&C);
    snd_seq_get_client_info (_handle, C);
    _client = snd_seq_client_info_get_client (C);
    snd_seq_client_info_set_name (C, _appname);
    snd_seq_set_client_info (_handle, C);

    if ((_ipport = snd_seq_create_simple_port (_handle, "In",
        SND_SEQ_PORT_CAP_WRITE | SND_SEQ_PORT_CAP_SUBS_WRITE,
        SND_SEQ_PORT_TYPE_APPLICATION)) < 0)
    {
        fprintf(stderr, "Error creating sequencer input port.\n");
        exit(1);
    }

    if ((_opport = snd_seq_create_simple_port (_handle, "Out",
         SND_SEQ_PORT_CAP_WRITE,
         SND_SEQ_PORT_TYPE_APPLICATION)) < 0)
    {
        fprintf(stderr, "Error creating sequencer output port.\n");
        exit(1);
    }

    M = new M_midi_info ();
    M->_client = _client;
    M->_ipport = _ipport;
    memcpy (M->_chbits, _midimap, 16 * sizeof (uint16_t));
    send_event (TO_MODEL, M);
}


void Imidi::close_midi (void)
{
    if (_handle) snd_seq_close (_handle);
}


void Imidi::proc_midi (void) 
{
    snd_seq_event_t  *E;
    int              c, f, k, n, p, t, v;

    // Read and process MIDI commands from the ALSA port.
    // Events related to keyboard state are sent to the 
    // audio thread via the qnote queue. All the rest is
    // sent as raw MIDI to the model thread via qmidi.

    while (true)
    {
	snd_seq_event_input(_handle, &E);
        c = E->data.note.channel;               
        k = _midimap [c] & 15;
        f = _midimap [c] >> 12;
        t = E->type;

	switch (t)
	{ 
	case SND_SEQ_EVENT_NOTEON:
	case SND_SEQ_EVENT_NOTEOFF:
	    n = E->data.note.note;
	    v = E->data.note.velocity;
            if ((t == SND_SEQ_EVENT_NOTEON) && v)
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
		            _qmidi->write (0, 0x90);
		            _qmidi->write (1, n);
		            _qmidi->write (2, v);
		            _qmidi->write_commit (3);
			}
	            }
	        }
                else if (n <= 96)
	        {
                    if (f & 1)
		    {
	                if (_qnote->write_avail () > 0)
	                {
	                    _qnote->write (0, (1 << 24) | ((n - 36) << 16) | k);
                            _qnote->write_commit (1);
	                } 
		    }
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
                    if (f & 1)
		    {
	                if (_qnote->write_avail () > 0)
	                {
	                    _qnote->write (0, (0 << 24) | ((n - 36) << 16) | k);
                            _qnote->write_commit (1);
	                } 
		    }
		}
	    }
	    break;

	case SND_SEQ_EVENT_CONTROLLER:
	    p = E->data.control.param;
	    v = E->data.control.value;
	    switch (p)
	    {
	    case MIDICTL_HOLD:
		// Hold pedal.
                if (f & 1)
                {
                    c = (v > 63) ? 9 : 8;
                    if (_qnote->write_avail () > 0)
                    {
                        _qnote->write (0, (c << 24) | k);
                        _qnote->write_commit (1);
                    }
		}
		break;

	    case MIDICTL_ASOFF:
		// All sound off, accepted on control channels only.
		// Clears all keyboards, including held notes.
		if (f & 4)
		{
	            if (_qnote->write_avail () > 0)
	            {
	                _qnote->write (0, (2 << 24) | NKEYBD);
                        _qnote->write_commit (1);
	            } 
		}
		break;

            case MIDICTL_ANOFF:
		// All notes off, accepted on channels controlling
		// a keyboard. Does not clear held notes. 
		if (f & 1)
		{
	            if (_qnote->write_avail () > 0)
	            {
	                _qnote->write (0, (2 << 24) | k);
                        _qnote->write_commit (1);
	            } 
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
			_qmidi->write (0, 0xB0 | c);
			_qmidi->write (1, p);
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
			_qmidi->write (0, 0xB0 | c);
			_qmidi->write (1, p);
			_qmidi->write (2, v);
			_qmidi->write_commit (3);
		    }
		}
		break;
	    }
	    break;

	case SND_SEQ_EVENT_PGMCHANGE:
            // Program change sent to model thread
            // if on control-enabled channel.
	    if (f & 4)
	    {
   	        if (_qmidi->write_avail () >= 3)
	        {
		    _qmidi->write (0, 0xC0);
		    _qmidi->write (1, E->data.control.value);
		    _qmidi->write (2, 0);
		    _qmidi->write_commit (3);
		}
            }
	    break;

	case SND_SEQ_EVENT_USR0:
	    // User event, terminates this trhead if we sent it.
	    if (E->source.client == _client) return;
	}
    }
}

