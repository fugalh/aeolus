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
#include <clthreads.h>
#include "xiface.h"
#include "styles.h"
#include "callbacks.h"



extern "C" Iface *create_iface (int ac, char *av [])
{
    return new Xiface (ac, av);
}



Xiface::Xiface (int ac, char *av [])
{
    _xresm.init (&ac, av, (char *)"aeolus", 0, 0);
    _disp = new X_display (_xresm.get (".display", 0));
    if (_disp->dpy () == 0)
    {
	fprintf (stderr, "Can't open display !\n");
        delete _disp;
	exit (1);
    }
    init_styles (_disp, &_xresm);
    _root = new X_rootwin (_disp);
    _xhan = new X_handler (_disp, this, EV_XWIN);
    _xhan->next_event ();
    _aupar = 0;
    _dipar = 0;
    _editp = 0;
}


Xiface::~Xiface (void)
{
    delete _mainwin;
    delete _audiowin;
    delete _instrwin;
    delete _editwin;
    delete _xhan;
    delete _root;
    delete _disp;
}


void Xiface::stop (void)
{
    _stop = true;
}


void Xiface::thr_main (void)
{
    _stop  = false;
    _ready = false;
    set_time (0);
    inc_time (125000);
    while (! _stop)
    {
        switch (get_event_timed ())
	{
        case EV_TIME:
	    handle_time ();
            XFlush (_disp->dpy ());
            inc_time (125000);
 	    break;

        case FM_MODEL:
            handle_mesg (get_message ()); 
            XFlush (_disp->dpy ());
	    break;

        case EV_XWIN:
      	    _root->handle_event ();
	    _xhan->next_event ();
            break;

        case EV_EXIT:
            return;
	}
    }
    send_event (EV_EXIT, 1);
}


void Xiface::handle_time (void)
{
    if (_ready)
    {
        _mainwin->handle_time ();
        _editwin->handle_time ();
    }
    if (_aupar)
    {        
        send_event (TO_MODEL, _aupar);
        _aupar = 0;
    }
    if (_dipar)
    {        
        send_event (TO_MODEL, _dipar);
        _dipar = 0;
    }
}


void Xiface::handle_mesg (ITC_mesg *M)
{
    switch (M->type ())
    {
    case MT_IFC_INIT:
    {
        M_ifc_init *X = (M_ifc_init *) M;
        _mainwin  = new Mainwin (_root, this, 100, 100, &_xresm);           
        _midiwin  = new Midiwin (_root, this, 120, 120, &_xresm);           
        _audiowin = new Audiowin (_root, this, 140, 140, &_xresm);           
        _instrwin = new Instrwin (_root, this, 160, 160, &_xresm);           
        _editwin  = new Editwin (_root, this, 180, 180, &_xresm);
        _mainwin->setup (X);
        _midiwin->setup (X);
        _audiowin->setup (X);
        _instrwin->setup (X);
        _editwin->sdir (X->_stopsdir);
        _editwin->wdir (X->_wavesdir);
        _ready = true;         
        break;
    }
    case MT_IFC_READY:
        _mainwin->set_ready ();
        _editwin->lock (0);
	break;

    case MT_IFC_ELSET:
    case MT_IFC_ELCLR:
    case MT_IFC_ELATT:
    case MT_IFC_GRCLR:
    {
	M_ifc_ifelm *X = (M_ifc_ifelm *) M;
        _mainwin->set_ifelm (X);
	break;
    }
    case MT_IFC_PRRCL:
    {
	M_ifc_preset *X = (M_ifc_preset *) M;
        _mainwin->set_state (X);
	break;
    }
    case MT_IFC_AUPAR:
    {
	M_ifc_aupar *X = (M_ifc_aupar *) M;
        if (X->_srcid != SRC_GUI_DRAG) _audiowin->set_aupar (X);
	break;
    }
    case MT_IFC_DIPAR:
    {
	M_ifc_dipar *X = (M_ifc_dipar *) M;
        if (X->_srcid != SRC_GUI_DRAG) _instrwin->set_dipar (X);
	break;
    }
    case MT_IFC_RETUNE:
        _instrwin->set_tuning ((M_ifc_retune *) M);
        break;

    case MT_IFC_EDIT:
        if (! _editp)
	{
	    _editp = (M_ifc_edit *) M;
            _editwin->init (_editp->_synth);
            _editwin->x_mapraised ();
            M = 0;
	}  
        break;

    case MT_IFC_MCSET:
        _midiwin->setconf ((M_ifc_chconf *) M);
        break;

    default:
	;
    }
    if (M) M->recover ();
}


void Xiface::handle_callb (int k, X_window *W, XEvent *E)
{
    switch (k)
    {
    case CB_SHOW_AUDW:
	_audiowin->x_mapraised ();
	break;

    case CB_SHOW_MIDW:
	_midiwin->x_mapraised ();
	break;

    case CB_SHOW_INSW:
	_instrwin->x_mapraised ();
	break;

    case CB_GLOB_SAVE:
        send_event (TO_MODEL, new ITC_mesg (MT_IFC_SAVE));
	break;

    case CB_GLOB_MOFF:
        send_event (TO_MODEL, new ITC_mesg (MT_IFC_ANOFF));
	break;

    case CB_MAIN_MSG:
        send_event (TO_MODEL, _mainwin->mesg ());
        break;

    case CB_MAIN_END:
        stop ();
	break;

    case CB_AUDIO_ACT:
	if (_aupar) _aupar->_value = _audiowin->value ();
	else _aupar = new M_ifc_aupar (SRC_GUI_DRAG, _audiowin->asect (), _audiowin->parid (), _audiowin->value ());
        if (_audiowin->final ())
	{        
	    _aupar->_srcid = SRC_GUI_DONE;
            send_event (TO_MODEL, _aupar);
            _aupar = 0;
	}
        break;

    case CB_DIVIS_ACT:
	if (_dipar) _dipar->_value = _instrwin->value ();
	else _dipar = new M_ifc_dipar (SRC_GUI_DRAG, _instrwin->divis (), _instrwin->parid (), _instrwin->value ());
        if (_instrwin->final ())
	{        
	    _dipar->_srcid = SRC_GUI_DONE;
            send_event (TO_MODEL, _dipar);
            _dipar = 0;
	}
        break;

    case CB_RETUNE:
        send_event (TO_MODEL, new M_ifc_retune (_instrwin->freq (), _instrwin->temp ()));
	break;

    case CB_MIDI_SETCONF:
        send_event (TO_MODEL, new M_ifc_chconf (MT_IFC_MCSET, _midiwin->preset (), _midiwin->chconf ()));
	break;

    case CB_MIDI_GETCONF:
        send_event (TO_MODEL, new M_ifc_chconf (MT_IFC_MCGET, _midiwin->preset (), 0));
	break;

    case CB_EDIT_APP:
        send_event (TO_MODEL, new M_ifc_edit (MT_IFC_APPLY, _editp->_group, _editp->_ifelm, 0));
        _mainwin->set_label (_editp->_group, _editp->_ifelm, _editp->_synth->_stopname);
        _editwin->lock (true);
        break;

    case CB_EDIT_END:
        _editwin->x_unmap ();
        _editp->recover ();
        _editp = 0;
	break;
    }
}

