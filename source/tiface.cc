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


#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "tiface.h"



extern "C" Iface *create_iface (int ac, char *av [])
{
    return new Tiface (ac, av);
}




Reader::Reader (Edest *edest, int ipind) :
    H_thread (edest, ipind)
{
}


Reader::~Reader (void)
{
}


void Reader::read (void)
{
    put_event (0, new M_ifc_txtip);
}


void Reader::thr_main (void)
{
    M_ifc_txtip  *M;

    using_history ();
    while (1)
    {
	get_event (1);
	M = (M_ifc_txtip *) get_message ();
	M->_line = readline ("Aeolus> ");
        if (M->_line) add_history (M->_line);
        reply  (M);
    }
}




Tiface::Tiface (int ac, char *av []) :
    _reader (this, FM_TXTIP),
    _stop (false),
    _init (true),
    _initdata (0),
    _mididata (0)
{
    int i;

    for (i = 0; i < NGROUP; i++) _ifelms [i] = 0;
}


Tiface::~Tiface (void)
{
}


void Tiface::stop (void)
{
}


void Tiface::thr_main (void)
{
    set_time (0);
    inc_time (125000);

    while (! _stop)
    {
	switch (get_event ())
	{
        case FM_MODEL:
        case FM_TXTIP:
            handle_mesg (get_message ()); 
	    break;

        case EV_EXIT:
            return;
	}
    }
    send_event (EV_EXIT, 1);
}


void Tiface::handle_mesg (ITC_mesg *M)
{
    switch (M->type ())
    {
    case MT_IFC_INIT:
	handle_ifc_init ((M_ifc_init *) M);
	M = 0;
	break;

    case MT_IFC_READY:
	handle_ifc_ready ();
	break;

    case MT_IFC_ELCLR:
	handle_ifc_elclr ((M_ifc_ifelm *) M);
        break;

    case MT_IFC_ELSET:
	handle_ifc_elset ((M_ifc_ifelm *) M);
        break;

    case MT_IFC_ELATT:
	handle_ifc_elatt ((M_ifc_ifelm *) M);
        break;

    case MT_IFC_GRCLR:
	handle_ifc_grclr ((M_ifc_ifelm *) M);
        break;

    case MT_IFC_AUPAR:
	break;

    case MT_IFC_DIPAR:
	break;

    case MT_IFC_RETUNE:
	handle_ifc_retune ((M_ifc_retune *) M);
	break;

    case MT_IFC_MCSET:
	handle_ifc_mcset ((M_ifc_chconf *) M);
	M = 0;
	break;

    case MT_IFC_TXTIP:
	handle_ifc_txtip ((M_ifc_txtip *) M);
	break;

    case MT_IFC_PRRCL:
	break;

    default:
	printf ("Received message of unknown type %5ld\n", M->type ());
    }
    if (M) M->recover ();
}


void Tiface::handle_ifc_ready (void)
{
    if (_init)
    {
        printf ("Aeolus is ready.\n");
	print_info ();
        _reader.thr_start (SCHED_OTHER, 0, 0);
	_reader.read ();
    }
    _init = false;
}


void Tiface::handle_ifc_init (M_ifc_init *M)
{
    if (_initdata) _initdata ->recover ();
    _initdata = M;
}


void Tiface::handle_ifc_mcset (M_ifc_chconf *M)
{
    if (_mididata) _mididata ->recover ();
    _mididata = M;
    if (!_init) print_midimap ();
}


void Tiface::handle_ifc_retune (M_ifc_retune *M)
{
    printf ("Retuning Aeolus, A = %3.1lf Hz, %s (%s)\n",
	    M->_freq,  
	    _initdata->_temped [M->_temp]._label, 
	    _initdata->_temped [M->_temp]._mnemo); 
}


void Tiface::handle_ifc_grclr (M_ifc_ifelm *M)
{
    _ifelms [M->_group] = 0;
}


void Tiface::handle_ifc_elclr (M_ifc_ifelm *M)
{
    _ifelms [M->_group] &= ~(1 << M->_ifelm);
}


