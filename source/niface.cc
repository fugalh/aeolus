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


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>
#include "niface.h"


#ifndef STATIC_UI
extern "C" Iface *create_iface (int ac, char *av [])
{
    return new Niface (ac, av);
}
#endif


Niface::Niface (int ac, char *av []) :
    _stop (false),
    _init (true),
    _command_mode (false),
    _initdata (0),
    _mididata (0),
    _main_win (0),
    _status_win (0),
    _cursor_group (0),
    _cursor_ifelm (0),
    _max_rows (0),
    _max_cols (0),
    _groups_per_row (1),
    _group_width (20),
    _command_pos (0)
{
    int i;

    for (i = 0; i < NGROUP; i++) _ifelms [i] = 0;
    _command_buffer[0] = 0;
    init_ncurses ();
}


Niface::~Niface (void)
{
    cleanup_ncurses ();
}


void Niface::stop (void)
{
    _stop = true;
}


void Niface::init_ncurses (void)
{
    // Initialize ncurses
    initscr ();
    
    // Enable color if available
    if (has_colors ())
    {
        start_color ();
        // Define color pairs
        init_pair (1, COLOR_GREEN, COLOR_BLACK);   // Tremulants
        init_pair (2, COLOR_YELLOW, COLOR_BLACK);  // Couplers  
        init_pair (3, COLOR_BLACK, COLOR_WHITE);   // Cursor
        init_pair (4, COLOR_WHITE, COLOR_BLACK);   // Normal
    }
    
    // Configure terminal
    cbreak ();       // Disable line buffering
    noecho ();       // Don't echo input
    keypad (stdscr, TRUE);  // Enable function keys
    nodelay (stdscr, TRUE); // Non-blocking input
    curs_set (0);    // Hide cursor
    
    // Get screen dimensions
    getmaxyx (stdscr, _max_rows, _max_cols);
    
    // Create windows
    _main_win = newwin (_max_rows - 1, _max_cols, 0, 0);
    _status_win = newwin (1, _max_cols, _max_rows - 1, 0);
    
    // Set up signal handler for clean shutdown
    signal (SIGINT, SIG_IGN);
    signal (SIGTERM, SIG_IGN);
}


void Niface::cleanup_ncurses (void)
{
    if (_main_win) delwin (_main_win);
    if (_status_win) delwin (_status_win);
    endwin ();
}


void Niface::thr_main (void)
{
    set_time (0);
    inc_time (125000);

    while (! _stop)
    {
        switch (get_event ())
        {
        case FM_MODEL:
            handle_mesg (get_message ());
            break;

        case EV_EXIT:
            return;
            
        default:
            handle_input ();
            break;
        }
    }
    send_event (EV_EXIT, 1);
}


void Niface::handle_input (void)
{
    int ch = getch ();
    
    if (ch == ERR) return;  // No input available
    
    if (_command_mode)
    {
        switch (ch)
        {
        case 27:  // ESC
        case '\n':
        case '\r':
            if (ch == '\n' || ch == '\r')
            {
                if (strcmp (_command_buffer, "quit") == 0)
                {
                    send_event (EV_EXIT, 1);
                    return;
                }
            }
            _command_mode = false;
            _command_pos = 0;
            _command_buffer[0] = 0;
            draw_screen ();
            break;
            
        case KEY_BACKSPACE:
        case 127:
        case 8:
            if (_command_pos > 0)
            {
                _command_pos--;
                _command_buffer[_command_pos] = 0;
                draw_screen ();
            }
            break;
            
        default:
            if (isprint (ch) && _command_pos < sizeof(_command_buffer) - 1)
            {
                _command_buffer[_command_pos++] = ch;
                _command_buffer[_command_pos] = 0;
                draw_screen ();
            }
            break;
        }
        return;
    }
    
    // Normal mode key handling
    switch (ch)
    {
    case 4:  // Ctrl-D
        send_event (EV_EXIT, 1);
        break;
        
    case '/':
        enter_command_mode ();
        break;
        
    case KEY_UP:
        move_cursor (-1, 0);
        break;
        
    case KEY_DOWN:
        move_cursor (1, 0);
        break;
        
    case KEY_LEFT:
        move_cursor (0, -1);
        break;
        
    case KEY_RIGHT:
        move_cursor (0, 1);
        break;
        
    case ' ':
        toggle_current_stop ();
        break;
        
    case '0':
    case '`':
        general_cancel ();
        break;
        
    case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9':
        recall_preset (ch - '0');
        break;
        
    case '!': case '@': case '#': case '$': case '%':
    case '^': case '&': case '*': case '(':
        {
            const char *shift_keys = "!@#$%^&*(";
            const char *pos = strchr (shift_keys, ch);
            if (pos) store_preset (pos - shift_keys + 1);
        }
        break;
        
    case KEY_RESIZE:
        handle_resize ();
        break;
    }
}


void Niface::enter_command_mode (void)
{
    _command_mode = true;
    _command_pos = 0;
    _command_buffer[0] = 0;
    draw_screen ();
}


void Niface::handle_resize (void)
{
    endwin ();
    refresh ();
    getmaxyx (stdscr, _max_rows, _max_cols);
    
    if (_main_win) delwin (_main_win);
    if (_status_win) delwin (_status_win);
    
    _main_win = newwin (_max_rows - 1, _max_cols, 0, 0);
    _status_win = newwin (1, _max_cols, _max_rows - 1, 0);
    
    calculate_layout ();
    draw_screen ();
}


