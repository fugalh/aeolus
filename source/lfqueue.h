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


#ifndef __LFQUEUE_H
#define __LFQUEUE_H


#include <stdint.h>


class Lfq_u8
{
public:

    Lfq_u8 (int size);
    ~Lfq_u8 (void); 

    int       write_avail (void) const { return _size - _nwr + _nrd; } 
    void      write_commit (int n) { _nwr += n; }
    void      write (int i, uint8_t v) { _data [(_nwr + i) & _mask] = v; }

    int       read_avail (void) const { return _nwr - _nrd; } 
    void      read_commit (int n) { _nrd += n; }
    uint8_t   read (int i) { return _data [(_nrd + i) & _mask]; }

private:

    uint8_t * _data;
    int       _size;
    int       _mask;
    int       _nwr;
    int       _nrd;
};


class Lfq_u16
{
public:

    Lfq_u16 (int size);
    ~Lfq_u16 (void); 

    int       write_avail (void) const { return _size - _nwr + _nrd; } 
    void      write_commit (int n) { _nwr += n; }
    void      write (int i, uint16_t v) { _data [(_nwr + i) & _mask] = v; }

    int       read_avail (void) const { return _nwr - _nrd; } 
    void      read_commit (int n) { _nrd += n; }
    uint16_t  read (int i) { return _data [(_nrd + i) & _mask]; }

private:

    uint16_t *_data;
    int       _size;
    int       _mask;
    int       _nwr;
    int       _nrd;
};


class Lfq_u32
{
public:

    Lfq_u32 (int size);
    ~Lfq_u32 (void); 

    int       write_avail (void) const { return _size - _nwr + _nrd; } 
    void      write_commit (int n) { _nwr += n; }
    void      write (int i, uint32_t v) { _data [(_nwr + i) & _mask] = v; }

    int       read_avail (void) const { return _nwr - _nrd; } 
    void      read_commit (int n) { _nrd += n; }
    uint32_t  read (int i) { return _data [(_nrd + i) & _mask]; }

private:

    uint32_t *_data;
    int       _size;
    int       _mask;
    int       _nwr;
    int       _nrd;
};


#endif