void Tiface::handle_ifc_elset (M_ifc_ifelm *M)
{
    _ifelms [M->_group] |= (1 << M->_ifelm);
}


void Tiface::handle_ifc_elatt (M_ifc_ifelm *M)
{
    rewrite_label (_initdata->_groupd [M->_group]._ifelmd [M->_ifelm]._label);
    printf ("Retuning %7s %-1s (%s)\n",
	    _initdata->_groupd [M->_group]._label,
	    _tempstr,
	    _initdata->_groupd [M->_group]._ifelmd [M->_ifelm]._mnemo);
}


void Tiface::handle_ifc_txtip (M_ifc_txtip *M)
{
    if (M->_line == 0)
    {
        send_event (EV_EXIT, 1);
	return;
    }
    parse_command (M->_line);
    _reader.read ();
}


void Tiface::print_info (void)
{
    printf ("Application id:  %s\n", _initdata->_appid);
    printf ("Stops directory: %s\n", _initdata->_stopsdir);
    printf ("Instrument:      %s\n", _initdata->_instrdir);
    printf ("ALSA Midi port:  %d:%d\n", _initdata->_client, _initdata->_ipport);
    print_keybdd ();
    print_divisd ();
    print_midimap ();
}


void Tiface::print_keybdd (void)
{
    int i, b, k, n;

    printf ("Keyboards:\n");
    for (k = 0; k < NKEYBD; k++)
    {
	n = 0;
	if (_initdata->_keybdd [k]._label [0])
	{
            printf (" %-7s  midi", _initdata->_keybdd [k]._label);
	    for (i = 0; i < 16; i++)
	    {
		b = _mididata->_bits [i];
		if ((b & 0x1000) && ((b & 7) == k))
		{
                    printf (" %2d", i + 1);
		    n++;
		}
	    }
	    if (!n) printf ("  -");
	    printf ("\n");
	}
    }
}


void Tiface::print_divisd (void)
{
    int i, b, k, n;

    printf ("Divisions:\n");
    for (k = 0; k < NDIVIS; k++)
    {
	n = 0;
	if (_initdata->_divisd [k]._label [0])
	{
            printf (" %-7s  midi", _initdata->_divisd [k]._label);
	    for (i = 0; i < 16; i++)
	    {
		b = _mididata->_bits [i];
		if ((b & 0x2000) && (((b >> 8) & 7) == k))
		{
                    printf (" %2d", i + 1);
		    n++;
		}
	    }
	    if (!n) printf ("  -");
	    printf ("\n");
	}
    }
}


void Tiface::print_midimap (void)
{
    int c, f, k, n;

    printf ("Midi routing:\n");
    n = 0;
    for (c = 0; c < 16; c++)
    {
	f = _mididata->_bits [c];
	k = f & 7;
	f >>= 8;
	f >>= 4;
	if (f)
	{
	    printf (" %2d  ", c + 1);
	    if (f & 1) printf ("keybd %-7s", _initdata->_keybdd [k]._label);
	    if (f & 2) printf ("divis %-7s", _initdata->_divisd [k]._label);
	    if (f & 4) printf ("instr");
	    printf ("\n");
	    n++;
	}
    }
    if (n == 0) printf (" No channels are assigned.\n");
}


void Tiface::print_stops_short (int group)
{
    int       i, n;
    uint32_t  m;

    rewrite_label (_initdata->_groupd [group]._label);
    printf ("Stops in group %s\n", _tempstr);
    m = _ifelms [group];
    n = _initdata->_groupd [group]._nifelm;
    for (i = 0; i < n; i++)
    {
	printf ("  %c %-8s", (m & 1) ? '+' : '-',
                 _initdata->_groupd [group]._ifelmd [i]._mnemo); 
	if ((i % 5) == 4) printf ("\n");
	m >>= 1;
    }
    if (n % 5) printf ("\n");
}


