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


#include "alsa_midi.h"
#include "midi_processor.h"


AlsaMidi::AlsaMidi (Lfq_u32 *qnote, Lfq_u8 *qmidi, uint16_t *midimap, const char *appname) :
    MidiBackend ("AlsaMidi", qnote, qmidi, midimap, appname),
    _handle (0)
{
}


AlsaMidi::~AlsaMidi (void)
{
    if (_handle) close_midi ();
}


void AlsaMidi::terminate (void)
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


void AlsaMidi::open_midi (void)
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


void AlsaMidi::close_midi (void)
{
    if (_handle) 
    {
        snd_seq_close (_handle);
        _handle = 0;
    }
}


void AlsaMidi::proc_midi (void) 
{
    snd_seq_event_t  *E;
    int              c, n, p, t, v;

    // Read and process MIDI commands from the ALSA port using MidiProcessor

    while (true)
    {
	snd_seq_event_input(_handle, &E);
        c = E->data.note.channel;               
        t = E->type;

	switch (t)
	{ 
	case SND_SEQ_EVENT_NOTEON:
	case SND_SEQ_EVENT_NOTEOFF:
	    n = E->data.note.note;
	    v = E->data.note.velocity;
            
            // Convert ALSA event to raw MIDI format for MidiProcessor
            uint8_t status = ((t == SND_SEQ_EVENT_NOTEON) ? 0x90 : 0x80) | c;
            MidiProcessor::process_midi_event(status, n, v, c, _midimap, this, _qnote, _qmidi);
	    break;

	case SND_SEQ_EVENT_CONTROLLER:
	    p = E->data.control.param;
	    v = E->data.control.value;
            
            // Convert to controller MIDI message
            uint8_t ctrl_status = 0xB0 | c;
            MidiProcessor::process_midi_event(ctrl_status, p, v, c, _midimap, this, _qnote, _qmidi);
	    break;

	case SND_SEQ_EVENT_PGMCHANGE:
            // Program change - convert to MIDI format  
            uint8_t pgm_status = 0xC0 | c;
            MidiProcessor::process_midi_event(pgm_status, E->data.control.value, 0, c, _midimap, this, _qnote, _qmidi);
	    break;

	case SND_SEQ_EVENT_USR0:
	    // User event, terminates this thread if we sent it.
	    if (E->source.client == _client) return;
            break;
	}
    }
}