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


#ifndef __MIDIWIN_H
#define __MIDIWIN_H


#include <clxclient.h>
#include "midimatrix.h"
#include "messages.h"


class Midiwin : public X_window, public X_callback
{
public:

    Midiwin (X_window *parent, X_callback *callb, int xp, int yp, X_resman *xresm);
    ~Midiwin (void);

    void setup (M_ifc_init *);

    int  preset (void) const { return _preset; }
    uint16_t *chconf (void) const { return _matrix->get_chconf (); }
    void setconf (M_ifc_chconf *M);

private:

    enum { XSIZE = 840, YSIZE = 130 };

    virtual void handle_event (XEvent *);
    virtual void handle_callb (int, X_window *, XEvent *);

    void handle_xmesg (XClientMessageEvent *);
    void set_butt (int i);
    void add_text (int xp, int yp, int xs, int ys, const char *text, X_textln_style *style, int align);

    Atom            _atom;
    X_callback     *_callb;
    X_resman       *_xresm;
    int             _xp, _yp;
    int             _xs, _ys;
    int             _preset;
    Midimatrix     *_matrix;
    X_tbutton      *_bpres [8];
};


#endif
