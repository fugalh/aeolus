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


#include "styles.h"


struct colors     Colors;
struct fonts      Fonts;
struct xftcolors  XftColors;
struct xftfonts   XftFonts;

X_scale_style     sca_dBsm, sca_dBsh, sca_dBlg, sca_Tatt, sca_Patt, sca_0_12, sca_Tu1, sca_Tu2, sca_Tu3, sca_Tu4;
X_scale_style     sca_azim, sca_difg, sca_size, sca_trev, sca_spos, sca_Tfr, sca_Tmd, sca_Swl;
X_button_style    ife0, ife1, ife2, ife3, but1, but2;
X_textln_style    text0, text1, text2, texti, textc;
X_slider_style    sli1; 


void init_styles (X_display *disp, X_resman *xrm)
{
    XftColors.black    = disp->alloc_xftcolor ("black", 0);
    XftColors.white    = disp->alloc_xftcolor ("white", 0);
    XftColors.spla_fg  = disp->alloc_xftcolor (xrm->get (".color.spla.fg",  "blue"),      XftColors.black);
    XftColors.main_fg  = disp->alloc_xftcolor (xrm->get (".color.main.fg",  "white"),     XftColors.black);
    XftColors.text_fg  = disp->alloc_xftcolor (xrm->get (".color.text.fg",  "black"),     XftColors.black);
    XftColors.butt_fg0 = disp->alloc_xftcolor (xrm->get (".color.butt.fg0", "white"),     XftColors.white);
    XftColors.butt_fg1 = disp->alloc_xftcolor (xrm->get (".color.butt.fg1", "black"),     XftColors.black);
    XftColors.ife0_fg0 = disp->alloc_xftcolor (xrm->get (".color.ife0.fg0", "white"),     XftColors.white);
    XftColors.ife0_fg1 = disp->alloc_xftcolor (xrm->get (".color.ife0.fg1", "black"),     XftColors.black);
    XftColors.ife1_fg0 = disp->alloc_xftcolor (xrm->get (".color.ife1.fg0", "white"),     XftColors.white);
    XftColors.ife1_fg1 = disp->alloc_xftcolor (xrm->get (".color.ife1.fg1", "black"),     XftColors.black);
    XftColors.ife2_fg0 = disp->alloc_xftcolor (xrm->get (".color.ife2.fg0", "white"),     XftColors.white);
    XftColors.ife2_fg1 = disp->alloc_xftcolor (xrm->get (".color.ife2.fg1", "black"),     XftColors.black);
    XftColors.ife3_fg0 = disp->alloc_xftcolor (xrm->get (".color.ife3.fg0", "white"),     XftColors.white);
    XftColors.ife3_fg1 = disp->alloc_xftcolor (xrm->get (".color.ife3.fg1", "black"),     XftColors.black);
    XftColors.func_d0  = disp->alloc_xftcolor (xrm->get (".color.func.d0",  "gray50"),    XftColors.black);
    XftColors.func_d1  = disp->alloc_xftcolor (xrm->get (".color.func.d1",  "yellow"),    XftColors.black);
    XftColors.func_d2  = disp->alloc_xftcolor (xrm->get (".color.func.d2",  "green"),     XftColors.black);
    XftColors.midi_fg  = disp->alloc_xftcolor (xrm->get (".color.midi.fg",  "white"),     XftColors.black);

    Colors.black = disp->blackpixel ();
    Colors.white = disp->whitepixel ();
    Colors.spla_bg  = disp->alloc_color (xrm->get (".color.spla.bg",  "white"),   Colors.white);
    Colors.main_bg  = disp->alloc_color (xrm->get (".color.main.bg",  "gray30"),  Colors.black);
    Colors.main_ds  = disp->alloc_color (xrm->get (".color.main.ds",  "gray10"),  Colors.black);
    Colors.main_ls  = disp->alloc_color (xrm->get (".color.main.ls",  "gray50"),  Colors.white);
    Colors.text_bg  = disp->alloc_color (xrm->get (".color.text.bg",  "white"),   Colors.white);
    Colors.text_hl  = disp->alloc_color (xrm->get (".color.text.hl",  "white"),   Colors.white);
    Colors.text_ca  = disp->alloc_color (xrm->get (".color.text.ca",  "red"),     Colors.black);
    Colors.slid_kn  = disp->alloc_color (xrm->get (".color.slid.kn",  "yellow"),  Colors.white);
    Colors.slid_mk  = disp->alloc_color (xrm->get (".color.slid.mk",  "black"),   Colors.black);
    Colors.butt_bg0 = disp->alloc_color (xrm->get (".color.butt.bg0", "gray30"),  Colors.white);
    Colors.butt_bg1 = disp->alloc_color (xrm->get (".color.butt.bg1", "white"),   Colors.white);
    Colors.ife0_bg0 = disp->alloc_color (xrm->get (".color.ife0.bg0", "#202040"), Colors.white);
    Colors.ife0_bg1 = disp->alloc_color (xrm->get (".color.ife0.bg1", "#e0e0ff"), Colors.white);
    Colors.ife1_bg0 = disp->alloc_color (xrm->get (".color.ife1.bg0", "#203030"), Colors.white);
    Colors.ife1_bg1 = disp->alloc_color (xrm->get (".color.ife1.bg1", "#e0ffff"), Colors.white);
    Colors.ife2_bg0 = disp->alloc_color (xrm->get (".color.ife2.bg0", "#403020"), Colors.white);
    Colors.ife2_bg1 = disp->alloc_color (xrm->get (".color.ife2.bg1", "#ffffa0"), Colors.white);
    Colors.ife3_bg0 = disp->alloc_color (xrm->get (".color.ife3.bg0", "#204020"), Colors.white);
    Colors.ife3_bg1 = disp->alloc_color (xrm->get (".color.ife3.bg1", "#e0ffa0"), Colors.white);
    Colors.func_bg  = disp->alloc_color (xrm->get (".color.func.bg",  "gray10"),  Colors.white);
    Colors.func_gr  = disp->alloc_color (xrm->get (".color.func.gr",  "gray20"),  Colors.black);
    Colors.func_mk  = disp->alloc_color (xrm->get (".color.func.mk",  "red"),     Colors.black);
    Colors.midi_bg  = disp->alloc_color (xrm->get (".color.midi.bg",  "gray10"),  Colors.white);
    Colors.midi_gr1 = disp->alloc_color (xrm->get (".color.midi.gr1", "gray30"),  Colors.black);
    Colors.midi_gr2 = disp->alloc_color (xrm->get (".color.midi.gr2", "gray70"),  Colors.white);
    Colors.midi_co1 = disp->alloc_color (xrm->get (".color.midi.c01", "yellow"),  Colors.white);
    Colors.midi_co2 = disp->alloc_color (xrm->get (".color.midi.c02", "green"),   Colors.white);
    Colors.midi_co3 = disp->alloc_color (xrm->get (".color.midi.c03", "coral"),   Colors.white);

    XftFonts.spla1  = disp->alloc_xftfont (xrm->get (".font.spla1",  "serif:bold:pixelsize=20"));
    XftFonts.spla2  = disp->alloc_xftfont (xrm->get (".font.spla2",  "serif:bold:pixelsize=12"));
    XftFonts.main   = disp->alloc_xftfont (xrm->get (".font.main",   "luxi:pixelsize=12"));  
    XftFonts.large  = disp->alloc_xftfont (xrm->get (".font.large",  "serif:bold:pixelsize=16"));
    XftFonts.stops  = disp->alloc_xftfont (xrm->get (".font.stops",  "serif:bold:pixelsize=13"));
    XftFonts.button = disp->alloc_xftfont (xrm->get (".font.button", "luxi:pixelsize=12"));
    XftFonts.scales = disp->alloc_xftfont (xrm->get (".font.scales", "luxi:pixelsize=9"));
    XftFonts.midimt = disp->alloc_xftfont (xrm->get (".font.midimt", "luxi:bold:pixelsize=9"));

    text0.font = XftFonts.main;
    text0.color.normal.bgnd = Colors.main_bg;
    text0.color.normal.text = XftColors.main_fg;

    text1.font = XftFonts.main;
    text1.color.normal.bgnd = Colors.main_bg;
    text1.color.normal.text = XftColors.func_d1;

    text2.font = XftFonts.main;
    text2.color.normal.bgnd = Colors.main_bg;
    text2.color.normal.text = XftColors.func_d2;

    texti.font = XftFonts.main;
    texti.color.normal.bgnd = Colors.text_bg;
    texti.color.normal.text = XftColors.text_fg;
    texti.color.focus.bgnd  = Colors.text_hl;
    texti.color.focus.text  = XftColors.text_fg;
    texti.color.focus.line  = Colors.text_ca;
    texti.color.shadow.lite = Colors.main_ls;
    texti.color.shadow.dark = Colors.main_ds;

    textc.font = XftFonts.main;
    textc.color.normal.bgnd = Colors.main_bg;
    textc.color.normal.text = XftColors.main_fg;
    textc.color.focus.bgnd  = Colors.text_hl;
    textc.color.focus.text  = XftColors.text_fg;
    textc.color.focus.line  = Colors.text_ca;
    textc.color.shadow.lite = Colors.main_bg;
    textc.color.shadow.dark = Colors.main_bg;

    ife0.type = X_button_style::RAISED;
    ife0.font = XftFonts.stops;
    ife0.color.bg [0] = Colors.ife0_bg0;
    ife0.color.bg [1] = Colors.ife0_bg1;
    ife0.color.fg [0] = XftColors.ife0_fg0;
    ife0.color.fg [1] = XftColors.ife0_fg1;
    ife0.color.shadow.bgnd = Colors.main_bg;
    ife0.color.shadow.lite = Colors.main_ls;
    ife0.color.shadow.dark = Colors.main_ds;
    ife0.size.x = 84;
    ife0.size.y = 42;

    ife1.type = X_button_style::RAISED;
    ife1.font = XftFonts.stops;
    ife1.color.bg [0] = Colors.ife1_bg0;
    ife1.color.bg [1] = Colors.ife1_bg1;
    ife1.color.fg [0] = XftColors.ife1_fg0;
    ife1.color.fg [1] = XftColors.ife1_fg1;
    ife1.color.shadow.bgnd = Colors.main_bg;
    ife1.color.shadow.lite = Colors.main_ls;
    ife1.color.shadow.dark = Colors.main_ds;
    ife1.size.x = 84;
    ife1.size.y = 42;

    ife2.type = X_button_style::RAISED;
    ife2.font = XftFonts.stops;
    ife2.color.bg [0] = Colors.ife2_bg0;
    ife2.color.bg [1] = Colors.ife2_bg1;
    ife2.color.fg [0] = XftColors.ife2_fg0;
    ife2.color.fg [1] = XftColors.ife2_fg1;
    ife2.color.shadow.bgnd = Colors.main_bg;
    ife2.color.shadow.lite = Colors.main_ls;
    ife2.color.shadow.dark = Colors.main_ds;
    ife2.size.x = 84;
    ife2.size.y = 42;

    ife3.type = X_button_style::RAISED;
    ife3.font = XftFonts.stops;
    ife3.color.bg [0] = Colors.ife3_bg0;
    ife3.color.bg [1] = Colors.ife3_bg1;
    ife3.color.fg [0] = XftColors.ife3_fg0;
    ife3.color.fg [1] = XftColors.ife3_fg1;
    ife3.color.shadow.bgnd = Colors.main_bg;
    ife3.color.shadow.lite = Colors.main_ls;
    ife3.color.shadow.dark = Colors.main_ds;
    ife3.size.x = 84;
    ife3.size.y = 42;

    but1.type = X_button_style::RAISED;
    but1.font = XftFonts.button;
    but1.color.bg [0] = Colors.butt_bg0;
    but1.color.bg [1] = Colors.butt_bg1;
    but1.color.fg [0] = XftColors.butt_fg0;
    but1.color.fg [1] = XftColors.butt_fg1;
    but1.color.shadow.bgnd = Colors.main_bg;
    but1.color.shadow.lite = Colors.main_ls;
    but1.color.shadow.dark = Colors.main_ds;
    but1.size.x = 18;
    but1.size.y = 18;

    but2.type = X_button_style::RAISED;
    but2.font = XftFonts.button;
    but2.color.bg [0] = Colors.main_bg;
    but2.color.bg [1] = Colors.main_bg;
    but2.color.fg [0] = XftColors.main_fg;
    but2.color.fg [1] = XftColors.main_fg;
    but2.color.shadow.bgnd = Colors.main_bg;
    but2.color.shadow.lite = Colors.main_ls;
    but2.color.shadow.dark = Colors.main_ds;
    but2.size.x = 18;
    but2.size.y = 18;

    sca_dBlg.bg = Colors.main_bg;
    sca_dBlg.fg = XftColors.main_fg;
    sca_dBlg.marg = 0;
    sca_dBlg.font = XftFonts.scales;
    sca_dBlg.nseg = 9;
    sca_dBlg.set_tick ( 0,  10, -100, 0);
    sca_dBlg.set_tick ( 1,  15, -80, "-80");
    sca_dBlg.set_tick ( 2,  50, -70, "-70");
    sca_dBlg.set_tick ( 3,  85, -60, "-60");
    sca_dBlg.set_tick ( 4, 120, -50, "-50");
    sca_dBlg.set_tick ( 5, 155, -40, "-40");
    sca_dBlg.set_tick ( 6, 190, -30, "-30");
    sca_dBlg.set_tick ( 7, 225, -20, "-20");
    sca_dBlg.set_tick ( 8, 260, -10, "-10");
    sca_dBlg.set_tick ( 9, 295,   0, "0");

    sca_dBsm.bg = Colors.main_bg;
    sca_dBsm.fg = XftColors.func_d1;
    sca_dBsm.marg = 0;
    sca_dBsm.font = XftFonts.scales;
    sca_dBsm.nseg = 6;
    sca_dBsm.set_tick ( 0,  10, -100, 0);
    sca_dBsm.set_tick ( 1,  20,  -50, "-50");
    sca_dBsm.set_tick ( 2,  54,  -40, "-40");
    sca_dBsm.set_tick ( 3,  88,  -30, "-30");
    sca_dBsm.set_tick ( 4, 122,  -20, "-20");
    sca_dBsm.set_tick ( 5, 156,  -10, "-10");
    sca_dBsm.set_tick ( 6, 190,    0, "0");

    sca_size.bg = Colors.main_bg;
    sca_size.fg = XftColors.main_fg;
    sca_size.marg = 0;
    sca_size.font = XftFonts.scales;
    sca_size.nseg = 5;
    sca_size.set_tick ( 0,  10, 0.025, 0    );
    sca_size.set_tick ( 1,  46, 0.050, "50" );
    sca_size.set_tick ( 2,  82, 0.075, 0    );
    sca_size.set_tick ( 3, 118, 0.100, "100");
    sca_size.set_tick ( 4, 154, 0.125, 0    );
    sca_size.set_tick ( 5, 190, 0.150, "150");
 
    sca_trev.bg = Colors.main_bg;
    sca_trev.fg = XftColors.main_fg;
    sca_trev.marg = 0;
    sca_trev.font = XftFonts.scales;
    sca_trev.nseg = 5;
    sca_trev.set_tick ( 0,  10, 2, "2");
    sca_trev.set_tick ( 1,  46, 3, "3");
    sca_trev.set_tick ( 2,  82, 4, "4");
    sca_trev.set_tick ( 3, 118, 5, "5");
    sca_trev.set_tick ( 4, 154, 6, "6");
    sca_trev.set_tick ( 5, 190, 7, "7");

    sca_dBsh.bg = Colors.main_bg;
    sca_dBsh.fg = XftColors.main_fg;
    sca_dBsh.marg = 0;
    sca_dBsh.font = XftFonts.scales;
    sca_dBsh.nseg = 5;
    sca_dBsh.set_tick ( 0,  10,  0.000, 0    );
    sca_dBsh.set_tick ( 1,  18,  0.100, "-20");
    sca_dBsh.set_tick ( 2,  56,  0.178, 0    );
    sca_dBsh.set_tick ( 3,  94,  0.316, "-10");
    sca_dBsh.set_tick ( 4, 132,  0.562, 0    );
    sca_dBsh.set_tick ( 5, 170,  1.000, "0"  );

    sca_spos.bg = Colors.main_bg;
    sca_spos.fg = XftColors.main_fg;
    sca_spos.marg = 0;
    sca_spos.font = XftFonts.scales;
    sca_spos.nseg = 4;
    sca_spos.set_tick ( 0,  10, -1.0, "B");
    sca_spos.set_tick ( 1,  50, -0.5, 0);
    sca_spos.set_tick ( 2,  90,  0.0, "C");
    sca_spos.set_tick ( 3, 130,  0.5, 0);
    sca_spos.set_tick ( 4, 170,  1.0, "F");

    sca_azim.bg = Colors.main_bg;
    sca_azim.fg = XftColors.main_fg;
    sca_azim.marg = 0;
    sca_azim.font = XftFonts.scales;
    sca_azim.nseg = 4;
    sca_azim.set_tick ( 0,  10, -0.50, "B");
    sca_azim.set_tick ( 1,  50, -0.25, "L");
    sca_azim.set_tick ( 2,  90,  0.00, "F");
    sca_azim.set_tick ( 3, 130,  0.25, "R");
    sca_azim.set_tick ( 4, 170,  0.50, "B");

    sca_difg.bg = Colors.main_bg;
    sca_difg.fg = XftColors.main_fg;
    sca_difg.marg = 0;
    sca_difg.font = XftFonts.scales;
    sca_difg.nseg = 4;
    sca_difg.set_tick ( 0,  10,  0.00, "0");
    sca_difg.set_tick ( 1,  50,  0.25, 0);
    sca_difg.set_tick ( 2,  90,  0.50, "0.5");
    sca_difg.set_tick ( 3, 130,  0.75, 0);
    sca_difg.set_tick ( 4, 170,  1.00, "1");

    sca_Tatt.bg = Colors.main_bg;
    sca_Tatt.fg = XftColors.func_d1;
    sca_Tatt.marg = 0;
    sca_Tatt.font = XftFonts.scales;
    sca_Tatt.nseg = 6;
    sca_Tatt.set_tick ( 0,  10, 0.010,  "10");
    sca_Tatt.set_tick ( 1,  40, 0.025,  "25");
    sca_Tatt.set_tick ( 2,  70, 0.050,  "50");
    sca_Tatt.set_tick ( 3, 100, 0.100, "100");
    sca_Tatt.set_tick ( 4, 130, 0.200, "200");
    sca_Tatt.set_tick ( 5, 160, 0.300, "300");
    sca_Tatt.set_tick ( 6, 190, 0.400, "400");

    sca_Patt.bg = Colors.main_bg;
    sca_Patt.fg = XftColors.func_d2;
    sca_Patt.marg = 0;
    sca_Patt.font = XftFonts.scales;
    sca_Patt.nseg = 5;
    sca_Patt.set_tick ( 0,  10, -3.0, "-3");
    sca_Patt.set_tick ( 1,  46,  0.0,  "0");
    sca_Patt.set_tick ( 2,  82,  3.0,  "3");
    sca_Patt.set_tick ( 3, 118,  6.0,  "6");
    sca_Patt.set_tick ( 4, 154, 19.0,  "9");
    sca_Patt.set_tick ( 5, 190, 12.0, "12");

    sca_0_12.bg = Colors.main_bg;
    sca_0_12.fg = XftColors.func_d1;
    sca_0_12.marg = 0;
    sca_0_12.font = XftFonts.scales;
    sca_0_12.nseg = 6;
    sca_0_12.set_tick ( 0,  10,  0.0, "0");
    sca_0_12.set_tick ( 1,  40,  2.0, "2");
    sca_0_12.set_tick ( 2,  70,  4.0, "4");
    sca_0_12.set_tick ( 3, 100,  6.0, "6");
    sca_0_12.set_tick ( 4, 130,  8.0, "8");
    sca_0_12.set_tick ( 5, 160, 10.0, "10");
    sca_0_12.set_tick ( 6, 190, 12.0, "12");

    sca_Tu1.bg = Colors.main_bg;
    sca_Tu1.fg = XftColors.func_d1;
    sca_Tu1.marg = 0;
    sca_Tu1.font = XftFonts.scales;
    sca_Tu1.nseg = 6;
    sca_Tu1.set_tick ( 0,  10, -6.0, "-6");
    sca_Tu1.set_tick ( 1,  40, -4.0, "-4");
    sca_Tu1.set_tick ( 2,  70, -2.0, "-2");
    sca_Tu1.set_tick ( 3, 100,  0.0,  "0");
    sca_Tu1.set_tick ( 4, 130,  2.0,  "2");
    sca_Tu1.set_tick ( 5, 160,  4.0,  "4");
    sca_Tu1.set_tick ( 6, 190,  6.0,  "6");

    sca_Tu2.bg = Colors.main_bg;
    sca_Tu2.fg = XftColors.func_d2;
    sca_Tu2.marg = 0;
    sca_Tu2.font = XftFonts.scales;
    sca_Tu2.nseg = 6;
    sca_Tu2.set_tick ( 0,  10,  0.0, "0");
    sca_Tu2.set_tick ( 1,  40,  0.1, "0.1");
    sca_Tu2.set_tick ( 2,  70,  0.2, "0.2");
    sca_Tu2.set_tick ( 3, 100,  0.5, "0.5");
    sca_Tu2.set_tick ( 4, 130,  1.0, "1.0");
    sca_Tu2.set_tick ( 5, 160,  2.0, "2.0");
    sca_Tu2.set_tick ( 6, 190,  5.0, "5.0");

    sca_Tu3.bg = Colors.main_bg;
    sca_Tu3.fg = XftColors.func_d2;
    sca_Tu3.marg = 0;
    sca_Tu3.font = XftFonts.scales;
    sca_Tu3.nseg = 6;
    sca_Tu3.set_tick ( 0,  10, -60.0, "-60");
    sca_Tu3.set_tick ( 1,  40, -40.0, "-40");
    sca_Tu3.set_tick ( 2,  70, -20.0, "-20");
    sca_Tu3.set_tick ( 3, 100,   0.0,   "0");
    sca_Tu3.set_tick ( 4, 130,  20.0,  "20");
    sca_Tu3.set_tick ( 5, 160,  40.0,  "40");
    sca_Tu3.set_tick ( 6, 190,  60.0,  "60");

    sca_Tu4.bg = Colors.main_bg;
    sca_Tu4.fg = XftColors.func_d2;
    sca_Tu4.marg = 0;
    sca_Tu4.font = XftFonts.scales;
    sca_Tu4.nseg = 6;
    sca_Tu4.set_tick ( 0,  10, -0.01f,  0);
    sca_Tu4.set_tick ( 1,  20,  0.0,  "0");
    sca_Tu4.set_tick ( 2,  54,  2.0,  "2");
    sca_Tu4.set_tick ( 3,  88,  4.0,  "4");
    sca_Tu4.set_tick ( 4, 122,  6.0,  "6");
    sca_Tu4.set_tick ( 5, 156,  8.0,  "8");
    sca_Tu4.set_tick ( 6, 190, 10.0, "10");

    sca_Tfr.bg = Colors.main_bg;
    sca_Tfr.fg = XftColors.main_fg;
    sca_Tfr.marg = 0;
    sca_Tfr.font = XftFonts.scales;
    sca_Tfr.nseg = 6;
    sca_Tfr.set_tick ( 0,  10,  2.0, "2");
    sca_Tfr.set_tick ( 1,  30,  3.0, "3");
    sca_Tfr.set_tick ( 2,  50,  4.0, "4");
    sca_Tfr.set_tick ( 3,  70,  5.0, "5");
    sca_Tfr.set_tick ( 4,  90,  6.0, "6");
    sca_Tfr.set_tick ( 5, 110,  7.0, "7");
    sca_Tfr.set_tick ( 6, 130,  8.0, "8");

    sca_Tmd.bg = Colors.main_bg;
    sca_Tmd.fg = XftColors.main_fg;
    sca_Tmd.marg = 0;
    sca_Tmd.font = XftFonts.scales;
    sca_Tmd.nseg = 6;
    sca_Tmd.set_tick ( 0,  10,  0.0, "0ff");
    sca_Tmd.set_tick ( 1,  30,  0.1,     0);
    sca_Tmd.set_tick ( 2,  50,  0.2, "0.2");
    sca_Tmd.set_tick ( 3,  70,  0.3,     0);
    sca_Tmd.set_tick ( 4,  90,  0.4, "0.4");
    sca_Tmd.set_tick ( 5, 110,  0.5,     0);
    sca_Tmd.set_tick ( 6, 130,  0.6, "0.6");

    sca_Swl.bg = Colors.main_bg;
    sca_Swl.fg = XftColors.main_fg;
    sca_Swl.marg = 0;
    sca_Swl.font = XftFonts.scales;
    sca_Swl.nseg = 4;
    sca_Swl.set_tick ( 0,  10,  0.00, "C");
    sca_Swl.set_tick ( 1,  40,  0.25,   0);
    sca_Swl.set_tick ( 2,  70,  0.50, "H");
    sca_Swl.set_tick ( 3, 100,  0.75,   0);
    sca_Swl.set_tick ( 4, 130,  1.00, "O");

    sli1.bg   = Colors.main_bg;
    sli1.lite = Colors.main_ls;
    sli1.dark = Colors.main_ds;
    sli1.knob = Colors.slid_kn;
    sli1.mark = Colors.slid_mk;
    sli1.h = 19;
    sli1.w = 10; 
}
