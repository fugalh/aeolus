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


#include "instrwin.h"
#include "callbacks.h"
#include "styles.h"


Instrwin::Instrwin (X_window *parent, X_callback *callb, int xp, int yp, X_resman *xresm) :
    X_window (parent, xp, yp, XSIZE, YSIZE, Colors.main_bg),
    _callb (callb),
    _xresm (xresm),
    _xp (xp), _yp (yp)
{
    _atom = XInternAtom (dpy (), "WM_DELETE_WINDOW", True);
    XSetWMProtocols (dpy (), win (), &_atom, 1);
    _atom = XInternAtom (dpy (), "WM_PROTOCOLS", True);
}


Instrwin::~Instrwin (void)
{
}


void Instrwin::handle_event (XEvent *E)
{
    switch (E->type)
    {
    case ClientMessage:
        handle_xmesg ((XClientMessageEvent *) E);
        break;
    }
}


void Instrwin::handle_xmesg (XClientMessageEvent *E)
{
    if (E->message_type == _atom) x_unmap ();
}


void Instrwin::handle_callb (int k, X_window *W, XEvent *E)
{
    int c;

    switch (k)
    {
        case SLIDER | X_slider::MOVE:
        case SLIDER | X_slider::STOP:
        {
            X_slider *X = (X_slider *) W;
            c = X->cbid ();
            _divis = (c >> DIVIS_BIT0) - 1;
            _parid = c & DIVIS_MASK;
            _value = X->get_val ();
            _final = k == (SLIDER | X_slider::STOP);
            _callb->handle_callb (CB_DIVIS_ACT, this, E);
            break;
        }

        case BUTTON | X_button::PRESS:
        {
            X_button *B = (X_button *) W;
            k = B->cbid ();
            switch (k)
            {
            case TEMP_DEC:
            case TEMP_INC:
                incdec_temp (k == TEMP_DEC ? -1 : 1);
                break;

            case FREQ_DEC:
            case FREQ_INC:
                incdec_freq (k == FREQ_DEC ? -1 : 1);
                break;

            case TUNE_EXE:
                _callb->handle_callb (CB_RETUNE, this, E);
                break;

            case TUNE_CAN:
                _freq = _freq1;
                _temp = _temp1;
                show_tuning (0);
                break;
            }
        }
    }
}


