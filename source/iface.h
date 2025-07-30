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


#ifndef __IFACE_H
#define __IFACE_H


#include <clthreads.h>
#include "messages.h"


class Iface : public A_thread
{
public:

    Iface (void) : A_thread ("Iface") {}
    virtual ~Iface (void) {}
    virtual void stop (void) = 0;
    void terminate (void) {  put_event (EV_EXIT, 1); }

private:

    virtual void thr_main (void) = 0;
};


typedef Iface *iface_cr (int ac, char *av []);


#endif
