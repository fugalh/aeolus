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


#ifndef __FUNCTIONWIN_H
#define __FUNCTIONWIN_H


#include <clxclient.h>


class Functionwin : public X_window
{
public:

    Functionwin (X_window *parent, X_callback *callb, int xp, int yp, 
                 unsigned long bgnd, unsigned long grid, unsigned long mark);
    ~Functionwin (void);
 
    void set_xparam (int n, int x0, int dx);
    void set_yparam (int k, X_scale_style *scale, unsigned long color);
    void show (void);

    void reset (int k);
    void set_point (int k, int i, float v);
    void upd_point (int k, int i, float v);
    void clr_point (int k, int i);
    int   get_ind (void) const { return _ic; }
    int   get_fun (void) const { return _fc; }
    float get_val (void) const { return _vc; }   
    void set_mark (int i);
    void redraw (void);

    int xs (void) const { return _xs; }
    int ys (void) const { return _ys; }

private:

    enum { NFUNC = 2 };

    virtual void handle_event (XEvent *xe);

    void expose (XExposeEvent *E);
    void bpress (XButtonEvent *E);
    void motion (XPointerMovedEvent *E);
    void brelse (XButtonEvent *E);
    void plot_grid (void);
    void plot_mark (void);
    void plot_line (int);
    void move_point (int);
    void move_curve (int);
    void find_sect (int *);
    void move_sect (int *);

    X_callback     *_callb; 
    unsigned long   _bgnd;
    unsigned long   _grid;
    unsigned long   _mark;
    int             _xs;
    int             _ys;
    int             _x0;
    int             _dx;
    int             _ymin;
    int             _ymax;
    int             _n;
    unsigned long   _co [NFUNC];
    X_scale_style  *_sc [NFUNC];
    int            *_yc [NFUNC];
    char           *_st [NFUNC];
    int             _fc;
    int             _ic;
    int             _im;
    float           _vc;
};


#endif
