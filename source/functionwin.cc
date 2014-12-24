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
#include "functionwin.h"
#include "callbacks.h"
#include "styles.h"



Functionwin::Functionwin (X_window *parent, X_callback *callb, int xp, int yp,
                          unsigned long bgnd, unsigned long grid, unsigned long mark) :
    X_window (parent, xp, yp, 100, 100, bgnd),
    _callb (callb), _bgnd (bgnd), _grid (grid), _mark (mark)
{
    x_add_events (ExposureMask | ButtonMotionMask | ButtonPressMask | ButtonReleaseMask);
    x_set_bit_gravity (NorthWestGravity);
    for (int i = 0; i < NFUNC; i++)
    {
        _sc [i] = 0;
	_yc [i] = 0;
	_st [i] = 0;
    }
    _fc = 0;
    _ic = -1;
    _im = -1;
}


Functionwin::~Functionwin (void)
{
    for (int i = 0; i < NFUNC; i++)
    {
	delete[] _yc [i];
	delete[] _st [i];
    }
}


void Functionwin::handle_event (XEvent *E)
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


void Functionwin::set_xparam (int n, int x0, int dx)
{
    _n = n;
    _x0 = x0;
    _dx = dx;
    _xs = 2 * _x0 + (_n - 1) * _dx;
}


void Functionwin::set_yparam (int k, X_scale_style *scale, unsigned long color)
{
    if (k == 0)
    {
        _ys = scale->pixlen ();
        _ymax = _ys - 1 - scale->pix [0];
        _ymin = _ys - 1 - scale->pix [scale->nseg];
    }
    _sc [k] = scale;
    _co [k] = color;
    delete[] _yc [k];
    delete[] _st [k];
    _yc [k] = new int [_n];
    _st [k] = new char [_n];
    reset (k);
}


void Functionwin::show (void)
{
    x_resize (_xs, _ys);
    x_map ();   
}


void Functionwin::reset (int k)
{
    for (int i = 0; i < _n; i++)
    {
	_yc [k][i] = _ymax;
        _st [k][i] = 0;  
    }
}


void Functionwin::set_point (int k, int i, float v)
{
    if (_sc [k])
    {
	_st [k][i] = 1;
        _yc [k][i] = _ys - 1 - _sc [k]->calcpix (v);
    }
}


void Functionwin::upd_point (int k, int i, float v)
{
    if (_sc [k])
    {
        plot_line (k);
	_st [k][i] = 1;
        _yc [k][i] = _ys - 1 - _sc [k]->calcpix (v);
        plot_line (k);
    }
}


void Functionwin::clr_point (int k, int i)
{
    if (_sc [k])
    {
        plot_line (k);
	_st [k][i] = 0;
        plot_line (k);
    }
}


void Functionwin::set_mark (int i)
{
    if (_im != i)
    {
       plot_mark ();
       _im = i;
       plot_mark ();
    }        
}


void Functionwin::expose (XExposeEvent *E)
{
    if (E->count) return;
    redraw ();
}


void Functionwin::redraw (void)
{
    plot_grid ();
    plot_mark ();
    for (int i = 0; i < NFUNC; i++) if (_sc [i]) plot_line (i);
}


void Functionwin::bpress (XButtonEvent *E)
{
    int   i, j, n, x, y;
    int  *yc;
    char *st;

    x = E->x - _x0;
    y = E->y;
    i = (x + _dx / 2) / _dx;
    if ((i < 0) || (i >= _n)) return;
    if (abs (x - i * _dx) > 8) return;   

    yc = _yc [_fc];
    st = _st [_fc];

    if (E->state & ControlMask)
    {
	if (st [i])
	{
            for (j = n = 0; j < _n; j++) if (st [j]) n++;
	    if ((n > 1) && (abs (y - yc [i]) <= 8))
	    {
                plot_line (_fc);
                st [i] = 0;
                plot_line (_fc);
                if (_callb)
		{
                    _ic = i;
                    _vc = _sc [_fc]->calcval (_ys - 1 - y);
                    _callb->handle_callb (CB_FW_SEL, this, 0);
                    _callb->handle_callb (CB_FW_UND, this, 0);
                    _ic = -1;
		}                
	    }
	}
        else
	{
           plot_line (_fc);
           if (y > _ymax) y = _ymax;
           if (y < _ymin) y = _ymin;
           yc [i] = y;
           st [i] = 1;
           plot_line (_fc);
           if (_callb)
	   {
               _ic = i;
               _vc = _sc [_fc]->calcval (_ys - 1 - y);
               _callb->handle_callb (CB_FW_SEL, this, 0);
               _callb->handle_callb (CB_FW_DEF, this, 0);
	   }
	}
    }
    else
    {
	for (j = 0; j < NFUNC; j++)
	{
	    if (_sc [j] && _st [j][i] && (abs (_yc [j][i] - y) <= 8))
            {
		_fc = j;
                _ic = i;     
                if (_callb) _callb->handle_callb (CB_FW_SEL, this, 0);
                break;                
	    }
	}
    }
}


