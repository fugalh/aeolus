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


#include <unistd.h>
#include "slave.h"


void Slave::thr_main (void) 
{
    ITC_mesg *M;

    while (get_event () != EV_EXIT)
    {
	M = get_message ();
        if (! M) continue;

        switch (M->type ())
	{
            case MT_CALC_RANK:
            {
                M_def_rank *X = (M_def_rank *) M;
                send_event (TO_MODEL, new M_ifc_ifelm (MT_IFC_ELATT, X->_group, X->_ifelm)); 
                X->_rwave = new Rankwave (X->_synth->_n0, X->_synth->_n1);
                X->_rwave->gen_waves (X->_synth, X->_fsamp, X->_fbase, X->_scale);
                send_event (TO_AUDIO, M);
                break;
	    }

            case MT_LOAD_RANK:
            {
                M_def_rank *X = (M_def_rank *) M;
                send_event (TO_MODEL, new M_ifc_ifelm (MT_IFC_ELATT, X->_group, X->_ifelm)); 
                X->_rwave = new Rankwave (X->_synth->_n0, X->_synth->_n1);
                if (X->_rwave->load (X->_path, X->_synth, X->_fsamp, X->_fbase, X->_scale)) 
                {
                    X->_rwave->gen_waves (X->_synth, X->_fsamp, X->_fbase, X->_scale); 
		} 
                send_event (TO_AUDIO, M);
                break;
	    }

            case MT_SAVE_RANK:
            {
                M_def_rank *X = (M_def_rank *) M;
                X->_rwave->save (X->_path, X->_synth, X->_fsamp, X->_fbase, X->_scale); 
                M->recover ();
                break;
	    }

   	    case MT_AUDIO_SYNC:
		send_event (TO_AUDIO, M);
		break;
 
	    default:
	        M->recover ();
	} 
    }
    send_event (EV_EXIT, 1);
}


