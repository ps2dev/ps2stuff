/*	  Copyright (C) 2000,2001,2002  Sony Computer Entertainment America

       	  This file is subject to the terms and conditions of the GNU Lesser
	  General Public License Version 2.1. See the file "COPYING" in the
	  main directory of this archive for more details.                             */

#include <math.h>

#include "ps2s/cpu_matrix.h"

void cpu_mat_44::set_rotate(float angle, cpu_vec_xyz axis)
{
    axis.normalize();
    float x = axis(0), y = axis(1), z = axis(2);

    float c = cosf(angle), s = sinf(angle);

    set_col0(cpu_vec_xyzw(x * x * (1 - c) + c,
        y * x * (1 - c) + z * s,
        x * z * (1 - c) - y * s,
        0));
    set_col1(cpu_vec_xyzw(x * y * (1 - c) - z * s,
        y * y * (1 - c) + c,
        y * z * (1 - c) + x * s,
        0));
    set_col2(cpu_vec_xyzw(x * z * (1 - c) + y * s,
        y * z * (1 - c) - x * s,
        z * z * (1 - c) + c,
        0));
    set_col3(cpu_vec_xyzw(0, 0, 0, 1));
}

cpu_mat_44
cpu_mat_44::transpose() const
{
    cpu_mat_44 result;

    result.col0[1] = col1(0);
    result.col1[0] = col0(1);
    result.col1[2] = col2(1);
    result.col2[1] = col1(2);
    result.col2[3] = col3(2);
    result.col3[2] = col2(3);

    result.col0[2] = col2(0);
    result.col2[0] = col0(2);
    result.col0[3] = col3(0);
    result.col3[0] = col0(3);
    result.col1[3] = col3(1);
    result.col3[1] = col1(3);

    result.col0[0] = col0(0);
    result.col1[1] = col1(1);
    result.col2[2] = col2(2);
    result.col3[3] = col3(3);

    return result;
}
