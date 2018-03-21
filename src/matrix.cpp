/*	  Copyright (C) 2000,2001,2002  Sony Computer Entertainment America

       	  This file is subject to the terms and conditions of the GNU Lesser
	  General Public License Version 2.1. See the file "COPYING" in the
	  main directory of this archive for more details.                             */

#ifndef NO_VU0_VECTORS

#include "ps2s/matrix.h"

void mat_44::set_rotate(float angle, vec_xyz axis)
{
    axis.normalize();
    float x = vec_x(axis), y = vec_y(axis), z = vec_z(axis);

    float c = cosf(angle), s = sinf(angle);

    set_col0(vec_xyzw(x * x * (1 - c) + c,
        y * x * (1 - c) + z * s,
        x * z * (1 - c) - y * s,
        0));
    set_col1(vec_xyzw(x * y * (1 - c) - z * s,
        y * y * (1 - c) + c,
        y * z * (1 - c) + x * s,
        0));
    set_col2(vec_xyzw(x * z * (1 - c) + y * s,
        y * z * (1 - c) - x * s,
        z * z * (1 - c) + c,
        0));
    set_col3(vec_xyzw(0, 0, 0, 1));
}

#endif // NO_VU0_VECTORS
