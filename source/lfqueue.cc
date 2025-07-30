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


#include <assert.h>
#include "lfqueue.h"


Lfq_u8::Lfq_u8 (int size) : _size (size), _mask (_size - 1), _nwr (0), _nrd (0)
{
    assert (!(_size & _mask));
    _data = new uint8_t [_size];
}

Lfq_u8::~Lfq_u8 (void)
{
    delete[] _data;
}


Lfq_u16::Lfq_u16 (int size) : _size (size), _mask (_size - 1), _nwr (0), _nrd (0)
{
    assert (!(_size & _mask));
    _data = new uint16_t [_size];
}

Lfq_u16::~Lfq_u16 (void)
{
    delete[] _data;
}


Lfq_u32::Lfq_u32 (int size) : _size (size), _mask (_size - 1), _nwr (0), _nrd (0)
{
    assert (!(_size & _mask));
    _data = new uint32_t [_size];
}

Lfq_u32::~Lfq_u32 (void)
{
    delete[] _data;
}


