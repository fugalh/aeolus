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


#ifndef __NIFACE_H
#define __NIFACE_H

#include <curses.h>
#include "iface.h"
#include "model.h"

// Forward declaration
class Niface;

class NITCHandler : public H_thread
{
public:

    NITCHandler (Niface *niface);
    virtual ~NITCHandler (void);

private:

    virtual void thr_main (void);
    Niface *_niface;
};


class Niface : public Iface
{
    friend class NITCHandler;
    
public:

    Niface (int ac, char *av []);
    virtual ~Niface (void);
    virtual void stop (void);

private:

    virtual void thr_main (void);

    void handle_mesg (ITC_mesg *);
    void handle_time (void);
    void handle_ifc_ready (void);
    void handle_ifc_init (M_ifc_init *);
    void handle_ifc_mcset (M_ifc_chconf *);
    void handle_ifc_retune (M_ifc_retune *);
    void handle_ifc_grclr (M_ifc_ifelm *);
    void handle_ifc_elclr (M_ifc_ifelm *);
    void handle_ifc_elset (M_ifc_ifelm *);
    void handle_ifc_elatt (M_ifc_ifelm *);

    // NCurses specific methods
    void init_ncurses (void);
    void cleanup_ncurses (void);
    void handle_input (void);
    void handle_input_msg (void);
    void handle_key (int ch);
    void draw_screen (void);
    void draw_status (void);
    void draw_groups (void);
    void draw_cursor (void);
    void move_cursor (int dy, int dx);
    void toggle_current_stop (void);
    void recall_preset (int preset);
    void store_preset (int preset);
    void general_cancel (void);
    void enter_command_mode (void);
    void handle_resize (void);
    
    // Layout and display helpers
    void calculate_layout (void);
    void get_stop_position (int group, int ifelm, int *y, int *x);
    int get_stop_color (int group, int ifelm);
    void rewrite_label (const char *p, char *dest, int maxlen);

    bool            _stop;
    bool            _init;
    bool            _ready;
    bool            _command_mode;
    bool            _need_redraw;
    M_ifc_init     *_initdata;
    M_ifc_chconf   *_mididata;
    uint32_t        _ifelms [NGROUP];
    
    // NCurses state
    WINDOW         *_main_win;
    WINDOW         *_status_win;
    int             _cursor_group;
    int             _cursor_ifelm;
    int             _max_rows;
    int             _max_cols;
    int             _groups_per_row;
    int             _group_width;
    char            _tempstr [64];
    char            _command_buffer [256];
    int             _command_pos;
    
    // Tuning state
    bool            _has_tuning_stop;
    int             _tuning_group;
    int             _tuning_ifelm;
    bool            _tuning_blink_state;
    
    // ITC event handling
    NITCHandler     *_itc_handler;
};


#endif