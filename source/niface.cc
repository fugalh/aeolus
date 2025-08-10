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
#include <unistd.h>
#include <sys/time.h>
#include <sys/select.h>
#include <errno.h>
#include "niface.h"


#ifndef STATIC_UI
extern "C" Iface *create_iface (int ac, char *av [])
{
    return new Niface (ac, av);
}
#endif


NInputHandler::NInputHandler (Niface *niface) :
    H_thread (niface, EV_INPUT),
    _niface (niface)
{
}


NInputHandler::~NInputHandler (void)
{
}


void NInputHandler::thr_main (void)
{
    // This thread handles keyboard input using select() to avoid polling
    fd_set readfds;
    int stdin_fd = STDIN_FILENO;
    
    while (!_niface->_stop)
    {
        FD_ZERO(&readfds);
        FD_SET(stdin_fd, &readfds);
        
        // Block waiting for input on stdin
        int result = select(stdin_fd + 1, &readfds, nullptr, nullptr, nullptr);
        
        if (result < 0)
        {
            if (errno == EINTR) continue;  // Interrupted by signal, retry
            break;  // Error occurred
        }
        
        if (result > 0 && FD_ISSET(stdin_fd, &readfds))
        {
            // Input is available, signal the main thread
            reply();  // Send EV_INPUT event to main thread
        }
    }
}


NITCHandler::NITCHandler (Niface *niface) :
    H_thread (niface, EV_REDRAW),
    _niface (niface)
{
}


NITCHandler::~NITCHandler (void)
{
}


void NITCHandler::thr_main (void)
{
    // This thread handles ITC events from the model
    _niface->set_time (0);
    _niface->inc_time (125000);

    while (true)
    {
        switch (_niface->get_event ())
        {
        case FM_MODEL:
            _niface->handle_mesg (_niface->get_message ());
            // Wake up main thread to check for redraws
            reply();
            break;

        case EV_EXIT:
            _niface->_stop = true;
            return;
        }
    }
}


Niface::Niface (int ac, char *av []) :
    _stop (false),
    _init (true),
    _ready (false),
    _command_mode (false),
    _need_redraw (false),
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
    _command_pos (0),
    _has_tuning_stop (false),
    _tuning_group (0),
    _tuning_ifelm (0),
    _tuning_blink_state (false),
    _itc_handler (0),
    _input_handler (0)
{
    int i;

    for (i = 0; i < NGROUP; i++) _ifelms [i] = 0;
    _command_buffer[0] = 0;
    
    // No separate ITC handler thread - main thread handles model events directly
    _itc_handler = nullptr;
    
    // Create input handler thread (start it later in thr_main)
    _input_handler = new NInputHandler (this);
    
    // Initialize ncurses but don't draw anything yet
    init_ncurses_only ();
}


Niface::~Niface (void)
{
    cleanup_ncurses ();
    if (_itc_handler) delete _itc_handler;
    if (_input_handler) delete _input_handler;
}


void Niface::stop (void)
{
    _stop = true;
}


void Niface::init_ncurses_only (void)
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
    nodelay (stdscr, TRUE);  // Enable non-blocking getch()
    // No timeout needed for blocking input
    curs_set (0);    // Hide cursor
    
    // Get screen dimensions
    getmaxyx (stdscr, _max_rows, _max_cols);
    
    // Create windows
    _main_win = newwin (_max_rows - 1, _max_cols, 0, 0);
    _status_win = newwin (1, _max_cols, _max_rows - 1, 0);
    
    // Set reasonable defaults for layout before data arrives
    _group_width = 24;  // Wider to accommodate full stop names
    _groups_per_row = _max_cols / _group_width;
    if (_groups_per_row < 1) _groups_per_row = 1;
    
    // Set up signal handler for clean shutdown
    signal (SIGINT, SIG_IGN);
    signal (SIGTERM, SIG_IGN);
    
    // Show initial loading screen
    draw_initial_screen ();
}


