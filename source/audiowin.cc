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

#include <math.h>
#include "audiowin.h"
#include "callbacks.h"
#include "styles.h"


Audiowin::Audiowin (X_window *parent, X_callback *callb, int xp, int yp, X_resman *xresm) :
    X_window (parent, xp, yp, 200, 100, Colors.main_bg),
    _callb (callb),
    _xresm (xresm),
    _xp (xp),
    _yp (yp)
{
    _atom = XInternAtom (dpy (), "WM_DELETE_WINDOW", True);
    XSetWMProtocols (dpy (), win (), &_atom, 1);
    _atom = XInternAtom (dpy (), "WM_PROTOCOLS", True);
}


Audiowin::~Audiowin (void)
{
}


void Audiowin::handle_event (XEvent *E)
{
    switch (E->type)
    {
    case ClientMessage:
        handle_xmesg ((XClientMessageEvent *) E);
        break;
    }
}


void Audiowin::handle_xmesg (XClientMessageEvent *E)
{
    if (E->message_type == _atom) x_unmap ();
}


void Audiowin::handle_callb (int k, X_window *W, XEvent *E)
{
    int c;

    switch (k)
    {
        case SLIDER | X_slider::MOVE: 
        case SLIDER | X_slider::STOP: 
        {
            X_slider *X = (X_slider *) W;
            c = X->cbid ();
            _asect = (c >> ASECT_BIT0) - 1;
            _parid = c & ASECT_MASK;
            _value = X->get_val (); 
            _final = k == (X_callback::SLIDER | X_slider::STOP);
            _callb->handle_callb (CB_AUDIO_ACT, this, E);
            break; 
	}
    }
}


void Audiowin::setup (M_ifc_init *M)
{
    int      i, j, k, x;
    char     s [256];
    Asect    *S; 
    X_hints  H;
    
    but1.size.x = 20;
    but1.size.y = 20;
    _nasect = M->_nasect;
    for (i = 0; i < _nasect; i++)
    {
	S = _asectd + i;
        x = XOFFS + XSTEP * i;
        k = ASECT_STEP * (i + 1);

        (S->_slid [0] = new  X_hslider (this, this, &sli1, &sca_azim, x,  40, 20, k + 0))->x_map ();
        (S->_slid [1] = new  X_hslider (this, this, &sli1, &sca_difg, x,  75, 20, k + 1))->x_map ();
        (S->_slid [2] = new  X_hslider (this, this, &sli1, &sca_dBsh, x, 110, 20, k + 2))->x_map ();
        (S->_slid [3] = new  X_hslider (this, this, &sli1, &sca_dBsh, x, 145, 20, k + 3))->x_map ();
        (S->_slid [4] = new  X_hslider (this, this, &sli1, &sca_dBsh, x, 180, 20, k + 4))->x_map ();
        (new X_hscale (this, &sca_azim, x,  30, 10))->x_map ();
        (new X_hscale (this, &sca_difg, x,  65, 10))->x_map ();
        (new X_hscale (this, &sca_dBsh, x, 133, 10))->x_map ();
        (new X_hscale (this, &sca_dBsh, x, 168, 10))->x_map ();
        S->_label [0] = 0;
        for (j = 0; j <  M->_ndivis; j++)
	{
	    if (M->_divisd [j]._asect == i)
	    {
		if (S->_label [0]) strcat (S->_label, " + ");
                strcat (S->_label, M->_divisd [j]._label);
                add_text (x, 5, 200, 20, S->_label, &text0);
	    } 
	}
    }
    add_text ( 10,  40, 60, 20, "Azimuth", &text0);
    add_text ( 10,  75, 60, 20, "Width",   &text0);
    add_text ( 10, 110, 60, 20, "Direct ", &text0);
    add_text ( 10, 145, 60, 20, "Reflect", &text0);
    add_text ( 10, 180, 60, 20, "Reverb",  &text0);

    (_slid [0] = new  X_hslider (this, this, &sli1, &sca_dBsh, 520, 275, 20, 0))->x_map ();
    (_slid [1] = new  X_hslider (this, this, &sli1, &sca_size,  70, 240, 20, 1))->x_map ();
    (_slid [2] = new  X_hslider (this, this, &sli1, &sca_trev,  70, 275, 20, 2))->x_map ();
    (_slid [3] = new  X_hslider (this, this, &sli1, &sca_spos, 305, 275, 20, 3))->x_map ();
    (new X_hscale (this, &sca_size,  70, 230, 10))->x_map ();
    (new X_hscale (this, &sca_trev,  70, 265, 10))->x_map ();
    (new X_hscale (this, &sca_spos, 305, 265, 10))->x_map ();
    (new X_hscale (this, &sca_dBsh, 520, 265, 10))->x_map ();
    add_text ( 10, 240, 50, 20, "Delay",    &text0);
    add_text ( 10, 275, 50, 20, "Time",     &text0);
    add_text (135, 305, 60, 20, "Reverb",   &text0);
    add_text (355, 305, 80, 20, "Position", &text0);
    add_text (570, 305, 60, 20, "Volume",   &text0);

    sprintf (s, "%s   Aeolus-%s   Audio settings", M->_appid, VERSION);
    x_set_title (s);

    H.position (_xp, _yp);
    H.minsize (200, 100);
    H.maxsize (XOFFS + _nasect * XSTEP, YSIZE);
    H.rname (_xresm->rname ());
    H.rclas (_xresm->rclas ());
    x_apply (&H); 
    x_resize (XOFFS + _nasect * XSTEP, YSIZE);
}


void Audiowin::set_aupar (M_ifc_aupar *M)
{
    if (M->_asect < 0)
    {
        if ((M->_parid >= 0) && (M->_parid < 4))
	{
	    _slid [M->_parid]->set_val (M->_value);
	}
    }
    else if (M->_asect < _nasect)
    {
        if ((M->_parid >= 0) && (M->_parid < 5))
	{
	    _asectd [M->_asect]._slid [M->_parid]->set_val (M->_value);
	}
    }
}


void Audiowin::add_text (int xp, int yp, int xs, int ys, const char *text, X_textln_style *style)
{
    (new X_textln (this, style, xp, yp, xs, ys, text, -1))->x_map ();
}
