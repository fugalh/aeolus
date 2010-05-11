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


#ifndef __SLAVE_H
#define __SLAVE_H


#include <clthreads.h>
#include "messages.h"


class Slave : public A_thread
{
public:

    Slave (void) : A_thread ("Slave") {}
    virtual ~Slave (void) {}

    void terminate (void) {  put_event (EV_EXIT, 1); }

private:

    virtual void thr_main (void);
};


#endif
