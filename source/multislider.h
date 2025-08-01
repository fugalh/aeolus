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


#ifndef __MULTISLIDER_H
#define __MULTISLIDER_H


#include <clxclient.h>


namespace aeolus_x11 {

class Multislider : public X_window
{
public:

    Multislider (X_window *parent, X_callback *callb, int xp, int yp, unsigned long grid, unsigned long mark);
    ~Multislider (void);

    virtual void handle_event (XEvent *xe);

    void set_xparam (int n, int x0, int dx, int wx);
    void set_yparam (X_scale_style *scale, int rind);
    void set_colors (unsigned long col1, unsigned long col2) { _col1 = col1, _col2 = col2; }
    void show (void);

    void set_val (int i, int c, float v);
    void set_col (int i, int c);
    int   get_ind (void) { return _ind; }
    int   get_col (void) { return _col; }
    float get_val (void) { return _val; }
    void set_mark (int i);

private:

    void expose (XExposeEvent *E);
    void redraw (void);
    void plot_grid (void);
    void plot_mark (int);
    void plot_bars (void);
    void plot_1bar (int i);
    void update_val (int i, int y);
    void update_bar (int i, int y);
    void undefine_val (int i);
    void bpress (XButtonEvent *E);
    void motion (XPointerMovedEvent *E);
    void brelse (XButtonEvent *E);

    X_callback     *_callb;
    X_scale_style  *_scale;
    unsigned long   _bgnd;
    unsigned long   _col1;
    unsigned long   _col2;
    unsigned long   _grid;
    unsigned long   _mark;
    int             _xs;
    int             _ys;
    int             _n;
    int             _ymin;
    int             _ymax;
    int             _x0;
    int             _dx;
    int             _wx;
    int             _yr;
    int            *_yc;
    char           *_st;
    int             _move;
    int             _draw;
    int             _im;
    int             _ind;
    int             _col;
    float           _val;
};

} // namespace aeolus_x11


#endif