void Niface::calculate_layout (void)
{
    if (!_initdata) return;
    
    // Simple layout: calculate groups per row based on screen width
    _group_width = 20;  // Each group needs about 20 characters
    _groups_per_row = _max_cols / _group_width;
    if (_groups_per_row < 1) _groups_per_row = 1;
}


void Niface::draw_screen (void)
{
    if (!_main_win || !_status_win) return;
    
    werase (_main_win);
    werase (_status_win);
    
    draw_groups ();
    draw_cursor ();
    draw_status ();
    
    wrefresh (_main_win);
    wrefresh (_status_win);
}


void Niface::draw_groups (void)
{
    if (!_initdata) return;
    
    // TODO: Implement group drawing - placeholder for now
    mvwprintw (_main_win, 1, 1, "Aeolus NCurses Interface");
    mvwprintw (_main_win, 2, 1, "Groups will be displayed here");
}


void Niface::draw_cursor (void)
{
    if (!_initdata) return;
    
    // TODO: Implement cursor drawing
    mvwprintw (_main_win, 4, 1, "Cursor: Group %d, Stop %d", _cursor_group, _cursor_ifelm);
}


void Niface::draw_status (void)
{
    if (_command_mode)
    {
        mvwprintw (_status_win, 0, 0, "/%s", _command_buffer);
    }
    else
    {
        mvwprintw (_status_win, 0, 0, "Arrow keys:move, Space:toggle, 1-9:presets, /:command, Ctrl-D:quit");
    }
}


void Niface::move_cursor (int dy, int dx)
{
    // TODO: Implement proper cursor movement with bounds checking
    draw_screen ();
}


void Niface::toggle_current_stop (void)
{
    // TODO: Implement stop toggling
}


void Niface::recall_preset (int preset)
{
    // TODO: Implement preset recall
}


void Niface::store_preset (int preset)
{
    // TODO: Implement preset storage
}


void Niface::general_cancel (void)
{
    // TODO: Implement general cancel (push all stops)
}


void Niface::handle_mesg (ITC_mesg *M)
{
    switch (M->type ())
    {
    case MT_IFC_INIT:
        handle_ifc_init ((M_ifc_init *) M);
        M = 0;
        break;

    case MT_IFC_READY:
        handle_ifc_ready ();
        break;

    case MT_IFC_ELCLR:
        handle_ifc_elclr ((M_ifc_ifelm *) M);
        break;

    case MT_IFC_ELSET:
        handle_ifc_elset ((M_ifc_ifelm *) M);
        break;

    case MT_IFC_ELATT:
        handle_ifc_elatt ((M_ifc_ifelm *) M);
        break;

    case MT_IFC_GRCLR:
        handle_ifc_grclr ((M_ifc_ifelm *) M);
        break;

    case MT_IFC_AUPAR:
        break;

    case MT_IFC_DIPAR:
        break;

    case MT_IFC_RETUNE:
        handle_ifc_retune ((M_ifc_retune *) M);
        break;

    case MT_IFC_MCSET:
        handle_ifc_mcset ((M_ifc_chconf *) M);
        M = 0;
        break;

    case MT_IFC_PRRCL:
        break;

    default:
        break;
    }
    if (M) M->recover ();
}


void Niface::handle_ifc_ready (void)
{
    if (_init)
    {
        calculate_layout ();
        draw_screen ();
    }
    _init = false;
}


void Niface::handle_ifc_init (M_ifc_init *M)
{
    if (_initdata) _initdata->recover ();
    _initdata = M;
}


void Niface::handle_ifc_mcset (M_ifc_chconf *M)
{
    if (_mididata) _mididata->recover ();
    _mididata = M;
}


void Niface::handle_ifc_retune (M_ifc_retune *M)
{
    // TODO: Show retuning message in status
}


void Niface::handle_ifc_grclr (M_ifc_ifelm *M)
{
    _ifelms [M->_group] = 0;
    draw_screen ();
}


void Niface::handle_ifc_elclr (M_ifc_ifelm *M)
{
    _ifelms [M->_group] &= ~(1 << M->_ifelm);
    draw_screen ();
}


void Niface::handle_ifc_elset (M_ifc_ifelm *M)
{
    _ifelms [M->_group] |= (1 << M->_ifelm);
    draw_screen ();
}


void Niface::handle_ifc_elatt (M_ifc_ifelm *M)
{
    // TODO: Handle element attachment (retuning)
    draw_screen ();
}


void Niface::rewrite_label (const char *p, char *dest, int maxlen)
{
    strncpy (dest, p, maxlen - 1);
    dest[maxlen - 1] = 0;
    
    char *t = strstr (dest, "-$");
    if (t) strcpy (t, t + 2);
    else
    {
        t = strchr (dest, '$');
        if (t) *t = ' ';
    }
}


int Niface::get_stop_color (int group, int ifelm)
{
    if (!_initdata || group >= _initdata->_ngroup) return 4;
    
    if (ifelm >= _initdata->_groupd[group]._nifelm) return 4;
    
    int type = _initdata->_groupd[group]._ifelmd[ifelm]._type;
    
    // Color coding based on stop type
    // TODO: Define proper type constants
    switch (type)
    {
    case 1:  // Tremulant
        return 1;  // Green
    case 2:  // Coupler
        return 2;  // Yellow/Brown
    default:
        return 4;  // Normal white
    }
}


void Niface::get_stop_position (int group, int ifelm, int *y, int *x)
{
    // TODO: Calculate proper screen positions based on layout
    *y = 5 + ifelm;
    *x = 1 + (group % _groups_per_row) * _group_width;
}