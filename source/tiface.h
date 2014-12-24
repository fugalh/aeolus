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


#ifndef __TIFACE_H
#define __TIFACE_H


#include "iface.h"


class Reader : public H_thread
{
public:

    Reader (Edest *, int);
    virtual ~Reader (void);

    void read (void);

private:

    virtual void thr_main (void);
};


           
class Tiface : public Iface
{
public:

    Tiface (int ac, char *av []);
    virtual ~Tiface (void);
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
    void handle_ifc_txtip (M_ifc_txtip *);
    void print_info (void);
    void print_midimap (void);
    void print_keybdd (void);
    void print_divisd (void);
    void print_asectd (void);
    void print_stops_short (int);
    void print_stops_long (int);
    void rewrite_label (const char *);
    void parse_command (const char *);
    void command_s (const char *);
    int  find_group (const char *);
    int  find_ifelm (const char *, int);
    int  comm1 (const char *);
    
    Reader          _reader;
    bool            _stop;
    bool            _init;
    M_ifc_init     *_initdata;
    M_ifc_chconf   *_mididata;
    uint32_t        _ifelms [NGROUP];
    char            _tempstr [64];
};


#endif