void Niface::cleanup_ncurses (void)
{
    if (_main_win) delwin (_main_win);
    if (_status_win) delwin (_status_win);
    endwin ();
}


void Niface::thr_main (void)
{
    // No separate ITC handler thread - we handle model events directly
    
    // Start the input handler thread  
    _input_handler->thr_start (SCHED_OTHER, 0, 0);
    
    // Set up timing for tuning blink (~4Hz like GUI)
    set_time (0);
    inc_time (250000); // 250ms = 4Hz blink rate
    
    struct timeval last_blink = {0, 0};
    gettimeofday (&last_blink, nullptr);
    
    while (! _stop)
    {
        // Block waiting for events - input, messages, or timer
        int event = get_event_timed ();
        bool had_event = false;
        bool need_blink_update = false;
        
        switch (event)
        {
        case EV_INPUT:
            // Input is available, read it non-blocking
            {
                int ch = getch ();
                if (ch != ERR)
                {
                    handle_key (ch);
                    had_event = true;
                }
            }
            break;
            
        case FM_MODEL:
            // Model message arrived, process it directly
            handle_mesg (get_message ());
            had_event = true;
            break;
            
        case EV_TIME:
            // Timer event for tuning blink
            if (_has_tuning_stop)
            {
                _tuning_blink_state = !_tuning_blink_state;
                need_blink_update = true;
            }
            // Reset timer for next blink
            inc_time (250000);
            break;
            
        case EV_EXIT:
            _stop = true;
            break;
            
        default:
            // No event or other event, continue
            break;
        }
        
        // Always check if screen needs redrawing (could be set by NITCHandler)
        if (had_event || _need_redraw || need_blink_update)
        {
            _need_redraw = false;
            draw_screen ();
        }
    }
}


void Niface::handle_input_msg (void)
{
    M_ifc_txtip *M = (M_ifc_txtip *) get_message ();
    
    if (!M->_line) 
    {
        M->recover ();
        return;
    }
    
    int ch = M->_line[0];
    delete[] M->_line;
    M->recover ();
    
    handle_key (ch);
}




void Niface::handle_key (int ch)
{
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
                    _stop = true;
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
    case 4:   // Ctrl-D (EOT)
    case EOF: // EOF (also Ctrl-D in some cases)
        _stop = true;
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
    // The dynamic resize checking in draw_screen() should handle this
    // Just force a screen redraw
    draw_screen ();
}


void Niface::calculate_layout (void)
{
    // Calculate optimal group layout based on screen size
    _group_width = 24;  // Each group needs about 24 characters for full stop names
    _groups_per_row = _max_cols / _group_width;
    if (_groups_per_row < 1) _groups_per_row = 1;
    
    // If we have init data, refine the layout based on actual group count
    if (_initdata)
    {
        // Ensure we don't exceed available groups
        if (_groups_per_row > _initdata->_ngroup) 
            _groups_per_row = _initdata->_ngroup;
    }
    
    // Adjust group width to use full screen width
    if (_groups_per_row > 1)
        _group_width = _max_cols / _groups_per_row;
}


void Niface::draw_screen (void)
{
    if (!_main_win || !_status_win) return;
    
    // Clear the entire screen first to remove any stray output
    clear ();
    refresh ();
    
    // Always check current screen dimensions before drawing
    int current_rows, current_cols;
    getmaxyx (stdscr, current_rows, current_cols);
    
    // Update layout if dimensions changed
    if (current_rows != _max_rows || current_cols != _max_cols)
    {
        _max_rows = current_rows;
        _max_cols = current_cols;
        calculate_layout ();
        
        // Resize windows to match new dimensions
        wresize (_main_win, _max_rows - 1, _max_cols);
        wresize (_status_win, 1, _max_cols);
        mvwin (_status_win, _max_rows - 1, 0);
    }
    
    werase (_main_win);
    werase (_status_win);
    
    draw_groups ();
    draw_cursor ();
    draw_status ();
    
    wrefresh (_main_win);
    wrefresh (_status_win);
}


