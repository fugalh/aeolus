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


#ifndef __REVERB_H
#define __REVERB_H


class Delelm
{
private:

    friend class Reverb;

    void init (int size, float fb);
    void fini (void);
    void set_t60mf (float tmf);
    void set_t60lo (float tlo, float _wlo);
    void set_t60hi (float thi, float chi);
    void print (void);
    float process (float x);

    int        _i;
    int        _size;
    float     *_line;
    float      _fb;
    float      _gmf;
    float      _glo;
    float      _wlo;
    float      _whi;
    float      _slo;
    float      _shi;
};



class Reverb
{
public:

    void init (float rate);
    void fini (void);
    void process (int n, float gain, float *R, float *W, float *X, float *Y, float *Z);

    void set_delay (float del);
    void set_t60mf (float tmf);
    void set_t60lo (float tlo, float flo);
    void set_t60hi (float thi, float fhi);

private:

    void print (void);
    float  *_line;
    int     _size;
    int     _idel;
    int     _i;
    Delelm  _delm [16];
    float   _rate;
    float   _gain;
    float   _tmf;
    float   _tlo;
    float   _thi;
    float   _flo;
    float   _fhi;
    float   _x0, _x1, _x2, _x3, _x4, _x5, _x6, _x7;
    float   _z;

    static int   _sizes [16];
    static float _feedb [16];
};


#endif
