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


#ifndef __STYLES_H
#define __STYLES_H


#include <clxclient.h>


struct colors
{
    unsigned long   white;
    unsigned long   black;
    unsigned long   spla_bg;
    unsigned long   main_bg;
    unsigned long   main_ds;
    unsigned long   main_ls;
    unsigned long   text_bg;
    unsigned long   text_hl;
    unsigned long   text_ca;
    unsigned long   slid_kn;
    unsigned long   slid_mk;
    unsigned long   butt_bg0;
    unsigned long   butt_bg1;
    unsigned long   ife0_bg0;
    unsigned long   ife0_bg1;
    unsigned long   ife1_bg0;
    unsigned long   ife1_bg1;
    unsigned long   ife2_bg0;
    unsigned long   ife2_bg1;
    unsigned long   ife3_bg0;
    unsigned long   ife3_bg1;
    unsigned long   func_bg;
    unsigned long   func_gr;
    unsigned long   func_mk;
    unsigned long   midi_bg;
    unsigned long   midi_gr1;
    unsigned long   midi_gr2;
    unsigned long   midi_co1;
    unsigned long   midi_co2;
    unsigned long   midi_co3;
};


struct xftcolors
{
    XftColor   *white;
    XftColor   *black;
    XftColor   *spla_fg;
    XftColor   *main_fg;
    XftColor   *text_fg;
    XftColor   *butt_fg0;
    XftColor   *butt_fg1;
    XftColor   *ife0_fg0;
    XftColor   *ife0_fg1;
    XftColor   *ife1_fg0;
    XftColor   *ife1_fg1;
    XftColor   *ife2_fg0;
    XftColor   *ife2_fg1;
    XftColor   *ife3_fg0;
    XftColor   *ife3_fg1;
    XftColor   *func_d0;
    XftColor   *func_d1;
    XftColor   *func_d2;
    XftColor   *midi_fg;
};


struct fonts 
{
};


struct xftfonts 
{
    XftFont   *spla1;
    XftFont   *spla2;
    XftFont   *main;
    XftFont   *large;
    XftFont   *stops;
    XftFont   *button;
    XftFont   *scales;
    XftFont   *midimt;
};


extern struct colors    Colors;
extern struct fonts     Fonts;
extern struct xftcolors XftColors;
extern struct xftfonts  XftFonts;
extern X_scale_style    sca_dBsh, sca_dBsm, sca_dBlg, sca_Tatt, sca_Patt, sca_0_12, sca_Tu1, sca_Tu2, sca_Tu3, sca_Tu4;
extern X_scale_style    sca_azim, sca_difg, sca_size, sca_trev, sca_spos, sca_Tfr, sca_Tmd, sca_Swl;
extern X_button_style   ife0, ife1, ife2, ife3, but1, but2;
extern X_menuwin_style  menu1;
extern X_textln_style   text0, text1, text2, texti, textc;
extern X_slider_style   sli1; 

extern void init_styles (X_display *disp, X_resman *xrm);


#endif