void Functionwin::motion (XPointerMovedEvent *E)
{
    if (_ic >= 0)
    {
       if (E->state & Button3Mask) move_curve (E->y);
       else  move_point (E->y);
    }
}


void Functionwin::brelse (XButtonEvent *E)
{
    _ic = -1;
}


void Functionwin::plot_grid (void)
{
    int    i, x;
    X_draw D (dpy (), win (), dgc (), 0);

    D.clearwin ();
    D.setfunc (GXcopy);
    D.setcolor (_grid);

    for (i = 0; i <= _sc [0]->nseg; i++)
    {
	D.move (0, _ys - _sc [0]->pix [i] - 1);
        D.rdraw (_xs, 0);
    }
    x = _x0;
    for (i = 0; i <= 10; i++)
    {
        D.move (x, 0);
        D.rdraw (0, _ys); 
        x += _dx;
    }

    D.setcolor (Colors.main_ds);
    D.move (0, _ys);
    D.draw (0,0);
    D.draw (_xs, 0);
}


void Functionwin::plot_mark (void)
{
    X_draw D (dpy (), win (), dgc (), 0);

    if (_im >= 0)
    {
        D.setfunc (GXxor);
        D.setcolor (_mark ^ _grid);
        D.move (_x0 + _im * _dx, 0);
        D.rdraw (0, _ys);
    }
}


void Functionwin::plot_line (int k)
{
    int    i0, i1, x0, x1; 
    int   *yc;
    char  *st;
    X_draw D (dpy (), win (), dgc (), 0);

    yc = _yc [k];
    st = _st [k];
    D.setcolor (_co [k] ^ _bgnd);
    D.setfunc (GXxor);

    x0 = x1 = _x0;
    i0 = 0;
    if (st [0]) D.drawrect (x0 - 4, yc [i0] - 4, x0 + 4, yc [i0] + 4);
    for (i1 = 1; i1 < _n; i1++)
    {
        x1 += _dx;
	if (st [i1])
	{
	    if (st [i0]) D.move (x0, yc [i0]);
    	    else         D.move (x0, yc [i1]);
            D.draw (x1, yc [i1]);
            D.drawrect (x1 - 4, yc [i1] - 4, x1 + 4, yc [i1] + 4);
            x0 = x1;
            i0 = i1;
	}    
    }
    if (x1 != x0)
    {
         D.move (x0, yc [i0]);
         D.draw (x1, yc [i0]);
    }
}


void Functionwin::move_point (int y)
{
    plot_line (_fc);
    if (y > _ymax) y = _ymax;
    if (y < _ymin) y = _ymin;
    _yc [_fc][_ic] = y;
    plot_line (_fc);
    if (_callb)
    {
	_vc = _sc [_fc]->calcval (_ys - 1 - y);
        _callb->handle_callb (CB_FW_UPD, this, 0);
    }
}


void Functionwin::move_curve (int y)
{
    int i, j, dy;
    int  *yc = _yc [_fc];
    char *st = _st [_fc];

    plot_line (_fc); 
    if (y > _ymax) y = _ymax;
    if (y < _ymin) y = _ymin;
    dy = y - yc [_ic];
    for (j = 0; j < _n; j++)
    {
        if (st [j])
	{
   	    y = yc [j] + dy;
            if (y > _ymax) y = _ymax;
            if (y < _ymin) y = _ymin;
            yc [j] = y;
	}
    }
    plot_line (_fc); 
    if (_callb)
    {
        i = _ic;
        for (j = 0; j < _n; j++)
        {
	    if (st [j])
            {
		 _ic = j;
		 _vc = _sc [_fc]->calcval (_ys - 1 - yc [j]);
		 _callb->handle_callb (CB_FW_UPD, this, 0);
	    }
	}
        _ic = i;
    }
}


