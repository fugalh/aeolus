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
#include "editwin.h"
#include "styles.h"
#include "callbacks.h"


#define FW_X0 16
#define FW_DX 32


H_scale::H_scale (X_window *parent, X_callback *callb, int xp, int yp) :
    X_window (parent, xp, yp, 778, 18, Colors.main_bg),
    _callb (callb)
{
    x_add_events (ExposureMask | ButtonPressMask); 
    x_map ();
}


void H_scale::handle_event (XEvent *E)
{
    int           x;
    XExposeEvent *X;
    XButtonEvent *B;

    switch (E->type)
    {
    case Expose:
        X = (XExposeEvent *) E;
        if (X->count == 0) redraw ();
        break;  

    case ButtonPress:
        B = (XButtonEvent *) E;
        x = B->x - 5; 
        _i = x / 12;       
        x -= 12 * _i + 6; 
        if (_callb && (abs (x) < 6)) _callb->handle_callb (CB_SC_HARM, this, E);
        break;  
    }
}


void H_scale::redraw (void)
{
    X_draw D (dpy (), win (), dgc (), xft ());
    int    i;
    char   s [4]; 

    D.setcolor (XftColors.main_fg);
    D.setfont (XftFonts.scales);
    for (i = 0; i < 64; i++)
    {
	D.move (11 + i * 12, 12);
	sprintf (s, "%d", i + 1);
	D.drawstring (s, 0);
	if (i > 8) i++;
    }
}



N_scale::N_scale (X_window *parent, X_callback *callb, int xp, int yp) :
    X_window (parent, xp, yp, 778, 18, Colors.main_bg), _callb (callb)
{
    x_add_events (ExposureMask | ButtonPressMask); 
    x_map ();
}


void N_scale::handle_event (XEvent *E)
{
    int           x;
    XExposeEvent *X;
    XButtonEvent *B;

    switch (E->type)
    {
    case Expose:
        X = (XExposeEvent *) E;
        if (X->count == 0) redraw ();
        break;  

    case ButtonPress:
        B = (XButtonEvent *) E;
        x = B->x + 8; 
        _i = x / FW_DX;       
        x -= _i * FW_DX + FW_DX / 2; 
        if (_callb && (abs (x) < 10)) _callb->handle_callb (CB_SC_NOTE, this, E);
        break;  
    }
}


void N_scale::redraw (void)
{
    X_draw D (dpy (), win (), dgc (), xft ());
    int    i;
    char   s [4]; 

    D.setcolor (XftColors.main_fg);
    D.setfont (XftFonts.scales);
    for (i = 0; i <= 10 ; i++)
    {
	sprintf (s, "%d", 36 + 6 * i);
	D.move (FW_X0 + i * FW_DX, 12);
	D.drawstring (s, 0);
    }
}




const char *Editwin::_pftb_text [N_PFTB] = 
{
    "32", "16", "10 2/3", "8", "5 1/3", "4", "2 2/3", "2", "1 3/5", "1 1/3", "1"
};

const char Editwin::_fn [N_PFTB] = { 1,1,3,1,3,2,3,4,5,6,8 };
const char Editwin::_fd [N_PFTB] = { 4,2,4,1,2,1,1,1,1,1,1 };