void Tiface::print_stops_long (int group)
{
    int       i, n;
    uint32_t  m;

    rewrite_label (_initdata->_groupd [group]._label);
    printf ("Stops in group %s\n", _tempstr);
    m = _ifelms [group];
    n = _initdata->_groupd [group]._nifelm;
    for (i = 0; i < n; i++)
    {
        rewrite_label (_initdata->_groupd [group]._ifelmd [i]._label);
	printf ("  %c %-7s %-1s\n", (m & 1) ? '+' : '-', 
                _initdata->_groupd [group]._ifelmd [i]._mnemo, _tempstr);
	m >>= 1;
    }
}              


void Tiface::rewrite_label (const char *p)
{
    char *t;

    strcpy (_tempstr, p);
    t = strstr (_tempstr, "-$");
    if (t) strcpy (t, t + 2);
    else
    {
        t = strchr (_tempstr, '$');
        if (t) *t = ' ';
    }
}


void Tiface::parse_command (const char *p)
{
    int c1, c2;

    while (isspace (*p)) p++;
    c1 = *p++;
    if (c1 == 0) return;
    c2 = *p++;
    if (c2 && !isspace (c2)) 
    {
        printf ("Bad command\n");
	return;
    }
    if (c1 == 0) return;
    switch (c1)
    {
    case 'S':
    case 's':
	command_s (p);
	break;

    case 'Q':
    case 'q':
	fclose (stdin);
	break;

    case '!':
	send_event (TO_MODEL, new ITC_mesg (MT_IFC_SAVE));
	break;

    default:
	printf ("Unknown command '%c'\n", c1); 
    }
}


void Tiface::command_s (const char *p)
{
    int  g, i, k, n;
    char s [64];

    if (   (sscanf (p, "%s%n", s, &n) != 1)
	|| ((g = find_group (s)) < 0))
    {
	printf ("Expected a group name, ? or ??\n");
	return;
    }
    p += n;
    if (g == NGROUP + 1)
    {
	for (i = 0; i < _initdata->_ngroup; i++) print_stops_short (i);
	return;
    }
    if (g == NGROUP + 2)
    {
	for (i = 0; i < _initdata->_ngroup; i++) print_stops_long (i);
	return;
    }
    if (   (sscanf (p, "%s%n", s, &n) != 1)
	|| ((k = comm1 (s)) < 0))
    {
	printf ("Expected one of ? ?? + - =\n");
	return;
    }
    p += n;
    if (k == 0)
    {
	print_stops_short (g);
	return;
    }
    if (k == 1)
    {
	print_stops_long (g);
	return;
    }
    if (k == 4)
    {
        send_event (TO_MODEL, new M_ifc_ifelm (MT_IFC_GRCLR, g, 0));
	k = 2;
    }
    if (k == 2) k = MT_IFC_ELSET;
    else        k = MT_IFC_ELCLR;
    while (sscanf (p, "%s%n", s, &n) == 1)
    {
	i = find_ifelm (s, g);
	if (i < 0) printf ("No stop '%s' in this group\n", s);
	else send_event (TO_MODEL, new M_ifc_ifelm (k, g, i));
	p += n;
    }
}


int Tiface::find_group (const char *p)
{
    int g;

    if (! strcmp (p, "?"))  return NGROUP + 1;
    if (! strcmp (p, "??")) return NGROUP + 2;
    for (g = 0; g < _initdata->_ngroup; g++)
    {
	if (! strcmp (p, _initdata->_groupd [g]._label)) return g;
    }
    return -1;
}


int Tiface::find_ifelm (const char *p, int g)
{
    int i, n;

    n = _initdata->_groupd [g]._nifelm;
    for (i = 0; i < n; i++)
    {
	if (! strcmp (p, _initdata->_groupd [g]._ifelmd [i]._mnemo)) return i;
    }
    return -1;
}


int Tiface::comm1 (const char *p)
{
    if (! strcmp (p, "?"))  return 0;
    if (! strcmp (p, "??")) return 1;
    if (! strcmp (p, "+"))  return 2;
    if (! strcmp (p, "-"))  return 3;
    if (! strcmp (p, "="))  return 4;
    return -1;
}
