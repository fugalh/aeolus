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


#include "mainwin.h"
#include "messages.h"
#include "callbacks.h"
#include "styles.h"


Splashwin::Splashwin (X_window *parent, int xp, int yp) :
    X_window (parent, xp, yp, XSIZE, YSIZE, Colors.spla_bg, Colors.black, 2)
{
    x_add_events (ExposureMask); 
}


Splashwin::~Splashwin (void)
{
}


void Splashwin::handle_event (XEvent *E)
{
    switch (E->type)
    {
    case Expose:
        expose ((XExposeEvent *) E);
        break;  
    }
}


void Splashwin::expose (XExposeEvent *E)
{
    int     x, y;
    char    s [256];
    X_draw  D (dpy (), win (), dgc (), xft ());

    if (E->count) return;
    x = XSIZE / 2;
    y = YSIZE / 2;

    sprintf (s, "Aeolus-%s", VERSION);
    D.setfunc (GXcopy);
    D.setfont (XftFonts.spla1);
    D.setcolor (XftColors.spla_fg);
    D.move (x, y - 50);
    D.drawstring (s, 0); 
    D.setfont (XftFonts.spla2);
    D.move (x, y);
    D.drawstring ("(C) 2003-2022 Fons Adriaensen", 0); 
    D.move (x, y + 50);
    D.drawstring ("This is free software, and you are welcome to distribute it", 0); 
    D.move (x, y + 70);
    D.drawstring ("under certain conditions. See the file COPYING for details.", 0); 
}



Mainwin::Mainwin (X_window *parent, X_callback *callb, int xp, int yp, X_resman *xresm) :
    X_window (parent, xp, yp, 100, 100, Colors.main_bg),
    _callb (callb),
    _xresm (xresm),
    _count (0),
    _flashb (0),
    _local (false)
{
    int i;

    _atom = XInternAtom (dpy (), "WM_DELETE_WINDOW", True);
    XSetWMProtocols (dpy (), win (), &_atom, 1);
    _atom = XInternAtom (dpy (), "WM_PROTOCOLS", True);
    x_add_events (ExposureMask); 
    x_set_bit_gravity (NorthWestGravity);

    for (i = 0; i < NGROUP; i++)
    {
	_st_mod [i] = 0;
	_st_loc [i] = 0;
    }
}


Mainwin::~Mainwin (void)
{
}


void Mainwin::handle_event (XEvent *E)
{
    switch (E->type)
    {
    case Expose:
        expose ((XExposeEvent *) E);
        break;  

    case ClientMessage:
        xcmesg ((XClientMessageEvent *) E);
        break;
    }
}


void Mainwin::expose (XExposeEvent *E)
{
    int     g;
    Group   *G;
    X_draw  D (dpy (), win (), dgc (), xft ());

    if (E->count) return;

    D.setfont (XftFonts.large);
    D.setfunc (GXcopy);  
    for (g = 0; g < _ngroup; g++)
    {
        G = _groups + g; 
        D.move (10, G->_ylabel );
        D.setcolor (XftColors.main_fg);
        D.drawstring (G->_label, -1);
        D.setcolor (Colors.main_ls);
        D.move (15, G->_ydivid);
        D.rdraw (_xsize - 30, 0);     
        D.setcolor (Colors.main_ds);
        D.rmove (0, -1);
        D.rdraw (30 - _xsize, 0);     
    }
}


void Mainwin::xcmesg (XClientMessageEvent *E)
{
    _callb->handle_callb (CB_MAIN_END, 0, 0);
}



