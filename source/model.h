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


#ifndef __MODEL_H
#define __MODEL_H


#include <clthreads.h>
#include "messages.h"
#include "lfqueue.h"
#include "addsynth.h"
#include "rankwave.h"
#include "global.h"


class Asect
{
public:

    Asect (void) { *_label = 0; }

    char    _label [64];
};


class Rank
{
public:

    int         _count;
    Addsynth   *_synth;
    Rankwave   *_rwave;
};

    
class Divis
{
public:

    enum { HAS_SWELL = 1, HAS_TREM = 2 };
    enum { SWELL, TFREQ, TMODD, NPARAM };

    Divis (void);

    char        _label [16];
    int         _flags;
    int         _dmask;
    int         _nrank;
    int         _asect;
    int         _keybd;
    Fparm       _param [NPARAM];
    Rank        _ranks [NRANKS];
};

    
class Keybd
{
public:

    Keybd (void);

    char    _label [16];
    bool    _pedal;
};

    
class Ifelm
{
public:

    enum { DIVRANK, KBDRANK, COUPLER, TREMUL };

    Ifelm (void);

    char      _label [32];
    char      _mnemo [8];
    int       _type;
    int       _keybd;
    int       _state;
    uint32_t  _action0;
    uint32_t  _action1;
};

    
class Group
{
public:

    enum { NIFELM = NRANKS + 8 };

    Group (void);

    char     _label [16];
    int      _nifelm;
    Ifelm    _ifelms [NIFELM]; 
};


class Midiconf
{
public:

    Midiconf (void)
    {
        memset (_bits, 0, 16 * sizeof (uint16_t));
    }

    uint16_t  _bits [16];
};


class Preset
{
public:

    Preset (void) { memset (_bits, 0, NGROUP * sizeof (uint32_t)); }

    uint32_t  _bits [NGROUP];
};

    

class Model : public A_thread
{
public:

    Model (Lfq_u32      *qcomm,
           Lfq_u8       *qmidi,
	   uint16_t     *midimap,
           const char   *appname,
           const char   *stops,
           const char   *instr,
           const char   *waves,
           bool          uhome);

    virtual ~Model (void);
   
    void terminate (void) {  put_event (EV_EXIT, 1); }

private:

    virtual void thr_main (void);

    void init (void);
    void fini (void);
    void proc_mesg (ITC_mesg *M);
    void proc_qmidi (void);
    void init_audio (void);
    void init_iface (void);
    void init_ranks (int comm);
    void proc_rank (int g, int i, int comm);
    void set_ifelm (int g, int i, int m);
    void clr_group (int g);
    void set_aupar (int s, int a, int p, float v);
    void set_dipar (int s, int d, int p, float v);
    void set_mconf (int i, uint16_t *d);
    void get_state (uint32_t *bits);
    void set_state (int bank, int pres);
    void midi_off (int mask);
    void retune (float freq, int temp);
    void recalc (int g, int i);
    void save (void);
    Rank *find_rank (int g, int i);
    int  read_instr (void);
    int  write_instr (void);
    int  get_preset (int bank, int pres, uint32_t *bits);
    void set_preset (int bank, int pres, uint32_t *bits);
    void ins_preset (int bank, int pres, uint32_t *bits);
    void del_preset (int bank, int pres);
    int  read_presets (void);
    int  write_presets (void);

    Lfq_u32        *_qcomm; 
    Lfq_u8         *_qmidi; 
    uint16_t       *_midimap;
    const char     *_appname;
    const char     *_stopsdir;
    char            _instrdir [1024];
    char            _wavesdir [1024];
    bool            _uhome;
    bool            _ready;

    Asect           _asect [NASECT];
    Keybd           _keybd [NKEYBD];
    Divis           _divis [NDIVIS];
    Group           _group [NGROUP];

    int             _nasect;
    int             _ndivis;
    int             _nkeybd;
    int             _ngroup;
    float           _fbase;
    int             _itemp;
    int             _count;
    int             _bank;
    int             _pres;
    int             _client;
    int             _portid;
    int             _sc_cmode; // stop control command mode
    int             _sc_group; // stop control group number
    Midiconf        _chconf [8];
    Preset         *_preset [NBANK][NPRES];
    M_audio_info   *_audio;
    M_midi_info    *_midi;
};


#endif