Editwin::Editwin (X_window *parent, X_callback *callb, int xp, int yp, X_resman *xresm) :
    X_window (parent, xp, yp, 100, 100, Colors.main_bg),
    _callb (callb),
    _xresm (xresm)
{
    int  i, x, y;
    X_hints    H;
    X_window  *W;

    _atom = XInternAtom (dpy (), "WM_DELETE_WINDOW", True);
    XSetWMProtocols (dpy (), win (), &_atom, 1);
    _atom = XInternAtom (dpy (), "WM_PROTOCOLS", True);

    _xs = 841;
    _ys = 700;
    H.position (xp, yp);
    x_apply (&H); 

    _lock = 0;
    but1.size.x = 150;
    but1.size.y = 20;
    _tabb [0] = new X_tbutton (this, this, &but1, 0,   0, "General / Vol / Tune", 0, TAB_GEN);
    _tabb [1] = new X_tbutton (this, this, &but1, 150, 0, "Harmonics - Levels",   0, TAB_LEV);
    _tabb [2] = new X_tbutton (this, this, &but1, 300, 0, "Harmonics - Attack",   0, TAB_ATT);
    _tabb [3] = new X_tbutton (this, this, &but1, 450, 0, "Harmonics - Random",   0, TAB_RAN);
    for (i = 0; i < 4; i++) _tabb [i]->x_map ();     
    but1.size.x = 80;
    _appl = new X_tbutton (this, this, &but1, 600, 0, "Apply", 0, B_APPL);
    _appl->x_map ();
    _moff = new X_tbutton (this, this, &but1, 680, 0, "Midi Off", 0, B_MOFF);
    _moff->x_map ();


    _tabw [0] = W = new X_window (this, 0, 20, _xs, _ys, Colors.main_bg);
    y = 20;
    add_text (W, 30, y, 90, 20, "File", &text0);
    _file = new X_textip (W, this, &texti, 120, y, 300, 20, 63);
    _file->x_map ();
    _save = new X_tbutton (W, this, &but1, 440, y, "Save", 0, B_SAVE);
    _save->x_map ();
    _load = new X_tbutton (W, this, &but1, 520, y, "Load", 0, B_LOAD);
    _load->x_map ();
    _lnew = new X_tbutton (W, this, &but1, 600, y, "New",  0, B_LNEW);
    _lnew->x_map ();
    y += 30;
    add_text (W, 30, y, 90, 20, "Name", &text0);
    _name = new X_textip (W, this, &texti, 120, y, 300, 20, 31, _file);
    _name->x_map ();
    _mnem = new X_textip (W, this, &texti, 440, y, 100, 20, 7, _name);
    _mnem->x_map ();
    y += 30;
    add_text (W, 30, y, 90, 20, "Copyright", &text0);
    _copy = new X_textip (W, this, &texti, 120, y, 500, 20, 55, _mnem);
    _copy->x_map ();
    y += 30;
    add_text (W, 30, y, 90, 20, "Comments", &text0);
    _comm = new X_textip (W, this, &texti, 120, y, 500, 20, 63, _copy);
    _comm->x_map ();
    _file->callb_modified ();
    _name->callb_modified ();
    _mnem->callb_modified ();
    _copy->callb_modified ();
    _comm->callb_modified ();

    y += 40;
    x = 28;
    but1.size.x = 60;
    for (i = 0; i < N_PFTB; i++)     
    {
        _pftb [i] = new X_tbutton (W, this, &but1, x, y, _pftb_text [i], 0, B_PFTB + i);
	_pftb [i]->x_map ();
        x += 60;
    }
    _pedal = new X_tbutton (W, this, &but1, x + 20, y, "Pedal", 0, B_PEDAL);
    _pedal->x_map ();

    x = 0;
    y += 40;
    add_text (W,  28 + x, y,  80, 20, "Volume", &text1);
    add_text (W, 240 + x, y, 150, 20, "Instability (c)",  &text2);
    _vol_fun = new Functionwin (W, this, x + 28, y + 20, Colors.func_bg, Colors.func_gr, Colors.func_mk);
    _vol_fun->set_xparam (11, FW_X0, FW_DX);
    _vol_fun->set_yparam (0, &sca_dBsm, XftColors.func_d1->pixel);
    _vol_fun->set_yparam (1, &sca_Tu4,  XftColors.func_d2->pixel);
    _vol_fun->show ();
    _vol_nsc = new N_scale (W, this, x + 28, y + 20 + _vol_fun->ys ());
    _vol_nsc->x_map ();
    (new X_vscale (W, &sca_dBsm, x, y + 20, 28))->x_map ();
    (new X_vscale (W, &sca_Tu4, x + 28 + _vol_fun->xs (), y + 20, 28))->x_map ();

    x += 420;
    add_text (W,  28 + x, y, 150, 20, "Tuning offset (Hz)", &text1);
    add_text (W, 240 + x, y, 150, 20, "Random error (Hz)",  &text2);
    _tun_fun = new Functionwin (W, this, x + 28, y + 20, Colors.func_bg, Colors.func_gr, Colors.func_mk);
    _tun_fun->set_xparam (11, FW_X0, FW_DX);
    _tun_fun->set_yparam (0, &sca_Tu1, XftColors.func_d1->pixel);
    _tun_fun->set_yparam (1, &sca_Tu2, XftColors.func_d2->pixel);
    _tun_fun->show ();
    _tun_nsc = new N_scale (W, this, x + 28, y + 20 + _tun_fun->ys ());
    _tun_nsc->x_map ();
    (new X_vscale (W, &sca_Tu1, x, y + 20, 28))->x_map ();
    (new X_vscale (W, &sca_Tu2, x + 28 + _tun_fun->xs (), y + 20, 28))->x_map ();

    x = 0;
    y += 45 + _tun_fun->ys (); 
    add_text (W,  28 + x, y, 150, 20, "Attack time (ms)",  &text1);
    add_text (W, 240 + x, y, 150, 20, "Attack detune (c)", &text2);
    _atu_fun = new Functionwin (W, this, x + 28, y + 20, Colors.func_bg, Colors.func_gr, Colors.func_mk);
    _atu_fun->set_xparam (11, FW_X0, FW_DX);
    _atu_fun->set_yparam (0, &sca_Tatt, XftColors.func_d1->pixel);
    _atu_fun->set_yparam (1, &sca_Tu3,  XftColors.func_d2->pixel);
    _atu_fun->show ();
    _atu_nsc = new N_scale (W, this, x + 28, y + 20 + _atu_fun->ys ());
    _atu_nsc->x_map ();
    (new X_vscale (W, &sca_Tatt, x, y + 20, 28))->x_map ();
    (new X_vscale (W, &sca_Tu3, x + 28 + _atu_fun->xs (), y + 20, 28))->x_map ();

    x += 420;
    add_text (W,  28 + x, y, 150, 20, "Decay time (ms)",  &text1);
    add_text (W, 240 + x, y, 150, 20, "Decay detune (c)", &text2);
    _dtu_fun = new Functionwin (W, this, x + 28, y + 20, Colors.func_bg, Colors.func_gr, Colors.func_mk);
    _dtu_fun->set_xparam (11, FW_X0, FW_DX);
    _dtu_fun->set_yparam (0, &sca_Tatt, XftColors.func_d1->pixel);
    _dtu_fun->set_yparam (1, &sca_Tu3,  XftColors.func_d2->pixel);
    _dtu_fun->show ();
    _dtu_nsc = new N_scale (W, this, x + 28, y + 20 + _dtu_fun->ys ());
    _dtu_nsc->x_map ();
    (new X_vscale (W, &sca_Tatt, x, y + 20, 28))->x_map ();
    (new X_vscale (W, &sca_Tu3,  x + 28 + _dtu_fun->xs (), y + 20, 28))->x_map ();

    y += 45 + _dtu_fun->ys (); 
    _tabh [0] = y + 20;


    _tabw [1] = W = new X_window (this, 0, 20, _xs, _ys, Colors.main_bg);
    y = 15;
    _lev_msl = new Multislider (W, this, 28, y, Colors.func_gr, Colors.func_mk);
    _lev_msl->set_xparam (64, 5, 12, 7);
    _lev_msl->set_yparam (&sca_dBlg, 0);     
    _lev_msl->set_colors (XftColors.func_d0->pixel, XftColors.func_d1->pixel);
    _lev_msl->show ();
    _lev_hsc = new H_scale (W, this, 28, y + 306);
    _lev_hsc->x_map ();
    (new X_vscale (W, &sca_dBlg, 0, y, 28))->x_map ();
    y += 335;
    _lev_fun = new Functionwin (W, this, 28, y, Colors.func_bg, Colors.func_gr, Colors.func_mk);
    _lev_fun->set_xparam (11, FW_X0, FW_DX);
    _lev_fun->set_yparam (0, &sca_dBlg, XftColors.func_d1->pixel);
    _lev_fun->show ();
    _lev_nsc = new N_scale (W, this, 28, y + 306);
    _lev_nsc->x_map ();
    (new X_vscale (W, &sca_dBlg, 0, y, 28))->x_map ();
    add_text (W, 600, y, 150, 20, "Harmonic level (dB)", &text1);
    y += 335;
    _tabh [1] = y + 20;

        
    _tabw [2] = W = new X_window (this, 0, 20, _xs, _ys, Colors.main_bg);
    y = 15;
    _att_msl = new Multislider (W, this, 28, y, Colors.func_gr, Colors.func_mk);
    _att_msl->set_xparam (64, 5, 12, 7);
    _att_msl->set_yparam (&sca_Tatt, 0);     
    _att_msl->set_colors (XftColors.func_d0->pixel, XftColors.func_d1->pixel);
    _att_msl->show ();
    _att_hsc = new H_scale (W, this, 28, y + 201);
    _att_hsc->x_map ();
    (new X_vscale (W, &sca_Tatt, 0, y, 28))->x_map ();
    y += 230;
    _atp_msl = new Multislider (W, this, 28, y, Colors.func_gr, Colors.func_mk);
    _atp_msl->set_xparam (64, 5, 12, 7);
    _atp_msl->set_yparam (&sca_Patt, 1);     
    _atp_msl->set_colors (XftColors.func_d0->pixel, XftColors.func_d2->pixel);
    _atp_msl->show ();
    _atp_hsc = new H_scale (W, this, 28, y + 201);
    _atp_hsc->x_map ();
    (new X_vscale (W, &sca_Patt, 0, y, 28))->x_map ();
    y += 230;
    _att_fun = new Functionwin (W, this, 28, y, Colors.func_bg, Colors.func_gr, Colors.func_mk);
    _att_fun->set_xparam (11, FW_X0, FW_DX);
    _att_fun->set_yparam (0, &sca_Tatt, XftColors.func_d1->pixel);
    _att_fun->set_yparam (1, &sca_Patt, XftColors.func_d2->pixel);
    _att_fun->show ();
    _att_nsc = new N_scale (W, this, 28, y + 201);
    _att_nsc->x_map ();
    (new X_vscale (W, &sca_Tatt,                    0, y, 28))->x_map ();
    (new X_vscale (W, &sca_Patt, 28 + _att_fun->xs (), y, 28))->x_map ();
    add_text (W, 600, y, 150, 20, "Attack time (ms)", &text1);
    add_text (W, 600, y + 20, 150, 20, "Attack peak (dB)", &text2);
    y += 230;
    _tabh [2] = y + 20;


    _tabw [3] = W = new X_window (this, 0, 20, _xs, _ys, Colors.main_bg);
    y = 15;
    _ran_msl = new Multislider (W, this, 28, y, Colors.func_gr, Colors.func_mk);
    _ran_msl->set_xparam (64, 5, 12, 7);
    _ran_msl->set_yparam (&sca_0_12, 0);     
    _ran_msl->set_colors (XftColors.func_d0->pixel, XftColors.func_d1->pixel);
    _ran_msl->show ();
    _ran_hsc = new H_scale (W, this, 28, y + 201);
    _ran_hsc->x_map ();
    (new X_vscale (W, &sca_0_12, 0, y, 28))->x_map ();
    y += 230;
    _ran_fun = new Functionwin (W, this, 28, y, Colors.func_bg, Colors.func_gr, Colors.func_mk);
    _ran_fun->set_xparam (11, FW_X0, FW_DX);
    _ran_fun->set_yparam (0, &sca_0_12, XftColors.func_d1->pixel);
    _ran_fun->show ();
    _ran_nsc = new N_scale (W, this, 28, y + 201);
    _ran_nsc->x_map ();
    (new X_vscale (W, &sca_0_12, 0, y, 28))->x_map ();
    add_text (W, 600, y, 150, 20, "Random level (dB)", &text1);
    y += 230;
    _tabh [3] = y + 20;
    _ctab = -1;
    _cpft = -1;
}