void Mainwin::handle_callb (int k, X_window *W, XEvent *E)
{
    int          g, i;
    X_button     *B;
    XButtonEvent *Z;
    char         s [256];

    if (k == (BUTTON | X_button::PRESS))
    {
        B = (X_button *) W;
        k = B->cbid ();
        if (k >= CB_GLOB_SAVE) _callb->handle_callb (k, this, E);
        else if (k < GROUP_STEP)
	{
            switch (k)
	    {
            case B_DECB:
                if (_local) { if (_b_loc > 0) _b_loc--; }
                else   	    { if (_b_mod > 0) _b_mod--; }                
                upd_pres ();
		break;
 
            case B_INCB:
                if (_local) { if (_b_loc < NBANK - 1) _b_loc++; }
                else  	    { if (_b_mod < NBANK - 1) _b_mod++;	}                
                upd_pres ();
		break;
 
            case B_DECM:
                if (_local) { if (_p_loc > 0) _p_loc--; }
                else   	    { if (_p_mod > 0) _p_mod--; }                
                upd_pres ();
		break;
 
            case B_INCM:
                if (_local) { if (_p_loc < NPRES - 1) _p_loc++; }
                else  	    { if (_p_mod < NPRES - 1) _p_mod++;	}                
                upd_pres ();
		break;
 
            case B_MRCL:
                _mesg = new M_ifc_preset (MT_IFC_PRRCL, _b_mod, _p_mod, 0, 0);
                _callb->handle_callb (CB_MAIN_MSG, this, 0); 
		break;

            case B_PREV:
                _mesg = new ITC_mesg (MT_IFC_PRDEC);
                _callb->handle_callb (CB_MAIN_MSG, this, 0); 
		break;

            case B_NEXT:
                _mesg = new ITC_mesg (MT_IFC_PRINC);
                _callb->handle_callb (CB_MAIN_MSG, this, 0); 
		break;

            case B_MSTO:
                _mesg = new M_ifc_preset (MT_IFC_PRSTO, _b_mod, _p_mod, _ngroup, _st_mod);
                _callb->handle_callb (CB_MAIN_MSG, this, 0); 
                sprintf (s, "%d:%d  Stored", _b_mod + 1, _p_mod + 1);
                _t_comm->set_text (s); 
		break;

            case B_MINS:
                _mesg = new M_ifc_preset (MT_IFC_PRINS, _b_mod, _p_mod, _ngroup, _st_mod);
                _callb->handle_callb (CB_MAIN_MSG, this, 0); 
                sprintf (s, "%d:%d  Stored", _b_mod + 1, _p_mod + 1);
                _t_comm->set_text (s); 
		break;

            case B_MDEL:
                _mesg = new M_ifc_preset (MT_IFC_PRDEL, _b_mod, _p_mod, 0, 0);
                _callb->handle_callb (CB_MAIN_MSG, this, 0); 
                _t_comm->set_text (""); 
		break;

            case B_CANC:
                for (g = 0; g < _ngroup; g++)
		{
                    if (_local)
		    {
                        clr_group (_groups + g);
                        _st_loc [g] = 0;
		    }
                    else
		    {
                        _mesg = new M_ifc_ifelm (MT_IFC_GRCLR, g, 0);
                        _callb->handle_callb (CB_MAIN_MSG, this, 0);
		    } 
		}
                _t_comm->set_text (""); 
		break;
	    } 
	}
	else
	{
	    g = (k >> GROUP_BIT0) - 1;
            i = k & GROUP_MASK;
            if (_local)
	    {
                if (B->stat ())
		{
		    B->set_stat (0);
                    _st_loc [g] &= ~(1 << i);
		}
		else
		{
		    B->set_stat (1);
                    _st_loc [g] |= (1 << i);
		}
	    }
	    else
	    {
                Z = (XButtonEvent *) E;
                if (Z->state & ControlMask)
		{
		    _mesg = new M_ifc_edit (MT_IFC_EDIT, g, i, 0);
                    _callb->handle_callb (CB_MAIN_MSG, this, 0);
		}
		else
		{
      	            if (Z->button == Button3)
		    {
			_mesg = new M_ifc_ifelm (MT_IFC_GRCLR, g, 0);
			_callb->handle_callb (CB_MAIN_MSG, this, 0);
		    }
                    _mesg = new M_ifc_ifelm (MT_IFC_ELXOR, g, i);
                    _callb->handle_callb (CB_MAIN_MSG, this, 0);
		}
	    }
	}
    }
}


void Mainwin::handle_time (void)
{
    if (_count == 30) _splash->x_mapraised ();
    if (_count && ! --_count) _splash->x_unmap ();
    if (!_local && _flashb) _flashb->set_stat (_flashb->stat () ? 0 : 1);
}


