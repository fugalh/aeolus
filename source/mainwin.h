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


#ifndef __MAINWIN_H
#define __MAINWIN_H


#include <clxclient.h>
#include "messages.h"


class Group
{
public:

    const char   *_label;
    int           _nifelm;
    X_tbutton    *_butt [32];
    int           _ylabel;
    int           _ydivid;
};



class Splashwin : public X_window
{
public:

    Splashwin (X_window *parent, int xp, int yp);
    ~Splashwin (void);

    enum { XSIZE = 500, YSIZE = 300 };

private:

    virtual void handle_event (XEvent *);
    
    void expose (XExposeEvent *);
};



class Mainwin : public X_window, public X_callback 
{
public:

    Mainwin (X_window *parent, X_callback *callb, int xp, int yp, X_resman *xresm);
    ~Mainwin (void);

    void handle_time (void);    
    void setup (M_ifc_init *); 
    void set_ifelm (M_ifc_ifelm *M);
    void set_state (M_ifc_preset *M);
    void set_ready (void);
    void set_label (int group, int ifelm, const char *label);
    ITC_mesg *mesg (void) const { return _mesg; }
 
private:

    enum
    {
	B_DECB, B_INCB, B_DECM, B_INCM,
        B_MRCL, B_PREV, B_NEXT, B_MSTO, B_MINS, B_MDEL, B_CANC,
        GROUP_BIT0 = 8, GROUP_STEP = (1 << GROUP_BIT0), GROUP_MASK = (GROUP_STEP - 1),
    }; 
           
    virtual void handle_event (XEvent *);
    virtual void handle_callb (int, X_window *, XEvent *);

    void xcmesg (XClientMessageEvent *);
    void expose (XExposeEvent *);
    void add_text (int xp, int yp, int xs, int ys, const char *text, X_textln_style *style);
    void clr_group (Group *);
    void set_butt (void);
    void upd_pres (void);

    Atom            _atom;
    X_callback     *_callb;
    X_resman       *_xresm;
    Splashwin      *_splash; 
    ITC_mesg       *_mesg;
    int             _xsize;
    int             _ysize;
    int             _count;
    int             _ngroup;
    Group           _groups [NGROUP];
    uint32_t        _st_mod [NGROUP];
    uint32_t        _st_loc [NGROUP];
    int             _group;
    int             _ifelm;
    X_button       *_flashb;
    int             _flashg;
    int             _flashi;
    bool            _local;
    int             _b_mod;
    int             _p_mod;
    int             _b_loc;
    int             _p_loc;

    X_button       *_b_decb;
    X_button       *_b_incb;
    X_button       *_b_decm;
    X_button       *_b_incm;
    X_textip       *_t_bank;
    X_textip       *_t_pres;
    X_textip       *_t_comm;
    X_button       *_b_mrcl;   
    X_button       *_b_next;
    X_button       *_b_prev;
    X_button       *_b_svpr;
    X_button       *_b_msto;   
    X_button       *_b_mins;   
    X_button       *_b_mdel;   
    X_button       *_b_canc;   

    X_button       *_b_save;
    X_button       *_b_moff;
    X_button       *_b_insw;   
    X_button       *_b_audw;   
    X_button       *_b_midw;   
};


#endif