Editwin::~Editwin (void)
{
}


void Editwin::handle_event (XEvent *E)
{
    switch (E->type)
    {
    case ClientMessage:
        handle_xmesg ((XClientMessageEvent *) E);
        break;
    }
}


void Editwin::handle_xmesg (XClientMessageEvent *E)
{
    if (_appl->stat ()) _callb->handle_callb (CB_EDIT_APP, this, 0);
    _callb->handle_callb (CB_EDIT_END, this, 0);
}


void Editwin::handle_mesg (ITC_mesg *M)
{
}


void Editwin::handle_time (void)
{
    if (_lock) _appl->set_stat (! _appl->stat ());
}


void Editwin::handle_callb (int k, X_window *W, XEvent *E )
{
    if (_lock) return;
    switch (k)
    {
        case BUTTON | X_button::RELSE:
        {
            X_button *B = (X_button *) W;
            XButtonEvent *Z = (XButtonEvent *) E;

            switch  (B->cbid ())
	    {
	    case TAB_GEN:
	    case TAB_LEV:
	    case TAB_ATT:
	    case TAB_RAN:
	        set_tab (B->cbid () - TAB_GEN);
                break; 

            case B_PEDAL:
                _save->set_stat (1);
                _appl->set_stat (1);
                if (_pedal->stat ()) { _pedal->set_stat (0); _edit->_n1 = 96; } 
                else                 { _pedal->set_stat (1); _edit->_n1 = 67; } 
                break;

	    case B_PFTB:
	    case B_PFTB+1:
	    case B_PFTB+2:
	    case B_PFTB+3:
	    case B_PFTB+4:
	    case B_PFTB+5:
	    case B_PFTB+6:
	    case B_PFTB+7:
	    case B_PFTB+8:
	    case B_PFTB+9:
	    case B_PFTB+10:
	        set_pft (B->cbid () - B_PFTB);
                _save->set_stat (1);
                _appl->set_stat (1);
                break; 

            case B_APPL:
                strcpy (_edit->_stopname, _name->text ());
                _callb->handle_callb (CB_EDIT_APP, this, 0);
                break;

            case B_MOFF:
                _callb->handle_callb (CB_GLOB_MOFF, this, 0);
		break;

            case B_SAVE:
                save (_sdir);
                break;

            case B_LOAD:
                load (_sdir);
                _callb->handle_callb (CB_EDIT_APP, this, 0);
                break;

            case B_LNEW:
                if (Z->state & ShiftMask)
		{
                    _save->set_stat (0);
                    _edit->reset ();
                    init (_edit);
		}
                break;
            }
            break;
        }

        case TEXTIP | X_textip::BUT:
            XSetInputFocus (dpy (), W->win (), RevertToParent, CurrentTime);
            break;

        case TEXTIP | X_textip::KEY:
           break;

        case TEXTIP | X_textip::MODIF:
           _save->set_stat (1);
           break;

        case CB_SC_HARM: 
        case CB_MS_SEL: 
	{
            int h;

            if (k == CB_SC_HARM) h = ((H_scale *) W)->get_ind ();
   	    else                 h = ((Multislider *) W)->get_ind ();

            switch (_ctab)
	    {
            case TAB_LEV:
                 set_harm (&_edit->_h_lev, _lev_msl, _lev_fun, 0, _lev_harm = h);
                 break;
            case TAB_ATT:
                 set_harm (&_edit->_h_att, _att_msl, _att_fun, 0, _att_harm = h);
                 set_harm (&_edit->_h_atp, _atp_msl, _att_fun, 1, _att_harm = h);
                 break;
            case TAB_RAN:
                 set_harm (&_edit->_h_ran, _ran_msl, _ran_fun, 0, _ran_harm = h);
                 break;
	    }              
            break;
	}  
        case CB_SC_NOTE: 
        case CB_FW_SEL:
	{
            int n;
            
            if (k == CB_SC_NOTE) n = ((N_scale *) W)->get_ind ();
            else                 n = ((Functionwin *) W)->get_ind ();

            switch (_ctab)
	    {
            case TAB_LEV:
                 set_note (&_edit->_h_lev, _lev_msl, _lev_fun, _lev_note = n);
                 break;
            case TAB_ATT:
                 set_note (&_edit->_h_att, _att_msl, _att_fun, _att_note = n);
                 set_note (&_edit->_h_atp, _atp_msl, _att_fun, _att_note = n);
                 break;
            case TAB_RAN:
                 set_note (&_edit->_h_ran, _ran_msl, _ran_fun, _ran_note = n);
                 break;
	    }              
            break;
	}  
        case CB_MS_UPD:
        case CB_MS_UND:
	{
            Multislider *M = (Multislider *) W;
            int d = (k != CB_MS_UND);

            switch (_ctab)
	    {
            case TAB_LEV:
		msl_update (&_edit->_h_lev, _lev_msl, _lev_fun, 0, d, _lev_harm, _lev_note);
                break;
            case TAB_ATT:
		if (M == _att_msl) msl_update (&_edit->_h_att, _att_msl, _att_fun, 0, d, _att_harm, _att_note);
		else               msl_update (&_edit->_h_atp, _atp_msl, _att_fun, 1, d, _att_harm, _att_note);
                break;
            case TAB_RAN:
		msl_update (&_edit->_h_ran, _ran_msl, _ran_fun, 0, d, _ran_harm, _ran_note);
                break;
	    }              
            _save->set_stat (1);
            _appl->set_stat (1);
            break;
	}
        case CB_FW_UPD:
        case CB_FW_DEF:
        case CB_FW_UND:
        {
            Functionwin *F = (Functionwin *) W;
            int f = F->get_fun ();
            int d = (k != CB_FW_UND);

            switch (_ctab)
	    {
            case TAB_GEN:
                if (F == _vol_fun)
		{
		    if (f) fun_update (&_edit->_n_ins, F, d);
		    else   fun_update (&_edit->_n_vol, F, d);
		}
                else if (F == _tun_fun)
		{
		    if (f) fun_update (&_edit->_n_ran, F, d); 
		    else   fun_update (&_edit->_n_off, F, d); 
		}
                else if (F == _atu_fun)
		{
		    if (f) fun_update (&_edit->_n_atd, F, d); 
		    else   fun_update (&_edit->_n_att, F, d); 
		}
                else if (F == _dtu_fun)
		{
		    if (f) fun_update (&_edit->_n_dcd, F, d); 
		    else   fun_update (&_edit->_n_dct, F, d); 
		}
                break;
            case TAB_LEV:
		fun_update (&_edit->_h_lev, _lev_msl, _lev_fun, d, _lev_harm, _lev_note);
                break;
            case TAB_ATT:
		if (f) fun_update (&_edit->_h_atp, _atp_msl, _att_fun, d, _att_harm, _att_note);
                else   fun_update (&_edit->_h_att, _att_msl, _att_fun, d, _att_harm, _att_note);
                break;
            case TAB_RAN:
		fun_update (&_edit->_h_ran, _ran_msl, _ran_fun, d, _ran_harm, _ran_note);
                break;
	    }              
            _save->set_stat (1);
            _appl->set_stat (1);
            break;
	}
    }
}