void Mainwin::setup (M_ifc_init *M)
{
    int             g, i, x, y;
    Group           *G;
    X_button_style  *S;
    X_hints         H;
    char            s [256];

    _ngroup = M->_ngroup;
    y = 15;
    for (g = 0; g < _ngroup; g++)
    {
	G = _groups + g;
        G->_ylabel = y + 20;
        G->_label  = M->_groupd [g]._label;
        G->_nifelm = M->_groupd [g]._nifelm;
        x = 95;
        S = &ife0;
        for (i = 0; i < G->_nifelm; i++)
	{        
            switch (M->_groupd [g]._ifelmd [i]._type)
	    {
	    case 0: S = &ife0; break;
	    case 1: S = &ife1; break;
	    case 2: S = &ife2; break;
	    case 3: S = &ife3; break;
	    }
            if (i == 10) { x = 35; y += S->size.y + 4; }                
            if (i == 20) { x = 65; y += S->size.y + 4; }                
            G->_butt [i] = new X_tbutton (this, this, S, x, y, 0, 0, (g + 1) * GROUP_STEP + i);
            set_label (g, i, M->_groupd [g]._ifelmd [i]._label);
            G->_butt [i]->x_map ();              
            x += S->size.x + 4;             
	}
        y += S->size.y + 15;
        G->_ydivid = y;
        y += 15;    
    }

    x = _xsize = 990;

    but2.size.x = 17;
    but2.size.y = 17;
    add_text (15, y + 2,  60, 20, "Preset",   &text0);
    add_text (15, y + 24, 60, 20, "Bank",     &text0);
    (_t_pres = new X_textip  (this,    0, &text0, 80, y + 2,  40, 20,  7))->x_map ();
    (_t_bank = new X_textip  (this,    0, &text0, 80, y + 24, 40, 20,  7))->x_map ();
    (_b_decm = new X_ibutton (this, this, &but2, 125, y + 2,  disp ()->image1515 (X_display::IMG_LT), B_DECM))->x_map ();
    (_b_incm = new X_ibutton (this, this, &but2, 143, y + 2,  disp ()->image1515 (X_display::IMG_RT), B_INCM))->x_map ();
    (_b_decb = new X_ibutton (this, this, &but2, 125, y + 24, disp ()->image1515 (X_display::IMG_LT), B_DECB))->x_map ();
    (_b_incb = new X_ibutton (this, this, &but2, 143, y + 24, disp ()->image1515 (X_display::IMG_RT), B_INCB))->x_map ();

    but1.size.x = 80;
    but1.size.y = 20;
    (_b_mrcl = new X_tbutton (this, this, &but1, 244, y,      "Recall", 0, B_MRCL))->x_map ();
    (_b_prev = new X_tbutton (this, this, &but1, 328, y,      "Prev",   0, B_PREV))->x_map ();
    (_b_next = new X_tbutton (this, this, &but1, 412, y,      "Next",   0, B_NEXT))->x_map ();
    (_b_msto = new X_tbutton (this, this, &but1, 244, y + 25, "Store",  0, B_MSTO))->x_map ();
    (_b_mins = new X_tbutton (this, this, &but1, 328, y + 25, "Insert", 0, B_MINS))->x_map ();
    (_b_mdel = new X_tbutton (this, this, &but1, 412, y + 25, "Delete", 0, B_MDEL))->x_map ();
    (_b_canc = new X_tbutton (this, this, &but1, 532, y + 25, "Cancel", 0, B_CANC))->x_map ();
    (_b_save = new X_tbutton (this, this, &but1, x - 180, y,      "Save",     0, CB_GLOB_SAVE))->x_map ();
    (_b_moff = new X_tbutton (this, this, &but1, x -  96, y,      "Midi off", 0, CB_GLOB_MOFF))->x_map ();
    (_b_insw = new X_tbutton (this, this, &but1, x - 264, y + 25, "Instrum",  0, CB_SHOW_INSW))->x_map ();
    (_b_audw = new X_tbutton (this, this, &but1, x - 180, y + 25, "Audio",    0, CB_SHOW_AUDW))->x_map ();
    (_b_midw = new X_tbutton (this, this, &but1, x -  96, y + 25, "Midi",     0, CB_SHOW_MIDW))->x_map ();
    (_t_comm = new X_textip  (this,    0, &text0, 500, y, 160, 20, 15))->x_map ();

   _ysize = y + 55;
    if (_ysize < Splashwin::YSIZE + 20) _ysize = Splashwin::YSIZE + 20;

    H.position (100, 100);
    H.minsize (200, 100);
    H.maxsize (_xsize, _ysize);
    H.rname (_xresm->rname ());
    H.rclas (_xresm->rclas ());
    if (_xresm->getb (".iconic", 0)) H.state (IconicState);
    x_apply (&H); 

    sprintf (s, "%s   Aeolus-%s  [%d:%d]", M->_appid, VERSION, M->_client, M->_ipport);
    x_set_title (s);
    x_resize (_xsize, _ysize);
    _splash = new Splashwin (this, (_xsize - Splashwin::XSIZE) / 2, (_ysize - Splashwin::YSIZE) / 2); 

    _b_loc = _b_mod = 0;
    _p_loc = _p_mod = 0;
    upd_pres ();

    _count = 30;
    x_mapraised ();
}


