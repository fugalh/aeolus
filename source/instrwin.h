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


#ifndef __INSTRWIN_H
#define __INSTRWIN_H


#include <clxclient.h>
#include "messages.h"


class Divis
{
public:

    X_hslider  *_slid [3];
};


class Instrwin : public X_window, public X_callback
{
public:

    Instrwin (X_window *parent, X_callback *callb, int xp, int yp, X_resman *xresm);
    ~Instrwin (void);

    void setup (M_ifc_init *);
    void set_dipar (M_ifc_dipar *M);
    void set_tuning (M_ifc_retune *M);

    int   divis (void) const { return _divis; }
    int   parid (void) const { return _parid; }
    float value (void) const { return _value; }
    bool  final (void) const { return _final; }
    float freq (void) const { return _freq; }
    int   temp (void) const { return _temp; }

private:

    enum { XSIZE = 840, YSIZE = 130 };

    enum
    {
        TEMP_DEC, TEMP_INC, FREQ_DEC, FREQ_INC, TUNE_EXE, TUNE_CAN,
        NDIVIS = 8, DIVIS_BIT0 = 8, DIVIS_STEP = (1 << DIVIS_BIT0), DIVIS_MASK = (DIVIS_STEP - 1),
        NTEMPE = 16
    }; 
           

    virtual void handle_event (XEvent *);
    virtual void handle_callb (int, X_window *, XEvent *);

    void handle_xmesg (XClientMessageEvent *);
    void show_tuning (int s);
    void incdec_temp (int d);
    void incdec_freq (int d);
    void add_text (int xp, int yp, int xs, int ys, const char *text, X_textln_style *style, int align);

    Atom            _atom;
    X_callback     *_callb;
    X_resman       *_xresm;
    int             _xp, _yp;
    X_button       *_dec_freq;
    X_button       *_inc_freq;
    X_button       *_dec_temp;
    X_button       *_inc_temp;
    X_button       *_tune_exe;
    X_button       *_tune_can;
    X_textip       *_freq_txt;
    X_textip       *_temp_txt;
    X_slider       *_trem0_freq;
    X_slider       *_trem0_ampl;
    X_slider       *_trem1_freq;
    X_slider       *_trem1_ampl;
    Divis           _divisd [NDIVIS];
    int             _divis;
    int             _parid;
    float           _value;
    bool            _final;
    int             _ntempe;
    const char     *_temped [NTEMPE];
    float           _freq, _freq1;
    int             _temp, _temp1;
};


#endif