void Editwin::set_func (N_func *D, Functionwin *F, int k)
{
    F->reset (k);
    for (int i = 0; i < N_NOTE; i++) if (D->st (i)) F->set_point (k, i, D->vs (i));
    F->redraw ();
}


void Editwin::set_harm (HN_func *D, Multislider *M, Functionwin *F, int k, int h)
{
    F->reset (k);
    for (int i = 0; i < N_NOTE; i++) if (D->st (h, i)) F->set_point (k, i, D->vs (h, i));
    F->redraw ();
    M->set_mark (h);
}


void Editwin::set_note (HN_func *D, Multislider *M, Functionwin *F, int n)
{
    for (int i = 0; i < N_HARM; i++) M->set_val (i, D->st (i, n), D->vs (i, n));
    F->set_mark (n);
}


void Editwin::msl_update (HN_func *D, Multislider *M, Functionwin *F, int k, int d, int h, int n)
{
    int   i = M->get_ind ();
    float v = M->get_val ();
   
    if (d) D->setv (i, n, v);
    else   D->clrv (i, n);  
    M->set_val (i, D->st (i, n), D->vs (i, n));
    if (i == h)
    {
	if (D->st (i, n)) F->upd_point (k, n, v);
	else              F->clr_point (k, n);
    }
}    