void Mainwin::set_ifelm (M_ifc_ifelm *M)
{
    Group *G = _groups + M->_group;

    switch (M->type ())
    {
    case MT_IFC_GRCLR:
        _st_mod [M->_group] = 0;
        if (! _local) clr_group (G);
        _t_comm->set_text ("");
	break;

    case MT_IFC_ELCLR:
        _st_mod [M->_group] &= ~(1 << M->_ifelm);
        if (! _local) G->_butt [M->_ifelm]->set_stat (0);
        _t_comm->set_text ("");
	break;

    case MT_IFC_ELSET:
        _st_mod [M->_group] |= (1 << M->_ifelm);
        if (! _local) G->_butt [M->_ifelm]->set_stat (1);
        _t_comm->set_text ("");
        break;

    case MT_IFC_ELATT:
        if (!_local && _flashb) _flashb->set_stat ((_st_mod [_flashg] >> _flashi) & 1);
        _flashb = G->_butt [M->_ifelm]; 
        _flashg = M->_group;
        _flashi = M->_ifelm;
	break;
    }
}


void Mainwin::set_state (M_ifc_preset *M)
{
    char s [256];
  
    if (M->_stat)
    {
        memcpy (_st_mod, M->_bits, NGROUP * sizeof (uint32_t));
        sprintf (s, "%d:%d  Loaded", M->_bank + 1, M->_pres + 1);
        if (!_local) set_butt ();
    }
    else
    {
        sprintf (s, "%d:%d  Empty", M->_bank + 1, M->_pres + 1);
        _t_comm->set_text (s);
    }      
    _t_comm->set_text (s);
    _b_mod = M->_bank; 
    _p_mod = M->_pres; 
    if (!_local) upd_pres ();
}


void Mainwin::clr_group (Group *G)
{
    for (int i = 0; i < G->_nifelm; i++) G->_butt [i]->set_stat (0);
}


void Mainwin::set_ready (void)
{
    if (!_local && _flashb)
    {
        _flashb->set_stat ((_st_mod [_flashg] >> _flashi) & 1);
    }
    _flashb = 0;
}


void Mainwin::set_butt (void)
{
    int        g, i;
    uint32_t   b, *s;
    Group      *G;

    s = _local ? _st_loc : _st_mod;
    for (g = 0, G = _groups; g < _ngroup; g++, G++)
    {
        for (i = 0, b = *s++; i < G->_nifelm; i++, b >>= 1)
	{
	    G->_butt [i]->set_stat (b & 1);
	}
    }
}


void Mainwin::upd_pres (void)
{
    char s [80];

    sprintf (s, "%d", (_local ? _b_loc : _b_mod) + 1);
    _t_bank->set_text (s);
    sprintf (s, "%d", (_local ? _p_loc : _p_mod) + 1);
    _t_pres->set_text (s);
}


void Mainwin::set_label (int group, int ifelm, const char *label)
{
    char p [32], *q;

    strcpy (p, label);
    q = strchr (p, '$');
    if (q) *q++ = 0; 
    _groups [group]._butt [ifelm]->set_text (p, q);
}


void Mainwin::add_text (int xp, int yp, int xs, int ys, const char *text, X_textln_style *style)
{
    (new X_textln (this, style, xp, yp, xs, ys, text, -1))->x_map ();
}
