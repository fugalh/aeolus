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


#ifndef __EDITWIN_H
#define __EDITWIN_H


#include <clxclient.h>
#include "multislider.h"
#include "functionwin.h"
#include "addsynth.h"
#include "rankwave.h"


class H_scale : public X_window
{
public:

    H_scale (X_window *parent, X_callback *callb, int xp, int yp);
    int get_ind (void) { return _i; }

private:

    void handle_event (XEvent *E);
    void redraw (void);

    X_callback *_callb;
    int         _i;
};



class N_scale : public X_window
{
public:

    N_scale (X_window *parent, X_callback *callb, int xp, int yp);
    int get_ind (void) { return _i; }

private:

    void handle_event (XEvent *E);
    void redraw (void);

    X_callback *_callb;
    int         _i;
};



class Editwin : public X_window, public X_callback
{
public:

    Editwin (X_window *parent, X_callback *callb, int xp, int yp, X_resman *xresm);
    ~Editwin (void);

    void handle_mesg (ITC_mesg *);
    void handle_time (void);

    void init (Addsynth *);
    void lock (int);
    void show (void) { x_mapraised (); }
    void hide (void) { x_unmap (); }
    void sdir (const char *sdir) { _sdir = sdir; }
    void wdir (const char *wdir) { _wdir = wdir; }
 
private:

    enum {
	    TAB_GEN, TAB_LEV, TAB_ATT, TAB_RAN,
            B_APPL, B_MOFF, B_SAVE, B_LOAD, B_LNEW, B_PEDAL, B_PFTB, N_PFTB = 11
         };
 
    virtual void handle_event (XEvent *xe);
    virtual void handle_callb (int, X_window*, _XEvent*);

    void handle_xmesg (XClientMessageEvent *E);
    void set_func (N_func *D, Functionwin *F, int k);
    void set_harm (HN_func *D, Multislider *M, Functionwin *F, int k, int h);
    void set_note (HN_func *D, Multislider *M, Functionwin *F, int n);
    void msl_update (HN_func *D, Multislider *M, Functionwin *F, int k, int d, int h, int n);
    void fun_update (HN_func *D, Multislider *M, Functionwin *F, int d, int h, int n);
    void fun_update (N_func *D, Functionwin *F, int d);
    void add_text (X_window *win, int xp, int yp, int xs, int ys, const char *text, X_textln_style *style);
    void set_tab (int);
    void set_pft (int);
    void load (const char *sdir);
    void save (const char *sdir);

    Atom        _atom;
    X_callback *_callb;
    X_resman   *_xresm;
    int         _xs;
    int         _ys;
    int         _lock;    
    const char *_sdir;
    const char *_wdir;
    Addsynth   *_edit;

    X_button   *_tabb [4];
    X_window   *_tabw [4];
    int         _tabh [4];
    int         _ctab;
    X_button   *_appl;
    X_button   *_moff;

    X_textip   *_file;
    X_textip   *_name;
    X_textip   *_mnem;
    X_textip   *_copy;
    X_textip   *_comm;
    X_button   *_save;
    X_button   *_load;
    X_button   *_lnew;
    X_button   *_pedal;
    X_button   *_pftb [N_PFTB];    
    int         _cpft;

    Functionwin *_vol_fun;
    N_scale     *_vol_nsc; 
    Functionwin *_tun_fun;
    N_scale     *_tun_nsc; 
    Functionwin *_atu_fun;
    N_scale     *_atu_nsc; 
    Functionwin *_dtu_fun;
    N_scale     *_dtu_nsc; 

    Multislider *_lev_msl;
    H_scale     *_lev_hsc; 
    Functionwin *_lev_fun;
    N_scale     *_lev_nsc; 
    int          _lev_harm;
    int          _lev_note;

    Multislider *_att_msl;
    H_scale     *_att_hsc; 
    Multislider *_atp_msl;
    H_scale     *_atp_hsc; 
    Functionwin *_att_fun;
    N_scale     *_att_nsc; 
    int          _att_harm;
    int          _att_note;

    Multislider *_ran_msl;
    H_scale     *_ran_hsc; 
    Functionwin *_ran_fun;
    N_scale     *_ran_nsc; 
    int          _ran_harm;
    int          _ran_note;

    static const char  *_pftb_text [N_PFTB]; 
    static const char   _fn [N_PFTB];
    static const char   _fd [N_PFTB];
};


#endif