void Editwin::fun_update (HN_func *D, Multislider *M, Functionwin *F, int d, int h, int n)
{
    int   i = F->get_ind ();
    float v = F->get_val ();

    if (d) D->setv (h, i, v);
    else   D->clrv (h, i);  
    if (i == n) M->set_val (h, d, D->vs (h, n));
}


void Editwin::fun_update (N_func *D, Functionwin *F, int d)
{
    int   i = F->get_ind ();
    float v = F->get_val ();

    if (d) D->setv (i, v);
    else   D->clrv (i);  
}


void Editwin::init (Addsynth *synth)
{
    int   i;
    char  s [256];

    _edit = synth;
    _lev_harm = 0;
    _lev_note = 4;
    _att_harm = 0;
    _att_note = 4;
    _ran_harm = 0;
    _ran_note = 4;
    _file->set_text (_edit->_filename);
    _name->set_text (_edit->_stopname);
    _mnem->set_text (_edit->_mnemonic);
    _copy->set_text (_edit->_copyrite);
    _comm->set_text (_edit->_comments);

    for (i = 0; i < N_PFTB; i++)
    {
	if (_edit->_fn == _fn [i] && _edit->_fd == _fd [i])
	{
	    set_pft (i);
            break;
	}
    }
    if (i == N_PFTB) set_pft (3);

    _pedal->set_stat ((_edit->_n1 == 96) ? 0 : 1);

    set_func (&_edit->_n_vol, _vol_fun, 0);
    set_func (&_edit->_n_ins, _vol_fun, 1);
    set_func (&_edit->_n_off, _tun_fun, 0);
    set_func (&_edit->_n_ran, _tun_fun, 1);
    set_func (&_edit->_n_att, _atu_fun, 0);
    set_func (&_edit->_n_atd, _atu_fun, 1);
    set_func (&_edit->_n_dct, _dtu_fun, 0);
    set_func (&_edit->_n_dcd, _dtu_fun, 1);
    set_note (&_edit->_h_lev, _lev_msl, _lev_fun, _lev_note);
    set_harm (&_edit->_h_lev, _lev_msl, _lev_fun, 0, _lev_harm);
    set_note (&_edit->_h_att, _att_msl, _att_fun, _att_note);
    set_note (&_edit->_h_atp, _atp_msl, _att_fun, _att_note);
    set_harm (&_edit->_h_att, _att_msl, _att_fun, 0, _att_harm);
    set_harm (&_edit->_h_atp, _atp_msl, _att_fun, 1, _att_harm);
    set_note (&_edit->_h_ran, _ran_msl, _ran_fun, _ran_note);
    set_harm (&_edit->_h_ran, _ran_msl, _ran_fun, 0, _ran_harm);

    sprintf (s, "Aeolus-%s    Additive synthesis editor", VERSION);
    x_set_title (s);
    set_tab (0);
    x_mapraised ();
}