void Niface::draw_initial_screen (void)
{
    if (!_main_win || !_status_win) return;
    
    // Clear everything
    clear ();
    werase (_main_win);
    werase (_status_win);
    
    // Show simple loading message
    mvwprintw (_main_win, 0, 1, "Aeolus NCurses Interface");
    mvwprintw (_main_win, 2, 1, "Loading instrument data...");
    mvwprintw (_status_win, 0, 0, "Arrow keys:move, Controls disabled - Ctrl-D:quit");
    
    wrefresh (_main_win);
    wrefresh (_status_win);
    refresh ();
}


void Niface::draw_groups (void)
{
    if (!_initdata) 
    {
        // Show loading message before instrument data arrives
        mvwprintw (_main_win, 0, 1, "Aeolus NCurses Interface");
        mvwprintw (_main_win, 2, 1, "Loading instrument data...");
        return;
    }
    
    // Show normal display, but handle tuning overlay separately
    // Don't return early - we want to show stops with tuning overlay
    
    int row = 1, col = 1;
    int group_start_row = 2;
    
    // Title
    mvwprintw (_main_win, 0, 1, "Aeolus NCurses Interface - %s", _initdata->_instrdir);
    
    // Draw each group
    for (int g = 0; g < _initdata->_ngroup; g++)
    {
        // Calculate group position
        int group_col = col + (g % _groups_per_row) * _group_width;
        int group_row = group_start_row + (g / _groups_per_row) * 15; // 15 lines per group max
        
        if (group_row >= _max_rows - 2) break; // Don't overflow screen
        
        // Group header with rewritten label
        char group_label[32];
        rewrite_label (_initdata->_groupd[g]._label, group_label, sizeof(group_label));
        
        // Use color for group header
        if (has_colors ()) wattron (_main_win, COLOR_PAIR(4) | A_BOLD);
        mvwprintw (_main_win, group_row, group_col, "%s", group_label);
        if (has_colors ()) wattroff (_main_win, COLOR_PAIR(4) | A_BOLD);
        
        // Draw stops in this group
        for (int i = 0; i < _initdata->_groupd[g]._nifelm; i++)
        {
            int stop_row = group_row + 1 + i;
            if (stop_row >= _max_rows - 1) break; // Don't overflow
            
            // Check if this stop is pulled (active)
            bool is_pulled = (_ifelms[g] & (1 << i)) != 0;
            
            // Check if this stop is currently being tuned
            bool is_tuning = _has_tuning_stop && _tuning_group == g && _tuning_ifelm == i;
            
            // Get stop color based on type
            int color_pair = get_stop_color (g, i);
            
            // Apply highlighting - tuning stops blink, pulled stops are reversed
            if (has_colors ())
            {
                if (is_tuning && _tuning_blink_state)
                    wattron (_main_win, COLOR_PAIR(color_pair) | A_BLINK | A_BOLD);
                else if (is_pulled)
                    wattron (_main_win, COLOR_PAIR(color_pair) | A_REVERSE | A_BOLD);
                else
                    wattron (_main_win, COLOR_PAIR(color_pair));
            }
            else
            {
                if (is_tuning && _tuning_blink_state)
                    wattron (_main_win, A_BLINK | A_BOLD);
                else if (is_pulled)
                    wattron (_main_win, A_REVERSE | A_BOLD);
            }
            
            // Draw the stop with its full label (rewritten to remove $)
            char stop_label[32];
            rewrite_label (_initdata->_groupd[g]._ifelmd[i]._label, stop_label, sizeof(stop_label));
            mvwprintw (_main_win, stop_row, group_col + 1, "%-16s", stop_label);
            
            // Turn off attributes
            if (has_colors ())
            {
                if (is_tuning && _tuning_blink_state)
                    wattroff (_main_win, COLOR_PAIR(color_pair) | A_BLINK | A_BOLD);
                else if (is_pulled)
                    wattroff (_main_win, COLOR_PAIR(color_pair) | A_REVERSE | A_BOLD);
                else
                    wattroff (_main_win, COLOR_PAIR(color_pair));
            }
            else
            {
                if (is_tuning && _tuning_blink_state)
                    wattroff (_main_win, A_BLINK | A_BOLD);
                else if (is_pulled)
                    wattroff (_main_win, A_REVERSE | A_BOLD);
            }
        }
    }
}