void Instrwin::setup (M_ifc_init *M)
{
    int      i, k, n1, n2, x1, x2, y;
    char     s [256];
    Divis    *D;
    X_hints  H;


    add_text (100, 5, 60, 20, "Tuning", &text0, -1);
    but2.size.x = 17;
    but2.size.y = 17;
    _temp_txt = new X_textip  (this, 0,   &text0,  15, 41, 150, 20, 31);
    _temp_txt->set_align (1);
    _temp_txt->x_map ();
    _freq_txt = new X_textip  (this, 0,   &text0, 105, 65,  60, 20, 7);
    _freq_txt->set_align (1);
    _freq_txt->x_map ();
    (_dec_temp = new X_ibutton (this, this, &but2, 170, 41, disp ()->image1515 (X_display::IMG_LT), TEMP_DEC))->x_map ();
    (_inc_temp = new X_ibutton (this, this, &but2, 187, 41, disp ()->image1515 (X_display::IMG_RT), TEMP_INC))->x_map ();
    (_dec_freq = new X_ibutton (this, this, &but2, 170, 65, disp ()->image1515 (X_display::IMG_LT), FREQ_DEC))->x_map ();
    (_inc_freq = new X_ibutton (this, this, &but2, 187, 65, disp ()->image1515 (X_display::IMG_RT), FREQ_INC))->x_map ();
    but1.size.x = 60;
    but1.size.y = 20;
    (_tune_exe = new X_tbutton (this, this, &but1,  70, 100, "Retune", 0, TUNE_EXE))->x_map ();
    (_tune_can = new X_tbutton (this, this, &but1, 135, 100, "Cancel", 0, TUNE_CAN))->x_map ();

    for (i = n1 = n2 = 0; i < M->_ndivis; i++)
    {
        if (M->_divisd [i]._flags & 1) n1++;
        if (M->_divisd [i]._flags & 2) n2++;
    }
    x1 = 310;
    x2 = n1 ? 640 : x1;
    y = 40;
    D = _divisd;
    for (i = 0; i < M->_ndivis; i++)
    {
        k = DIVIS_STEP * (i + 1);
        if (M->_divisd [i]._flags & 1)
        {
            (D->_slid [0] = new X_hslider (this, this, &sli1, &sca_Swl, x2, y, 20, k))->x_map ();
            (new X_hscale (this, &sca_Swl, x2, y + 20, 10))->x_map ();
        }
        else D->_slid [0] = 0;
        if (M->_divisd [i]._flags & 2)
        {
            (D->_slid [1]  = new X_hslider (this, this, &sli1, &sca_Tfr, x1,       y, 20, k + 1))->x_map ();
            (D->_slid [2]  = new X_hslider (this, this, &sli1, &sca_Tmd, x1 + 160, y, 20, k + 2))->x_map ();
            (new X_hscale (this, &sca_Tfr, x1,       y + 20, 10))->x_map ();
            (new X_hscale (this, &sca_Tmd, x1 + 160, y + 20, 10))->x_map ();
        }
        else D->_slid [1] = D->_slid [2] = 0;
        if (D->_slid [0] || D->_slid [1])
        {
            add_text (x1 - 90, y, 80, 20,  M->_divisd [i]._label, &text0, 1);
            y += 40;
        }
        D++;
    }

    if (n1)
    {
        add_text (x1,       5, 80, 20, "Trem freq", &text0, -1);
        add_text (x1 + 160, 5, 80, 20, "Trem amp",  &text0, -1);
    }
    if (n2) add_text (x2, 5, 80, 20, "Swell", &text0, -1);

    y += 5;
    if (y < YSIZE) y = YSIZE;
    sprintf (s, "%s   Aeolus-%s   Instrument settings", M->_appid, VERSION);
    x_set_title (s);

    _ntempe = M->_ntempe;
    if (_ntempe > NTEMPE) _ntempe = NTEMPE;
    for (i = 0; i < _ntempe; i++) _temped [i] = M->_temped [i]._label;

    H.position (_xp, _yp);
    H.minsize (200, 100);
    H.maxsize (XSIZE, y);
    H.rname (_xresm->rname ());
    H.rclas (_xresm->rclas ());
    x_apply (&H);
    x_resize (XSIZE, y);
}


void Instrwin::set_dipar (M_ifc_dipar *M)
{
    X_slider *S;

    if ((M->_divis >= 0) && (M->_divis < NDIVIS))
    {
        if ((M->_parid >= 0) && (M->_parid < 3))
        {
            S = _divisd [M->_divis]._slid [M->_parid];
            if (S) S->set_val (M->_value);
        }
    }
}


void Instrwin::set_tuning (M_ifc_retune *M)
{
    _freq = _freq1 = M->_freq;
    _temp = _temp1 = M->_temp;
    show_tuning (0);
}


void Instrwin::show_tuning (int b)
{
    char s [16];

    sprintf (s, "%3.1lf", _freq);
    _freq_txt->set_text (s);
    _temp_txt->set_text (_temped [_temp] );
    _tune_exe->set_stat (b);
    _tune_can->set_stat (b);
}

void Instrwin::incdec_temp (int d)
{
    _temp += d + _ntempe;
    _temp %= _ntempe;
    show_tuning (1);
}


void Instrwin::incdec_freq (int d)
{
    _freq += d;
    if (_freq < 400) _freq = 400;
    if (_freq > 480) _freq = 480;
    show_tuning (1);
}


void Instrwin::add_text (int xp, int yp, int xs, int ys, const char *text, X_textln_style *style, int align)
{
    (new X_textln (this, style, xp, yp, xs, ys, text, align))->x_map ();
}
