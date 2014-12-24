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


#ifndef __AUDIOWIN_H
#define __AUDIOWIN_H


#include <clxclient.h>
#include "messages.h"


class Asect
{
public:

    X_hslider      *_slid [5];
    char            _label [64];
};


class Audiowin : public X_window, public X_callback
{
public:

    Audiowin (X_window *parent, X_callback *callb, int xp, int yp, X_resman *xresm);
    ~Audiowin (void);

    void setup (M_ifc_init *);
    void set_aupar (M_ifc_aupar *M);

    int   asect (void) const { return _asect; }
    int   parid (void) const { return _parid; }
    float value (void) const { return _value; }
    bool  final (void) const { return _final; }

private:

    enum { XOFFS = 90, XSTEP = 215, YSIZE = 330 };
    enum { NASECT = 4, ASECT_BIT0 = 8, ASECT_STEP = (1 << ASECT_BIT0), ASECT_MASK = (ASECT_STEP - 1) };
           
    virtual void handle_event (XEvent *);
    virtual void handle_callb (int, X_window *, XEvent *);

    void handle_xmesg (XClientMessageEvent *);
    void add_text (int xp, int yp, int xs, int ys, const char *text, X_textln_style *style);

    Atom            _atom;
    X_callback     *_callb;
    X_resman       *_xresm;
    int             _xp, _yp;
    int             _xs, _ys;
    X_hslider      *_slid [4];
    int             _nasect;
    Asect           _asectd [NASECT];
    int             _asect;
    int             _parid;
    float           _value;
    bool            _final;
};


#endif
