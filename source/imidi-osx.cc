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


// OS X CoreMIDI version of Imidi. Might be cleaner to do some kind of
// inheritance, but for now this will do. Makefile.osx doesn't build imidi.cc
// but rather this file, and vice versa for the normal Linux Makefile.


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
    put_event (EV_EXIT, 1);
}


void Imidi::thr_main (void)
{
    open_midi ();
    get_event (1 << EV_EXIT);
    close_midi ();
    send_event (EV_EXIT, 1);
}


static void coremidi_proc_static (const MIDIPacketList *pktlist, void *refCon, void *connRefCon)
{
    Imidi *imidi = (Imidi*)refCon;
    imidi->coremidi_proc (pktlist, 0, connRefCon);
}


void Imidi::open_midi (void)
{
    // If in the future dest is needed in close_midi (or elsewhere) this could
    // be made an instance variable.
    MIDIEndpointRef dest;

    _handle = NULL;
    CFStringRef name = CFStringCreateWithCString(0, _appname, kCFStringEncodingUTF8);
    MIDIClientCreate(name, NULL, NULL, &_handle);
    MIDIDestinationCreate(_handle, name, coremidi_proc_static, this, &dest);

#if 0
    // is this stuff needed? I'm guessing no, but we'll see when I plugin the
    // USB keyboard
    MIDIPortRef inPort = NULL;
    MIDIInputPortCreate(_handle, CFSTR("In"), coremidi_proc_static, this, &inPort);

    // open connections from all sources (TODO is this what we want?)
    int n = MIDIGetNumberOfSources();
    for (int i=0; i<n; i++)
    {
        MIDIEndpointRef src = MIDIGetSource(i);
        MIDIPortConnectSource(inPort, src, (void*)src);
    }
#endif

    // FA
    // _client and _ippport are printed in the main window's title as [_client:_ipport]
    // They are the info required to connect to a source using the ALSA sequencer.
    // Anything equivalent for OSX ?

    // hcf 
    // _appname is the name of the destination, and that's what users are
    // likely to use when connecting MIDI sources to it.

    M_midi_info *M = new M_midi_info ();
    M->_client = 0; 
    M->_ipport = 0;
    memcpy (M->_chbits, _midimap, 16);
    send_event (TO_MODEL, M);
}


void Imidi::close_midi (void)
{
    // nothing to do here - everything is closed automagically (if I
    // understand the docs correctly)
}


void Imidi::coremidi_proc(const MIDIPacketList *pktlist, void *refCon, void *connRefCon)
{
    int c, d, f, m, n, p, t, v, s;
    MIDIPacket *packet = (MIDIPacket *)pktlist->packet;

    for (unsigned int j=0; j < pktlist->numPackets; j++)
    {
        for (int i=0; i < packet->length; i++) 
        {
            // FA 
            // Maybe this should be factored out so it can be
            // shared with the ALSA version. But not urgent.

            // hcf
            // Yeah, and the JACK MIDI version too.

            Byte *E = &packet->data[i];
            s = E[0];
            t = s & 0xF0;
            c = s & 0x0F;

            if (!(s & 0x80))
                continue; // skip data bytes

            m = _midimap [c] & 127;        // Keyboard and hold bits
            d = (_midimap [c] >>  8) & 7;  // Division number if (f & 2)
            f = (_midimap [c] >> 12) & 7;  // Control enabled if (f & 4)

            switch (t)
            { 
            case 0x80:
            case 0x90:
                // Note on or off.
                n = E[1];
                v = E[2];
                if ((t == 0x90) && v)
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
                        if (m)
                        {
                            if (_qnote->write_avail () > 0)
                            {
                                _qnote->write (0, (1 << 24) | ((n - 36) << 8) | m);
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
                        if (m)
                        {
                            if (_qnote->write_avail () > 0)
                            {
                                _qnote->write (0, ((n - 36) << 8) | m);
                                _qnote->write_commit (1);
                            } 
                        }
                    }
                }
                break;

            case 0xB0:
                p = E[1];
                v = E[2];
                switch (p)
                {
                case MIDICTL_HOLD:
                    // Hold pedal.
                    if (m & HOLD_MASK)
                    {
                        v = (v > 63) ? 9 : 8;
                        if (_qnote->write_avail () > 0)
                        {
                            _qnote->write (0, (v << 24) | (m << 16));
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
                            _qnote->write (0, (2 << 24) | ( ALL_MASK << 16) | ALL_MASK);
                            _qnote->write_commit (1);
                        } 
                    }
                    break;

                case MIDICTL_ANOFF:
                    // All notes off, accepted on channels controlling
                    // a keyboard. Does not clear held notes. 
                    if (m)
                    {
                        if (_qnote->write_avail () > 0)
                        {
                            _qnote->write (0, (2 << 24) | (m << 16) | m);
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

            case 0xC0:
                // Program change sent to model thread
                // if on control-enabled channel.
                n = E[1];
                if (f & 4)
                {
                    if (_qmidi->write_avail () >= 3)
                    {
                        _qmidi->write (0, 0xC0);
                        _qmidi->write (1, n);
                        _qmidi->write (2, 0);
                        _qmidi->write_commit (3);
                    }
                }
                break;
            }
        }

        packet = MIDIPacketNext(packet);
    }
}
