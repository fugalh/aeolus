/*
    Copyright (C) 2003-2008 Fons Adriaensen <fons@kokkinizita.net>
    
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/


#include "midiwin.h"
#include "callbacks.h"
#include "styles.h"


Midiwin::Midiwin (X_window *parent, X_callback *callb, int xp, int yp, X_resman *xresm) :
    X_window (parent, xp, yp, XSIZE, YSIZE, Colors.main_bg),
    _callb (callb),
    _xresm (xresm),
    _xp (xp),
    _yp (yp),
    _preset (-1)
{
    _atom = XInternAtom (dpy (), "WM_DELETE_WINDOW", True);
    XSetWMProtocols (dpy (), win (), &_atom, 1);
    _atom = XInternAtom (dpy (), "WM_PROTOCOLS", True);
}


Midiwin::~Midiwin (void)
{
}


void Midiwin::handle_event (XEvent *E)
{
    switch (E->type)
    {
    case ClientMessage:
        handle_xmesg ((XClientMessageEvent *) E);
        break;
    }
}


void Midiwin::handle_xmesg (XClientMessageEvent *E)
{
    if (E->message_type == _atom) x_unmap ();
}


void Midiwin::handle_callb (int k, X_window *W, XEvent *E)
{
    switch (k)
    {
    case BUTTON | X_button::PRESS:
    {
	X_button      *B = (X_button *) W;
        XButtonEvent  *X = (XButtonEvent *) E;

        set_butt (B->cbid ());
        if (X->state & ShiftMask) _callb->handle_callb (CB_MIDI_SETCONF, this, 0);
        else                      _callb->handle_callb (CB_MIDI_GETCONF, this, 0);
	break;
    }
    case CB_MIDI_MODCONF:
        set_butt (-1);
        _callb->handle_callb (CB_MIDI_SETCONF, this, 0);
	break;
    }
}


void Midiwin::setup (M_ifc_init *M)
{
    X_hints H;
    int     i, x, y;
    char    s [256];

    _matrix = new Midimatrix (this, this, 10, 10); 
    _matrix->init (M);

    x = 10;
    y = _matrix->ysize () + 20;
    but1.size.x = 30;
    but1.size.y = 20;

    for (i = 0; i < 8; i++)
    {
        sprintf (s, "%d", i + 1);
	_bpres [i] = new X_tbutton (this, this, &but1, x, y, s, 0, i);        
	_bpres [i]->x_map ();
        x += 32;
    } 

    x += 10;
    add_text (x, y, 200, 20, "Shift-click to store preset", &text0, -1);
    _xs = _matrix->xsize () + 20;
    _ys = _matrix->ysize () + 60;
    H.position (_xp, _yp);
    H.minsize (_xs, _ys);
    H.maxsize (_xs, _ys);
    H.rname (_xresm->rname ());
    H.rclas (_xresm->rclas ());
    x_apply (&H); 
    x_resize (_xs, _ys);

    sprintf (s, "%s   Aeolus-%s   Midi settings", M->_appid, VERSION);
    x_set_title (s);
}


void Midiwin::setconf (M_ifc_chconf *M)
{
    int k;

    k = M->_index;
    if (k >= 0)
    {
	if (k >= 8) k = -1;
        set_butt (k);  
    }
    _matrix->set_chconf (M->_bits);
}


void Midiwin::set_butt (int i)
{
    if (i != _preset)
    {
        if (_preset >= 0) _bpres [_preset]->set_stat (0);
        _preset = i;
        if (_preset >= 0) _bpres [_preset]->set_stat (1);
    }
}


void Midiwin::add_text (int xp, int yp, int xs, int ys, const char *text, X_textln_style *style, int align)
{
    (new X_textln (this, style, xp, yp, xs, ys, text, align))->x_map ();
}