void Niface::draw_cursor (void)
{
    if (!_initdata || _cursor_group >= _initdata->_ngroup) return;
    
    // Calculate cursor position on screen
    int cursor_y, cursor_x;
    get_stop_position (_cursor_group, _cursor_ifelm, &cursor_y, &cursor_x);
    
    // Draw cursor indicator - a simple > character
    if (cursor_y > 0 && cursor_y < _max_rows - 1 && 
        cursor_x > 0 && cursor_x < _max_cols - 1)
    {
        if (has_colors ()) wattron (_main_win, COLOR_PAIR(3));
        mvwprintw (_main_win, cursor_y, cursor_x - 1, ">");
        if (has_colors ()) wattroff (_main_win, COLOR_PAIR(3));
    }
}



void Niface::draw_status (void)
{
    if (_command_mode)
    {
        mvwprintw (_status_win, 0, 0, "/%s", _command_buffer);
    }
    else
    {
        // Always show the standard controls message on the left
        if (!_ready)
        {
            mvwprintw (_status_win, 0, 0, "Arrow keys:move, Controls disabled - Ctrl-D:quit");
        }
        else
        {
            mvwprintw (_status_win, 0, 0, "Arrow keys:move, Space:toggle, 1-9:presets, /:command, Ctrl-D:quit");
        }
        
        // Add tuning info on the right side when actually tuning - fixed width to prevent jumping
        if (_has_tuning_stop)
        {
            // Reserve fixed width for tuning message (30 characters)
            int tuning_width = 30;
            int tuning_pos = _max_cols - tuning_width;
            if (tuning_pos < 65) tuning_pos = 65; // Don't overlap with controls message
            
            if (tuning_pos >= 0 && tuning_pos + tuning_width <= _max_cols)
            {
                if (_initdata && _tuning_group < _initdata->_ngroup &&
                    _tuning_ifelm < _initdata->_groupd[_tuning_group]._nifelm)
                {
                    char stop_name[20]; // Truncate long stop names
                    rewrite_label (_initdata->_groupd[_tuning_group]._ifelmd[_tuning_ifelm]._label, 
                                  stop_name, sizeof(stop_name));
                    mvwprintw (_status_win, 0, tuning_pos, "TUNING: %-18s", stop_name);
                }
                else
                {
                    mvwprintw (_status_win, 0, tuning_pos, "TUNING IN PROGRESS        ");
                }
            }
        }
    }
}


void Niface::move_cursor (int dy, int dx)
{
    if (!_initdata) return;
    
    // Handle vertical movement within a group
    if (dy != 0)
    {
        int new_ifelm = _cursor_ifelm + dy;
        
        // Check bounds within current group
        if (new_ifelm >= 0 && new_ifelm < _initdata->_groupd[_cursor_group]._nifelm)
        {
            _cursor_ifelm = new_ifelm;
        }
        else if (dy > 0) // Moving down, wrap to next group
        {
            if (_cursor_group + 1 < _initdata->_ngroup)
            {
                _cursor_group++;
                _cursor_ifelm = 0;
            }
        }
        else // Moving up, wrap to previous group
        {
            if (_cursor_group > 0)
            {
                _cursor_group--;
                _cursor_ifelm = _initdata->_groupd[_cursor_group]._nifelm - 1;
            }
        }
    }
    
    // Handle horizontal movement between groups
    if (dx != 0)
    {
        int new_group = _cursor_group + dx;
        
        // Check bounds for group
        if (new_group >= 0 && new_group < _initdata->_ngroup)
        {
            _cursor_group = new_group;
            // Ensure cursor ifelm is within new group bounds
            if (_cursor_ifelm >= _initdata->_groupd[_cursor_group]._nifelm)
            {
                _cursor_ifelm = _initdata->_groupd[_cursor_group]._nifelm - 1;
            }
        }
    }
    
    draw_screen ();
}