void Editwin::load (const char *sdir)
{
    _save->set_stat (0);
    _load->set_stat (1);
    XFlush (dpy ());  
    strcpy (_edit->_filename, _file->text ());
    _edit->load (sdir);
    init (_edit);
    _file->callb_modified ();
    _name->callb_modified ();
    _mnem->callb_modified ();
    _copy->callb_modified ();
    _comm->callb_modified ();
    _load->set_stat (0);
    _appl->set_stat (1);
}


void Editwin::save (const char *sdir)
{
    _save->set_stat (1);
    XFlush (dpy ());  
    strcpy (_edit->_filename, _file->text ());
    strcpy (_edit->_stopname, _name->text ());
    strcpy (_edit->_mnemonic, _mnem->text ());
    strcpy (_edit->_copyrite, _copy->text ());
    strcpy (_edit->_comments, _comm->text ());
    _file->callb_modified ();
    _name->callb_modified ();
    _mnem->callb_modified ();
    _copy->callb_modified ();
    _comm->callb_modified ();
    _edit->save (sdir);
    _save->set_stat (0);
}


void Editwin::lock (int lock)
{
    _lock = lock;
    if (! lock) _appl->set_stat (0);
}


void Editwin::add_text (X_window *win, int xp, int yp, int xs, int ys, const char *text, X_textln_style *style)
{
    (new X_textln (win, style, xp, yp, xs, ys, text, -1))->x_map ();
}


void Editwin::set_tab (int tab)
{
    if (_ctab == tab) return;
    if (_ctab >= 0)
    {
        _tabb [_ctab]->set_stat (0);
        _tabw [_ctab]->x_unmap ();
    }
    _ctab = tab;
    x_resize (_xs, _tabh [tab]);
    _tabb [tab]->set_stat (1);
    _tabw [tab]->x_map ();
}


void Editwin::set_pft (int pft)
{
    if (_cpft == pft) return;
    if (_cpft >= 0) _pftb [_cpft]->set_stat (0);
    _cpft = pft;
    _pftb [pft]->set_stat (1);
    _edit->_fn = _fn [pft]; 
    _edit->_fd = _fd [pft]; 
}

