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


#include <stdlib.h>
#include <stdio.h>
#include "multislider.h"
#include "callbacks.h"
#include "styles.h"



Multislider::Multislider (X_window *parent, X_callback *callb, int xp, int yp,
                          unsigned long grid, unsigned long mark) :
    X_window (parent, xp, yp, 100, 100, Colors.func_bg),
    _callb (callb),
    _bgnd (Colors.func_bg),
    _grid (grid), _mark (mark), _yc (0), _st (0), 
    _move (-1), _draw (-1)
{
    x_add_events (ExposureMask | ButtonMotionMask | ButtonPressMask | ButtonReleaseMask);
    x_set_bit_gravity (NorthWestGravity);
    _im = -1;
}


Multislider::~Multislider (void)
{
    delete[] _yc;
    delete[] _st;
}


void Multislider::handle_event (XEvent *E)
{
    switch (E->type)
    {
    case Expose:
        expose ((XExposeEvent *) E);
        break;  

    case ButtonPress:
        bpress ((XButtonEvent *) E);
        break;  

    case MotionNotify:
        motion ((XPointerMovedEvent *) E);
        break;  

    case ButtonRelease:
        brelse ((XButtonEvent *) E);
        break;  

    default:
	printf ("Multilsider::event %d\n", E->type);
    }
}


void Multislider::set_xparam (int n, int x0, int dx, int wx)
{
    _n = n;
    _x0 = x0;
    _dx = dx;
    _wx = wx;
    _xs = _n * _dx + 2 * x0;
    delete[] _yc;
    delete[] _st;
    _yc = new int [n];
    _st = new char [n];
}


void Multislider::set_yparam (X_scale_style *scale, int iref)
{
    _scale = scale;
    _ys = _scale->pixlen ();
    _yr = _ys - 1 - _scale->pix [iref];
    _ymax = _ys - 1 - _scale->pix [0];
    _ymin = _ys - 1 - _scale->pix [_scale->nseg];
    for (int i = 0; i < _n; i++)
    {
        _yc [i] = _yr;
        _st [i] = 255;
    } 
}


void Multislider::show (void)
{
    x_resize (_xs, _ys);
    x_map ();   
}


void Multislider::expose (XExposeEvent *E)
{
    if (E->count) return;
    redraw ();
}


void Multislider::redraw (void)
{
    plot_grid ();
    plot_mark (1);
    plot_bars ();
}


void Multislider::set_mark (int i)
{
    if (_im != i)
    {
	plot_mark (0);
        _im = i;
        plot_mark (1);
    }
}


void Multislider::plot_grid (void)
{
    int    i, x;
    X_draw D (dpy (), win (), dgc (), 0);

    D.setfunc (GXcopy);
    D.setcolor (_grid);

    for (i = 0; i <= _scale->nseg; i++)
    {
	D.move (0, _ys - _scale->pix [i] - 1);
        D.rdraw (_xs, 0);
    }
    x = _x0 + _dx / 2;
    for (i = 0; i < _n; i++)
    {
        D.move (x, 0);
        D.rdraw (0, _ys); 
        x += _dx;
    }

    D.setcolor (Colors.main_ds);
    D.move (0, _ys);
    D.draw (0, 0);
    D.draw (_xs, 0);
}


void Multislider::plot_mark (int k)
{
    X_draw D (dpy (), win (), dgc (), 0);
    int    x, y;

    if (_im >= 0)
    {
        x = _x0 + _im * _dx + _dx / 2;
        y = _yc [_im];
        D.setfunc (GXcopy);
        D.setcolor (k ? _mark : _grid);
        D.move (x, _ys);
        D.draw (x, (y < _yr) ? _yr + 1 : y + 1);
        D.move (x, 0);
        D.draw (x, (y < _yr) ? y + 1 : _yr + 1);
    }
}


void Multislider::plot_bars (void)
{
    int    i, x, y;
    X_draw D (dpy (), win (), dgc (), 0);

    D.setfunc (GXcopy);
    x = _dx / 2 + _x0;
    x -= _wx / 2;
    for (i = 0; i < _n; i++)
    {
        D.setcolor (_st [i] ? _col2 : _col1);
	y = _yc [i];
        if (y < _yr) D.fillrect (x, y, x + _wx, _yr + 1);
        else         D.fillrect (x, _yr, x + _wx, y + 1);
        x += _dx;
    }
}