void Niface::toggle_current_stop (void)
{
    if (!_initdata || _cursor_group >= _initdata->_ngroup) return;
    if (_cursor_ifelm >= _initdata->_groupd[_cursor_group]._nifelm) return;
    
    // Don't allow stop changes until organ is ready
    if (!_ready) return;
    
    // Send button toggle message to model
    M_ifc_ifelm *M = new M_ifc_ifelm (MT_IFC_ELXOR, _cursor_group, _cursor_ifelm);
    send_event (TO_MODEL, M);
}


void Niface::recall_preset (int preset)
{
    if (preset < 1 || preset > 9) return;
    
    // Don't allow presets until organ is ready
    if (!_ready) return;
    
    // Send preset recall message to model
    M_ifc_preset *M = new M_ifc_preset (MT_IFC_PRRCL, 0, preset - 1, 0, 0);
    send_event (TO_MODEL, M);
}


void Niface::store_preset (int preset)
{
    if (preset < 1 || preset > 9) return;
    
    // Don't allow presets until organ is ready
    if (!_ready) return;
    
    // Send preset store message to model - need current state bits
    // For now, just send with null bits - model will handle current state
    M_ifc_preset *M = new M_ifc_preset (MT_IFC_PRSTO, 0, preset - 1, 0, 0);
    send_event (TO_MODEL, M);
}


void Niface::general_cancel (void)
{
    if (!_initdata) return;
    
    // Don't allow general cancel until organ is ready
    if (!_ready) return;
    
    // Send clear group message for each group to push all stops
    for (int g = 0; g < _initdata->_ngroup; g++)
    {
        M_ifc_ifelm *M = new M_ifc_ifelm (MT_IFC_GRCLR, g, 0);
        send_event (TO_MODEL, M);
    }
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
    _init = false;
    _ready = true;
    _has_tuning_stop = false;  // Clear tuning stop when ready
    _need_redraw = true;
}


void Niface::handle_ifc_init (M_ifc_init *M)
{
    if (_initdata) _initdata->recover ();
    _initdata = M;
    _need_redraw = true;
}


void Niface::handle_ifc_mcset (M_ifc_chconf *M)
{
    if (_mididata) _mididata->recover ();
    _mididata = M;
}


void Niface::handle_ifc_retune (M_ifc_retune *M)
{
    // Show retuning message - means we're not ready yet
    _ready = false;
    _need_redraw = true;
}


void Niface::handle_ifc_grclr (M_ifc_ifelm *M)
{
    _ifelms [M->_group] = 0;
    _need_redraw = true;
}


void Niface::handle_ifc_elclr (M_ifc_ifelm *M)
{
    _ifelms [M->_group] &= ~(1 << M->_ifelm);
    _need_redraw = true;
}


void Niface::handle_ifc_elset (M_ifc_ifelm *M)
{
    _ifelms [M->_group] |= (1 << M->_ifelm);
    _need_redraw = true;
}


void Niface::handle_ifc_elatt (M_ifc_ifelm *M)
{
    // Handle element attachment (retuning) - this stop is being tuned
    _has_tuning_stop = true;
    _tuning_group = M->_group;
    _tuning_ifelm = M->_ifelm;
    _tuning_blink_state = true;
    _need_redraw = true;
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
    const char *label = _initdata->_groupd[group]._ifelmd[ifelm]._label;
    const char *mnemo = _initdata->_groupd[group]._ifelmd[ifelm]._mnemo;
    
    // Use _type first, then fallback to name matching
    switch (type)
    {
    case 0:  // Normal stops
        return 4;  // Normal white
    case 1:  // Tremulants 
        return 1;  // Green
    case 2:  // Couplers
        return 2;  // Yellow
    default:
        // Fallback to name-based detection
        if (strstr(label, "Tremulant") || strstr(label, "tremulant") ||
            strstr(mnemo, "trem") || strstr(mnemo, "Trem"))
        {
            return 1;  // Green
        }
        
        if (strstr(label, "Coupler") || strstr(label, "coupler") ||
            strstr(mnemo, "coup") || strstr(mnemo, "Coup") ||
            strstr(label, " II") || strstr(label, " III") ||
            strstr(label, " P") || strstr(label, "Ped") ||
            strstr(mnemo, "II") || strstr(mnemo, "III") ||
            strstr(mnemo, "P") || strstr(mnemo, "Ped"))
        {
            return 2;  // Yellow
        }
        
        return 4;  // Normal white
    }
}


void Niface::get_stop_position (int group, int ifelm, int *y, int *x)
{
    int group_start_row = 2;
    int group_col = 1 + (group % _groups_per_row) * _group_width;
    int group_row = group_start_row + (group / _groups_per_row) * 15;
    
    *y = group_row + 1 + ifelm; // +1 for group header
    *x = group_col + 1; // +1 to align with stop text
}

/*
 * NCurses Terminal Interface Threading Architecture
 * =================================================
 *
 * This implementation uses a clean event-driven architecture to eliminate
 * CPU-intensive polling while ensuring thread-safe ncurses operations.
 *
 * Threading Model:
 * ---------------
 *
 * 1. MAIN THREAD (Niface::thr_main):
 *    - The ONLY thread that performs ncurses drawing operations
 *    - Blocks on get_event_timed() waiting for events
 *    - Handles ALL UI events in a single event loop:
 *      * EV_INPUT: Keyboard input available
 *      * FM_MODEL: Model state changes (organ initialization, stop changes, etc.)
 *      * EV_TIME: Timer events for tuning blink animation
 *      * EV_EXIT: Shutdown signal
 *    - Processes message handlers that set _need_redraw flag
 *    - Performs screen updates atomically after processing events
 *
 * 2. INPUT HANDLER THREAD (NInputHandler::thr_main):
 *    - Uses select() to block waiting for keyboard input on stdin
 *    - When input is available, sends EV_INPUT event to main thread via reply()
 *    - Never touches ncurses directly - only signals input availability
 *    - Eliminates need for polling getch() with nodelay()
 *
 * Message Flow:
 * ------------
 *
 * Keyboard Input:
 *   stdin -> select() -> NInputHandler -> EV_INPUT -> Main Thread -> getch() -> handle_key()
 *
 * Model Updates:
 *   Model -> FM_MODEL -> Main Thread -> handle_mesg() -> sets _need_redraw -> draw_screen()
 *
 * Timer Events:
 *   ITC Timer -> EV_TIME -> Main Thread -> tuning blink logic -> draw_screen()
 *
 * Key Benefits:
 * ------------
 *
 * 1. THREAD SAFETY: All ncurses calls happen in main thread only
 * 2. NO POLLING: select() and get_event_timed() block efficiently
 * 3. NO RACE CONDITIONS: Model messages processed directly by main thread
 * 4. RESPONSIVE: Input and model updates wake main thread immediately
 * 5. CPU EFFICIENT: No usleep() or busy loops
 *
 * Event Processing Pattern:
 * ------------------------
 *
 * The main thread follows this pattern:
 *   1. Block waiting for event via get_event_timed()
 *   2. Process the specific event type
 *   3. Check if _need_redraw flag was set by message handlers
 *   4. If any changes occurred, call draw_screen() to update display
 *   5. Return to blocking wait
 *
 * This ensures the screen is always up-to-date with minimal CPU usage and
 * maximum responsiveness to both user input and system state changes.
 */