void Multislider::plot_1bar (int i)
{
    int    x, y;
    X_draw D (dpy (), win (), dgc (), 0);

    D.setfunc (GXcopy);
    D.setcolor (_st [i] ? _col2 : _col1);
    x = _dx / 2 + _x0 + i * _dx;
    x -= _wx / 2;
    y = _yc [i];
    if (y < _yr) D.fillrect (x, y, x + _wx, _yr + 1);
    else         D.fillrect (x, _yr, x + _wx, y + 1);
}


void Multislider::update_val (int i, int y)
{

    if (y < _ymin) y = _ymin;
    if (y > _ymax) y = _ymax;
    update_bar (i, y);
    if (_callb)
    {
        _ind = i;
        _val = _scale->calcval (_ys - y - 1);    
        _callb->handle_callb (CB_MS_UPD, this, 0);
    }
}


void Multislider::undefine_val (int i)
{
    if (_callb && _st [i])
    {
        _ind = i;
        _callb->handle_callb (CB_MS_UND, this, 0);
    }
}


void Multislider::update_bar (int i, int y)
{
    int  x, yc, ya0, ya1, ye0, ye1;
    X_draw D (dpy (), win (), dgc (), 0);

    D.setfunc (GXcopy);
    yc = _yc [i];
    if (y == yc) return;
    _yc [i] = y;
    x = i * _dx + _dx / 2 + _x0;
    x -= _wx / 2;

    if (y > yc)
    {
	ya0 = ya1 = y + 1;
        ye0 = ye1 = yc;
        if      (_yr < yc) ya0 = yc + 1;
        else if (_yr > y) ye1 = y;
        else
	{
	    ya0 = _yr + 1;
            ye1 = _yr;
	}    
    }
    else
    {
        ya0 = ya1 = y;
        ye0 = ye1 = yc + 1;
        if      (_yr > yc) ya1 = yc;
        else if (_yr < y) ye0 = y + 1;
        else
	{
	    ya1 = _yr;
            ye0 = _yr + 1;
	}    
    }

    if (ya0 != ya1)
    {
        D.setcolor (_st [i] ? _col2 : _col1);
        D.fillrect (x, ya0, x + _wx, ya1);
    }
    if (ye0 != ye1)
    {
        D.setcolor (_bgnd);
        D.fillrect (x, ye0, x + _wx, ye1);
        D.setcolor ((i == _im) ? _mark : _grid);
        D.move (x + _wx / 2, ye0);
        D.rdraw (0, ye1 - ye0);        
        D.setcolor (_grid);
        for (i = 0; i <= _scale->nseg; i++)
	{
	    y = _ys - _scale->pix [i] - 1; 
            if (y >=  ye1) continue;
            if (y <   ye0) break;
            D.move (x, y);
            D.draw (x + _wx, y);
	}
    }
}


void Multislider::bpress (XButtonEvent *E)
{
    int i, x;

    x = E->x - _x0;
    i = x / _dx;
    if ((i < 0) || (i >= _n)) return;
    x -= i * _dx + _dx / 2;
    if (E->button == Button3)
    {
        _draw = i;   
        if (E->state & ControlMask) undefine_val (i);
        else                        update_val (i, E->y);
    }
    else if (2 * abs (x) <= _wx)
    {
        if (E->state & ControlMask) undefine_val (i);
        else                        update_val (_move = i, E->y);
        if (_callb)
        {
            _ind = i;
            _callb->handle_callb (CB_MS_SEL, this, 0);
	}
    }
}


void Multislider::motion (XPointerMovedEvent *E)
{
    int i, x, y;

    if (_move >= 0) update_val (_move, E->y);
    else if (_draw >= 0)
    {
        x = E->x - _x0;
        i = x / _dx;
        if ((i < 0) || (i >= _n)) return;
        x -= i * _dx + _dx / 2;
        if (2 * abs (x) > _wx) return;
        
        if (E->state & ControlMask)
	{
            undefine_val (i);
	}
        else
        {
	    y = (E->state & ShiftMask) ? _yc [_draw] : E->y;
	    update_val (i, y);
	}
    } 
}


void Multislider::brelse (XButtonEvent *E)
{
    _move = -1;
    _draw = -1;
}


void Multislider::set_col (int i, int c)
{
    if (c != _st [i])
    {
	_st [i] = c;
        plot_1bar (i);
    }
}


void Multislider::set_val (int i, int c, float v)
{
    int y = _ys - 1 - _scale->calcpix (v);

    if (c == _st [i]) update_bar (i, y);
    else
    {
	update_bar (i, _yr);
        _st [i] = c;
        _yc [i] = y;
        plot_1bar (i);         
    }
}

