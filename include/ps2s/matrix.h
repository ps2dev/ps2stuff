/*	  Copyright (C) 2000,2001,2002  Sony Computer Entertainment America

       	  This file is subject to the terms and conditions of the GNU Lesser
	  General Public License Version 2.1. See the file "COPYING" in the
	  main directory of this archive for more details.                             */

#ifndef matrix_h
#define matrix_h

#include "ps2s/matrix_common.h"
#include <math.h>

class zero_33;
class zero_44;
class zero_43;
class zero_34;
class mat_34;
class mat_43;
class transform_t;

/********************************************
 * mat_33 - a 3x3 matrix
 */

class mat_33 : public mat_x3_template<vec_3> {
public:
    mat_33() {}

    mat_33(const mat_x3_template<vec_3>& mat)
    {
        col0 = mat.col0;
        col1 = mat.col1;
        col2 = mat.col2;
    }

    mat_33(const vec_3 col_0, const vec_3 col_1, const vec_3 col_2)
    {
        col0 = col_0;
        col1 = col_1;
        col2 = col_2;
    }

    mat_33(const zero_33);

    explicit inline mat_33(const mat_34& mat);
    explicit inline mat_33(const mat_43& mat);
    explicit inline mat_33(const mat_44& mat);

    vec_3 get_row3() const; // undefined
    void set_row3(vec_3);   // undefined

    void set_identity()
    {
        asm __volatile__(
            "vsub       $vf1, $vf0, $vf0         \n"
            "vsub       $vf2, $vf0, $vf0         \n"
            "vmr32      $vf3, $vf0               \n"
            "vaddw.x    $vf1, $vf1, $vf0         \n"
            "vaddw.y    $vf2, $vf2, $vf0         \n"
            "qmfc2      %[col0], $vf1            \n"
            "qmfc2      %[col1], $vf2            \n"
            "qmfc2      %[col2], $vf3            \n"
            : [col0] "=r"(col0.vec128), [col1] "=r"(col1.vec128), [col2] "=r"(col2.vec128)
            :
            : "memory");
    }

    void
    set_scale(vec_3 scale)
    {
        set_zero();
        // Extract components and set diagonal elements using VU0
        float sx = (float)vec_x(scale);
        float sy = (float)vec_y(scale);
        float sz = (float)vec_z(scale);
        asm __volatile__(
            "ctc2       %[sx], $vi21              \n"
            "vaddi.x    $vf1, $vf0, $I           \n"
            "ctc2       %[sy], $vi21              \n"
            "vaddi.y    $vf2, $vf0, $I           \n"
            "ctc2       %[sz], $vi21              \n"
            "vaddi.z    $vf3, $vf0, $I           \n"
            "qmfc2      %[col0], $vf1            \n"
            "qmfc2      %[col1], $vf2            \n"
            "qmfc2      %[col2], $vf3            \n"
            : [col0] "=r"(col0.vec128), [col1] "=r"(col1.vec128), [col2] "=r"(col2.vec128)
            : [sx] "r"(sx), [sy] "r"(sy), [sz] "r"(sz)
            : "memory");
    }

    void
    set(vec_4 quat)
    {
        vec128_t qv = quat.vec128;
        vec128_t col0_v128, col1_v128, col2_v128;
        asm __volatile__ ("### set rotation from unit quaternion ### \n"
            "qmtc2 %[quat], $vf1                    \n"
            "vmr32.xy $vf2, $vf1                    \n"
            "vaddx.z  $vf2, $vf0, $vf1             \n"
            "vaddz.x  $vf3, $vf0, $vf1             \n"
            "vaddx.y  $vf3, $vf0, $vf1             \n"
            "vaddy.z  $vf3, $vf0, $vf1             \n"
            "vmula $ACC, $vf1, $vf2                 \n"
            "vmsubw.z $vf4, $vf3, $vf1             \n"
            "vmsubw.x $vf5, $vf3, $vf1             \n"
            "vmsubw.y $vf6, $vf3, $vf1             \n"
            "vmula $ACC, $vf1, $vf3                 \n"
            "vmaddw.y $vf4, $vf2, $vf1             \n"
            "vmaddw.z $vf5, $vf2, $vf1             \n"
            "vmaddw.x $vf6, $vf2, $vf1             \n"
            "vmula $ACC, $vf3, $vf3                 \n"
            "vmadd.x  $vf4, $vf2, $vf2             \n"
            "vmadd.y  $vf5, $vf2, $vf2             \n"
            "vmadd.z  $vf6, $vf2, $vf2             \n"
            "vmaxw $vf7, $vf0, $vf0                 \n"
            "vadd $vf4, $vf4, $vf4                  \n"
            "vadd $vf5, $vf5, $vf5                  \n"
            "vadd $vf6, $vf6, $vf6                  \n"
            "vsub.x $vf4, $vf7, $vf4               \n"
            "vsub.y $vf5, $vf7, $vf5               \n"
            "vsub.z $vf6, $vf7, $vf6               \n"
            "qmfc2 %[c0], $vf4                      \n"
            "qmfc2 %[c1], $vf5                      \n"
            "qmfc2 %[c2], $vf6                      \n"
            : [c0] "=r"(col0_v128), [c1] "=r"(col1_v128), [c2] "=r"(col2_v128), "=r"(vu0_ACC)
            : [quat] "r"(qv));
        col0.vec128 = col0_v128;
        col1.vec128 = col1_v128;
        col2.vec128 = col2_v128;
    }

    mat_33(vec_4 quat)
    {
        set(quat);
    }

    void
    set_rotate_x(float angle)
    {
        float cs = cosf(angle);
        float sn = sinf(angle);
        col0.set(1, 0, 0);
        col1.set(0, cs, sn);
        col2.set(0, -sn, cs);
    }

    void
    set_rotate_y(float angle)
    {
        float cs = cosf(angle);
        float sn = sinf(angle);
        col0.set(cs, 0, -sn);
        col1.set(0, 1, 0);
        col2.set(sn, 0, cs);
    }

    void
    set_rotate_z(float angle)
    {
        float cs = cosf(angle);
        float sn = sinf(angle);
        col0.set(cs, sn, 0);
        col1.set(-sn, cs, 0);
        col2.set(0, 0, 1);
    }

    // negate

    mat_33 operator-() const;

    // transpose & inverse

    mat_33
    transpose() const;

    void
    transpose_in_place()
    {
        *this = transpose();
    }

    mat_33
    inverse() const;

    void
    inverse_in_place()
    {
        *this = inverse();
    }

    // matrix/scalar operations

    mat_33 operator*(float scale) const;
    mat_33 operator*(vec_x scale) const;
    mat_33 operator*(vec_y scale) const;
    mat_33 operator*(vec_z scale) const;
    mat_33 operator*(vec_w scale) const;

    void operator*=(float scale)
    {
        *this = *this * scale;
    }
    void operator*=(vec_x scale)
    {
        *this = *this * scale;
    }
    void operator*=(vec_y scale)
    {
        *this = *this * scale;
    }
    void operator*=(vec_z scale)
    {
        *this = *this * scale;
    }
    void operator*=(vec_w scale)
    {
        *this = *this * scale;
    }

    // matrix/vector operations

    vec_3
    operator*(vec_3 vec) const
    {
        return mat_x3_template<vec_3>::operator*(vec);
    }

    vec_3
    trans_mult(vec_3 vec) const
    {
        vec128_t result, temp0, temp1, temp2, ones;

        asm("### mat_33 trans_mult vec_3 ### \n"
            "qmtc2      %[col0], $vf10        \n"
            "qmtc2      %[col1], $vf11        \n"
            "qmtc2      %[col2], $vf12        \n"
            "qmtc2      %[vec], $vf13         \n"
            "vmul       $vf14, $vf10, $vf13   \n"
            "vmaxw      $vf15, $vf0, $vf0     \n"
            "vmul       $vf16, $vf11, $vf13   \n"
            "vmul       $vf17, $vf12, $vf13   \n"
            "vadday.x   $ACC, $vf14, $vf14    \n"
            "vmaddz.x   $vf14, $vf15, $vf14   \n"
            "vaddax.y   $ACC, $vf16, $vf16    \n"
            "vmaddz.y   $vf14, $vf15, $vf16   \n"
            "vaddax.z   $ACC, $vf17, $vf17    \n"
            "vmaddy.z   $vf14, $vf15, $vf17   \n"
            "qmfc2      %[result], $vf14      \n"
            : [result] "=&r"(result), "=r"(vu0_ACC)
            : [col0] "r"(col0.vec128),
            [col1] "r"(col1.vec128),
            [col2] "r"(col2.vec128),
            [vec] "r"(vec.vec128));

        return vec_3(result);
    }

    mat_33 mult_tilde(vec_3 vec) const;

    // matrix/matrix operations

    mat_33 operator+(const mat_33& mat) const;
    mat_33 operator-(const mat_33& mat) const;

    void operator+=(const mat_33& mat)
    {
        *this = *this + mat;
    }
    void operator-=(const mat_33& mat)
    {
        *this = *this - mat;
    }

    mat_33 operator*(const mat_33& mat) const;
    mat_34 operator*(const mat_34& mat) const;

    mat_33 trans_mult(const mat_33& mat) const;
    mat_34 trans_mult(const mat_34& mat) const;

    mat_33 mult_trans(const mat_33& mat) const;
    mat_34 mult_trans(const mat_43& mat) const;

    mat_33&
    operator=(const mat_33& mat)
    {
        col0 = mat.col0;
        col1 = mat.col1;
        col2 = mat.col2;
        return *this;
    }

    // matrix/zero_matrix operations

    mat_33 operator+(const zero_33 zero) const;
    mat_33 operator-(const zero_33 zero) const;

    void operator+=(const zero_33 zero);
    void operator-=(const zero_33 zero);

    zero_33 operator*(const zero_33 zero) const;
    zero_34 operator*(const zero_34 zero) const;

    zero_33 trans_mult(const zero_33 zero) const;
    zero_34 trans_mult(const zero_34 zero) const;

    zero_33 mult_trans(const zero_33 zero) const;
    zero_34 mult_trans(const zero_43 zero) const;

    mat_33& operator=(const zero_33 zero);

    void print() const
    {
        get_row0().print();
        get_row1().print();
        get_row2().print();
    }

    void print(const char* mat_name) const
    {
        printf("%s:\n", mat_name);
        print();
    }
};

/********************************************
 * mat_43 - a 4x3 matrix
 */

class mat_43 : public mat_x3_template<vec_4> {
public:
    mat_43() {}

    mat_43(const mat_x3_template<vec_4>& mat)
    {
        col0 = mat.col0;
        col1 = mat.col1;
        col2 = mat.col2;
    }

    mat_43(const zero_43);

    mat_43(const vec_4 col_0, const vec_4 col_1, const vec_4 col_2)
    {
        col0 = col_0;
        col1 = col_1;
        col2 = col_2;
    }

    explicit inline mat_43(const mat_44& mat);

    // negate

    mat_43 operator-() const;

    // transpose

    mat_34 transpose() const;

    // matrix/scalar operations

    mat_43 operator*(float scale) const;
    mat_43 operator*(vec_x vec) const;
    mat_43 operator*(vec_y vec) const;
    mat_43 operator*(vec_z vec) const;
    mat_43 operator*(vec_w vec) const;

    void operator*=(float scale)
    {
        *this = *this * scale;
    }
    void operator*=(vec_x scale)
    {
        *this = *this * scale;
    }
    void operator*=(vec_y scale)
    {
        *this = *this * scale;
    }
    void operator*=(vec_z scale)
    {
        *this = *this * scale;
    }
    void operator*=(vec_w scale)
    {
        *this = *this * scale;
    }

    // matrix/vector operations

    vec_4
    operator*(vec_3 vec) const
    {
        return mat_x3_template<vec_4>::operator*(vec);
    }

    vec_3
    trans_mult(vec_4 vec) const
    {
        vec128_t result;
        vec128_t col0_val = col0.vec128;
        vec128_t col1_val = col1.vec128;
        vec128_t col2_val = col2.vec128;
        vec128_t vec_val = vec.vec128;
        asm __volatile__(
            "### mat_43 trans_mult vec_4 ### \n"
            "qmtc2      %[col0_val], $vf1        \n"
            "qmtc2      %[col1_val], $vf2        \n"
            "qmtc2      %[col2_val], $vf3        \n"
            "qmtc2      %[vec_val], $vf4         \n"
            "vmaxw      $vf5, $vf0, $vf0         \n"
            "vmul       $vf6, $vf1, $vf4         \n"
            "vmul       $vf7, $vf2, $vf4         \n"
            "vmul       $vf8, $vf3, $vf4         \n"
            "vadday.x   $ACC, $vf6, $vf6         \n"
            "vmaddaz.x  $ACC, $vf5, $vf6         \n"
            "vmaddw.x   $vf9, $vf5, $vf6         \n"
            "vaddax.y   $ACC, $vf7, $vf7         \n"
            "vmaddaz.y  $ACC, $vf5, $vf7         \n"
            "vmaddw.y   $vf9, $vf5, $vf7         \n"
            "vaddax.z   $ACC, $vf8, $vf8         \n"
            "vmadday.z  $ACC, $vf5, $vf8         \n"
            "vmaddw.z   $vf9, $vf5, $vf8         \n"
            "qmfc2      %[result], $vf9          \n"
            : [result] "=&r"(result), [acc] "=r"(vu0_ACC)
            : [col0_val] "r"(col0_val), [col1_val] "r"(col1_val),
              [col2_val] "r"(col2_val), [vec_val] "r"(vec_val)
            : "memory");
        return vec_3(result);
    }

    vec_3
    trans_mult(vector_t vec) const
    {
        vec128_t result;
        vec128_t col0_val = col0.vec128;
        vec128_t col1_val = col1.vec128;
        vec128_t col2_val = col2.vec128;
        vec128_t vec_val = vec.vec128;
        asm __volatile__(
            "### mat_43 trans_mult vector_t ### \n"
            "qmtc2      %[col0_val], $vf1        \n"
            "qmtc2      %[col1_val], $vf2        \n"
            "qmtc2      %[col2_val], $vf3        \n"
            "qmtc2      %[vec_val], $vf4         \n"
            "vmaxw      $vf5, $vf0, $vf0         \n"
            "vmul       $vf6, $vf1, $vf4         \n"
            "vmul       $vf7, $vf2, $vf4         \n"
            "vmul       $vf8, $vf3, $vf4         \n"
            "vadday.x   $ACC, $vf6, $vf6         \n"
            "vmaddz.x   $vf9, $vf5, $vf6         \n"
            "vaddax.y   $ACC, $vf7, $vf7         \n"
            "vmaddz.y   $vf9, $vf5, $vf7         \n"
            "vaddax.z   $ACC, $vf8, $vf8         \n"
            "vmaddy.z   $vf9, $vf5, $vf8         \n"
            "qmfc2      %[result], $vf9          \n"
            : [result] "=&r"(result), [acc] "=r"(vu0_ACC)
            : [col0_val] "r"(col0_val), [col1_val] "r"(col1_val),
              [col2_val] "r"(col2_val), [vec_val] "r"(vec_val)
            : "memory");
        return vec_3(result);
    }

    vec_3
    trans_mult(point_t vec) const
    {
        vec128_t result;
        vec128_t col0_val = col0.vec128;
        vec128_t col1_val = col1.vec128;
        vec128_t col2_val = col2.vec128;
        vec128_t vec_val = vec.vec128;
        asm __volatile__(
            "### mat_43 trans_mult point_t ### \n"
            "qmtc2      %[col0_val], $vf1        \n"
            "qmtc2      %[col1_val], $vf2        \n"
            "qmtc2      %[col2_val], $vf3        \n"
            "qmtc2      %[vec_val], $vf4         \n"
            "vmaxw      $vf5, $vf0, $vf0         \n"
            "vmul       $vf6, $vf1, $vf4         \n"
            "vmul       $vf7, $vf2, $vf4         \n"
            "vmul       $vf8, $vf3, $vf4         \n"
            "vadday.x   $ACC, $vf6, $vf6         \n"
            "vmaddaz.x  $ACC, $vf5, $vf6         \n"
            "vmaddw.x   $vf9, $vf5, $vf1         \n"
            "vaddax.y   $ACC, $vf7, $vf7         \n"
            "vmaddaz.y  $ACC, $vf5, $vf7         \n"
            "vmaddw.y   $vf9, $vf5, $vf2         \n"
            "vaddax.z   $ACC, $vf8, $vf8         \n"
            "vmadday.z  $ACC, $vf5, $vf8         \n"
            "vmaddw.z   $vf9, $vf5, $vf3         \n"
            "qmfc2      %[result], $vf9          \n"
            : [result] "=&r"(result), [acc] "=r"(vu0_ACC)
            : [col0_val] "r"(col0_val), [col1_val] "r"(col1_val),
              [col2_val] "r"(col2_val), [vec_val] "r"(vec_val)
            : "memory");
        return vec_3(result);
    }

    // matrix/matrix operations

    mat_43 mult_tilde(vec_3 vec) const;

    mat_43 operator+(const mat_43& mat) const;
    mat_43 operator-(const mat_43& mat) const;

    void operator+=(const mat_43& mat)
    {
        *this = *this + mat;
    }
    void operator-=(const mat_43& mat)
    {
        *this = *this - mat;
    }

    mat_43 operator*(const mat_33& mat) const;
    mat_44 operator*(const mat_34& mat) const;

    mat_33 trans_mult(const mat_43& mat) const;
    mat_34 trans_mult(const mat_44& mat) const;
    mat_34 trans_mult(const transform_t& mat) const;

    mat_44 mult_trans(const mat_43& mat) const;
    mat_43 mult_trans(const mat_33& mat) const;

    mat_43&
    operator=(const mat_33& mat)
    {
        col0 = mat.col0;
        col1 = mat.col1;
        col2 = mat.col2;
        return *this;
    }

    // matrix/zero_matrix operations

    mat_43 operator+(const zero_43 zero) const;
    mat_43 operator-(const zero_43 zero) const;

    void operator+=(const zero_43 zero);
    void operator-=(const zero_43 zero);

    zero_43 operator*(const zero_33 zero) const;
    zero_44 operator*(const zero_34 zero) const;

    zero_33 trans_mult(const zero_43 zero) const;
    zero_34 trans_mult(const zero_44 zero) const;

    zero_44 mult_trans(const zero_43 zero) const;
    zero_43 mult_trans(const zero_33 zero) const;

    mat_43& operator=(const zero_43 zero);

    void print() const
    {
        get_row0().print();
        get_row1().print();
        get_row2().print();
        get_row3().print();
    }

    void print(const char* mat_name) const
    {
        printf("%s:\n", mat_name);
        print();
    }
};

/********************************************
 * mat_34 - a 3x4 matrix
 */

class mat_34 : public mat_x4_template<vec_3> {
public:
    mat_34() {}

    mat_34(const mat_x4_template<vec_3>& mat)
    {
        col0 = mat.col0;
        col1 = mat.col1;
        col2 = mat.col2;
        col3 = mat.col3;
    }

    mat_34(const vec_3 col_0, const vec_3 col_1,
        const vec_3 col_2, const vec_3 col_3)
    {
        col0 = col_0;
        col1 = col_1;
        col2 = col_2;
        col3 = col_3;
    }

    mat_34(const zero_34);

    explicit inline mat_34(const mat_44& mat);

    vec_4 get_row3() const; // undefined
    void set_row3(vec_4);   // undefined

    // negate

    mat_34 operator-() const;

    // transpose

    mat_43 transpose() const;

    // matrix/scalar operations

    mat_34 operator*(float scale) const;
    mat_34 operator*(vec_x vec) const;
    mat_34 operator*(vec_y vec) const;
    mat_34 operator*(vec_z vec) const;
    mat_34 operator*(vec_w vec) const;

    void operator*=(float scale)
    {
        *this = *this * scale;
    }
    void operator*=(vec_x scale)
    {
        *this = *this * scale;
    }
    void operator*=(vec_y scale)
    {
        *this = *this * scale;
    }
    void operator*=(vec_z scale)
    {
        *this = *this * scale;
    }
    void operator*=(vec_w scale)
    {
        *this = *this * scale;
    }

    // matrix/vector operations

    vec_3
    operator*(vec_4 vec) const
    {
        return mat_x4_template<vec_3>::operator*(vec);
    }

    vec_3
    operator*(vector_t vec) const
    {
        return mat_x4_template<vec_3>::operator*(vec);
    }

    vec_3
    operator*(point_t vec) const
    {
        return mat_x4_template<vec_3>::operator*(vec);
    }

    vec_4
    trans_mult(vec_3 vec) const
    {
        vec128_t result;
        vec128_t col0_val = col0.vec128;
        vec128_t col1_val = col1.vec128;
        vec128_t col2_val = col2.vec128;
        vec128_t col3_val = col3.vec128;
        vec128_t vec_val = vec.vec128;
        asm __volatile__(
            "### mat_34 trans_mult vec_3 ### \n"
            "qmtc2      %[col0_val], $vf1        \n"
            "qmtc2      %[col1_val], $vf2        \n"
            "qmtc2      %[col2_val], $vf3        \n"
            "qmtc2      %[col3_val], $vf4        \n"
            "qmtc2      %[vec_val], $vf5         \n"
            "vmul       $vf7, $vf4, $vf5         \n"
            "vmul       $vf6, $vf1, $vf5         \n"
            "vmul       $vf8, $vf2, $vf5         \n"
            "vmul       $vf9, $vf3, $vf5         \n"
            "vmulx.w    $vf7, $vf0, $vf7         \n"
            "vaddy.x    $vf6, $vf6, $vf6         \n"
            "vaddx.y    $vf8, $vf8, $vf8         \n"
            "vaddx.z    $vf9, $vf9, $vf9         \n"
            "vaddy.w    $vf7, $vf7, $vf7         \n"
            "vaddz.x    $vf10, $vf6, $vf6        \n"
            "vaddz.y    $vf10, $vf8, $vf8        \n"
            "vaddy.z    $vf10, $vf9, $vf9        \n"
            "vaddz.w    $vf10, $vf7, $vf7        \n"
            "qmfc2      %[result], $vf10         \n"
            : [result] "=&r"(result)
            : [col0_val] "r"(col0_val), [col1_val] "r"(col1_val),
              [col2_val] "r"(col2_val), [col3_val] "r"(col3_val), [vec_val] "r"(vec_val)
            : "memory");
        return vec_4(result);
    }

    // matrix/matrix operations

    mat_34 operator+(const mat_34& mat) const;
    mat_34 operator-(const mat_34& mat) const;

    void operator+=(const mat_34& mat)
    {
        *this = *this + mat;
    }
    void operator-=(const mat_34& mat)
    {
        *this = *this - mat;
    }

    mat_33 operator*(const mat_43& mat) const;
    mat_34 operator*(const mat_44& mat) const;
    mat_34 operator*(const transform_t& mat) const;

    mat_43 trans_mult(const mat_33& mat) const;
    mat_44 trans_mult(const mat_34& mat) const;

    mat_33 mult_trans(const mat_34& mat) const;
    mat_34 mult_trans(const mat_44& mat) const;

    mat_34&
    operator=(const mat_33& mat)
    {
        col0 = mat.col0;
        col1 = mat.col1;
        col2 = mat.col2;
        return *this;
    }

    // matrix/zero_matrix operations

    mat_34 operator+(const zero_34 zero) const;
    mat_34 operator-(const zero_34 zero) const;

    void operator+=(const zero_34 zero);
    void operator-=(const zero_34 zero);

    zero_33 operator*(const zero_43 zero) const;
    zero_34 operator*(const zero_44 zero) const;

    zero_43 trans_mult(const zero_33 zero) const;
    zero_44 trans_mult(const zero_34 zero) const;

    zero_33 mult_trans(const zero_34 zero) const;
    zero_34 mult_trans(const zero_44 zero) const;

    mat_34& operator=(const zero_34 zero);

    void print() const
    {
        get_row0().print();
        get_row1().print();
        get_row2().print();
    }

    void print(const char* mat_name) const
    {
        printf("%s:\n", mat_name);
        print();
    }
};

/********************************************
 * mat_44 - a 4x4 matrix
 */

class mat_44 : public mat_x4_template<vec_4> {
public:
    mat_44() {}

    mat_44(const mat_x4_template<vec_4>& mat)
    {
        col0 = mat.col0;
        col1 = mat.col1;
        col2 = mat.col2;
        col3 = mat.col3;
    }

    mat_44(const zero_44);

    mat_44(const vec_4 col_0, const vec_4 col_1,
        const vec_4 col_2, const vec_4 col_3)
    {
        col0 = col_0;
        col1 = col_1;
        col2 = col_2;
        col3 = col_3;
    }

    explicit inline mat_44(const transform_t& mat);

    void set_identity()
    {
        asm __volatile__(
            "vsub       $vf1, $vf0, $vf0         \n"
            "vsub       $vf2, $vf0, $vf0         \n"
            "vmr32      $vf3, $vf0               \n"
            "vmove      $vf4, $vf0               \n"
            "vaddw.x    $vf1, $vf1, $vf0         \n"
            "vaddw.y    $vf2, $vf2, $vf0         \n"
            "qmfc2      %[col0], $vf1            \n"
            "qmfc2      %[col1], $vf2            \n"
            "qmfc2      %[col2], $vf3            \n"
            "qmfc2      %[col3], $vf4            \n"
            : [col0] "=r"(col0.vec128), [col1] "=r"(col1.vec128), [col2] "=r"(col2.vec128), [col3] "=r"(col3.vec128)
            :
            : "memory");
    }

    void
    set_scale(vec_3 scale)
    {
        set_zero();
        // Extract components and set diagonal elements using VU0
        float sx = (float)vec_x(scale);
        float sy = (float)vec_y(scale);
        float sz = (float)vec_z(scale);
        asm __volatile__(
            "ctc2       %[sx], $vi21              \n"
            "vaddi.x    $vf1, $vf0, $I           \n"
            "ctc2       %[sy], $vi21              \n"
            "vaddi.y    $vf2, $vf0, $I           \n"
            "ctc2       %[sz], $vi21              \n"
            "vaddi.z    $vf3, $vf0, $I           \n"
            "vmove      $vf4, $vf0               \n"
            "qmfc2      %[col0], $vf1            \n"
            "qmfc2      %[col1], $vf2            \n"
            "qmfc2      %[col2], $vf3            \n"
            "qmfc2      %[col3], $vf4            \n"
            : [col0] "=r"(col0.vec128), [col1] "=r"(col1.vec128), [col2] "=r"(col2.vec128), [col3] "=r"(col3.vec128)
            : [sx] "r"(sx), [sy] "r"(sy), [sz] "r"(sz)
            : "memory");
    }

    void
    set_scale(vec_4 scale)
    {
        set_identity();
        // Extract components and set diagonal elements using VU0
        float sx = (float)vec_x(scale);
        float sy = (float)vec_y(scale);
        float sz = (float)vec_z(scale);
        float sw = (float)vec_w(scale);
        asm __volatile__(
            "vsub       $vf1, $vf0, $vf0         \n"
            "vsub       $vf2, $vf0, $vf0         \n"
            "vsub       $vf3, $vf0, $vf0         \n"
            "vsub       $vf4, $vf0, $vf0         \n"
            "ctc2       %[sx], $vi21              \n"
            "vaddi.x    $vf1, $vf0, $I           \n"
            "ctc2       %[sy], $vi21              \n"
            "vaddi.y    $vf2, $vf0, $I           \n"
            "ctc2       %[sz], $vi21              \n"
            "vaddi.z    $vf3, $vf0, $I           \n"
            "ctc2       %[sw], $vi21              \n"
            "vmuli.w    $vf4, $vf0, $I           \n"
            "qmfc2      %[col0], $vf1            \n"
            "qmfc2      %[col1], $vf2            \n"
            "qmfc2      %[col2], $vf3            \n"
            "qmfc2      %[col3], $vf4            \n"
            : [col0] "=r"(col0.vec128), [col1] "=r"(col1.vec128), [col2] "=r"(col2.vec128), [col3] "=r"(col3.vec128)
            : [sx] "r"(sx), [sy] "r"(sy), [sz] "r"(sz), [sw] "r"(sw)
            : "memory");
    }

    void
    set_translate(vec_3 xlate_amount)
    {
        set_identity();
        // Set translation in col3, preserving w=1 from set_identity
        float tx = (float)vec_x(xlate_amount);
        float ty = (float)vec_y(xlate_amount);
        float tz = (float)vec_z(xlate_amount);
        col3.set_x(tx);
        col3.set_y(ty);
        col3.set_z(tz);
        // w should already be 1.0 from set_identity, but ensure it
        col3.set_w(1.0f);
    }

    void
    set_rotate_x(float angle)
    {
        float cs = cosf(angle);
        float sn = sinf(angle);
        col0.set(1, 0, 0, 0);
        col1.set(0, cs, sn, 0);
        col2.set(0, -sn, cs, 0);
        col3.set(0, 0, 0, 1);
    }

    void
    set_rotate_y(float angle)
    {
        float cs = cosf(angle);
        float sn = sinf(angle);
        col0.set(cs, 0, -sn, 0);
        col1.set(0, 1, 0, 0);
        col2.set(sn, 0, cs, 0);
        col3.set(0, 0, 0, 1);
    }

    void
    set_rotate_z(float angle)
    {
        float cs = cosf(angle);
        float sn = sinf(angle);
        col0.set(cs, sn, 0, 0);
        col1.set(-sn, cs, 0, 0);
        col2.set(0, 0, 1, 0);
        col3.set(0, 0, 0, 1);
    }

    void set_rotate(float angle, vec_xyz axis);

    // negate

    mat_44 operator-() const;

    // transpose

    mat_44 transpose() const;

    void
    transpose_in_place()
    {
        *this = transpose();
    }

    // matrix/scalar operations

    mat_44 operator*(float scale) const;
    mat_44 operator*(vec_x vec) const;
    mat_44 operator*(vec_y vec) const;
    mat_44 operator*(vec_z vec) const;
    mat_44 operator*(vec_w vec) const;

    void operator*=(float scale)
    {
        *this = *this * scale;
    }
    void operator*=(vec_x scale)
    {
        *this = *this * scale;
    }
    void operator*=(vec_y scale)
    {
        *this = *this * scale;
    }
    void operator*=(vec_z scale)
    {
        *this = *this * scale;
    }
    void operator*=(vec_w scale)
    {
        *this = *this * scale;
    }

    // matrix/vector operations

    vec_4
    operator*(vec_4 vec) const
    {
        return mat_x4_template<vec_4>::operator*(vec);
    }

    vec_4
    operator*(vector_t vec) const
    {
        return mat_x4_template<vec_4>::operator*(vec);
    }

    vec_4
    operator*(point_t vec) const
    {
        return mat_x4_template<vec_4>::operator*(vec);
    }

    vec_4
    trans_mult(vec_4 vec) const
    {
        vec128_t result;
        vec128_t col0_val = col0.vec128;
        vec128_t col1_val = col1.vec128;
        vec128_t col2_val = col2.vec128;
        vec128_t col3_val = col3.vec128;
        vec128_t vec_val = vec.vec128;
        asm __volatile__(
            "### mat_44 trans_mult vec_4 ### \n"
            "qmtc2      %[col0_val], $vf1        \n"
            "qmtc2      %[col1_val], $vf2        \n"
            "qmtc2      %[col2_val], $vf3        \n"
            "qmtc2      %[col3_val], $vf4        \n"
            "qmtc2      %[vec_val], $vf5         \n"
            "vmul       $vf6, $vf1, $vf5         \n"
            "vmul       $vf7, $vf2, $vf5         \n"
            "vmul       $vf8, $vf3, $vf5         \n"
            "vmul       $vf9, $vf4, $vf5         \n"
            "vaddy.x    $vf6, $vf6, $vf6         \n"
            "vaddx.y    $vf7, $vf7, $vf7         \n"
            "vaddx.z    $vf8, $vf8, $vf8         \n"
            "vaddx.w    $vf9, $vf9, $vf9         \n"
            "vaddz.x    $vf6, $vf6, $vf6         \n"
            "vaddz.y    $vf7, $vf7, $vf7         \n"
            "vaddy.z    $vf8, $vf8, $vf8         \n"
            "vaddy.w    $vf9, $vf9, $vf9         \n"
            "vaddw.x    $vf10, $vf6, $vf6        \n"
            "vaddw.y    $vf10, $vf7, $vf7        \n"
            "vaddw.z    $vf10, $vf8, $vf8        \n"
            "vaddz.w    $vf10, $vf9, $vf9        \n"
            "qmfc2      %[result], $vf10         \n"
            : [result] "=&r"(result)
            : [col0_val] "r"(col0_val), [col1_val] "r"(col1_val),
              [col2_val] "r"(col2_val), [col3_val] "r"(col3_val), [vec_val] "r"(vec_val)
            : "memory");
        return vec_4(result);
    }

    vec_4
    trans_mult(vector_t vec) const
    {
        vec128_t result;
        vec128_t col0_val = col0.vec128;
        vec128_t col1_val = col1.vec128;
        vec128_t col2_val = col2.vec128;
        vec128_t col3_val = col3.vec128;
        vec128_t vec_val = vec.vec128;
        asm __volatile__(
            "### mat_44 trans_mult vector_t ### \n"
            "qmtc2      %[col0_val], $vf1        \n"
            "qmtc2      %[col1_val], $vf2        \n"
            "qmtc2      %[col2_val], $vf3        \n"
            "qmtc2      %[col3_val], $vf4        \n"
            "qmtc2      %[vec_val], $vf5         \n"
            "vmul       $vf9, $vf4, $vf5         \n"
            "vmul       $vf6, $vf1, $vf5         \n"
            "vmul       $vf7, $vf2, $vf5         \n"
            "vmul       $vf8, $vf3, $vf5         \n"
            "vmulx.w    $vf9, $vf0, $vf9         \n"
            "vaddy.x    $vf6, $vf6, $vf6         \n"
            "vaddx.y    $vf7, $vf7, $vf7         \n"
            "vaddx.z    $vf8, $vf8, $vf8         \n"
            "vaddy.w    $vf9, $vf9, $vf9         \n"
            "vaddz.x    $vf10, $vf6, $vf6        \n"
            "vaddz.y    $vf10, $vf7, $vf7        \n"
            "vaddy.z    $vf10, $vf8, $vf8        \n"
            "vaddz.w    $vf10, $vf9, $vf9        \n"
            "qmfc2      %[result], $vf10         \n"
            : [result] "=&r"(result)
            : [col0_val] "r"(col0_val), [col1_val] "r"(col1_val),
              [col2_val] "r"(col2_val), [col3_val] "r"(col3_val), [vec_val] "r"(vec_val)
            : "memory");
        return vec_4(result);
    }

    vec_4
    trans_mult(point_t vec) const
    {
        vec128_t result;
        vec128_t col0_val = col0.vec128;
        vec128_t col1_val = col1.vec128;
        vec128_t col2_val = col2.vec128;
        vec128_t col3_val = col3.vec128;
        vec128_t vec_val = vec.vec128;
        asm __volatile__(
            "### mat_44 trans_mult point_t ### \n"
            "qmtc2      %[col0_val], $vf1        \n"
            "qmtc2      %[col1_val], $vf2        \n"
            "qmtc2      %[col2_val], $vf3        \n"
            "qmtc2      %[col3_val], $vf4        \n"
            "qmtc2      %[vec_val], $vf5         \n"
            "vmul       $vf6, $vf1, $vf5         \n"
            "vmul       $vf7, $vf2, $vf5         \n"
            "vmul       $vf8, $vf3, $vf5         \n"
            "vmul       $vf9, $vf4, $vf5         \n"
            "vaddy.x    $vf6, $vf6, $vf6         \n"
            "vaddx.y    $vf7, $vf7, $vf7         \n"
            "vaddx.z    $vf8, $vf8, $vf8         \n"
            "vaddx.w    $vf9, $vf4, $vf9         \n"
            "vaddz.x    $vf6, $vf6, $vf6         \n"
            "vaddz.y    $vf7, $vf7, $vf7         \n"
            "vaddy.z    $vf8, $vf8, $vf8         \n"
            "vaddy.w    $vf9, $vf9, $vf9         \n"
            "vaddw.x    $vf10, $vf6, $vf1        \n"
            "vaddw.y    $vf10, $vf7, $vf2        \n"
            "vaddw.z    $vf10, $vf8, $vf3        \n"
            "vaddz.w    $vf10, $vf9, $vf9        \n"
            "qmfc2      %[result], $vf10         \n"
            : [result] "=&r"(result)
            : [col0_val] "r"(col0_val), [col1_val] "r"(col1_val),
              [col2_val] "r"(col2_val), [col3_val] "r"(col3_val), [vec_val] "r"(vec_val)
            : "memory");
        return vec_4(result);
    }

    // matrix/matrix operations

    mat_44 operator+(const mat_44& mat) const;
    mat_44 operator-(const mat_44& mat) const;

    void operator+=(const mat_44& mat)
    {
        *this = *this + mat;
    }
    void operator-=(const mat_44& mat)
    {
        *this = *this - mat;
    }

    mat_44 operator*(const mat_44& mat) const;
    mat_43 operator*(const mat_43& mat) const;
    mat_44 operator*(const transform_t& mat) const;

    mat_44 trans_mult(const mat_44& mat) const;
    mat_43 trans_mult(const mat_43& mat) const;
    mat_44 trans_mult(const transform_t& mat) const;

    mat_44 mult_trans(const mat_44& mat) const;
    mat_43 mult_trans(const mat_34& mat) const;

    mat_44&
    operator=(const mat_33& mat)
    {
        col0 = mat.col0;
        col1 = mat.col1;
        col2 = mat.col2;
        return *this;
    }

    mat_44&
    operator=(const mat_34& mat)
    {
        col0 = mat.col0;
        col1 = mat.col1;
        col2 = mat.col2;
        col3 = mat.col3;
        return *this;
    }

    mat_44&
    operator=(const mat_43& mat)
    {
        col0 = mat.col0;
        col1 = mat.col1;
        col2 = mat.col2;
        return *this;
    }

    mat_44&
    operator=(const mat_44& mat)
    {
        col0 = mat.col0;
        col1 = mat.col1;
        col2 = mat.col2;
        col3 = mat.col3;
        return *this;
    }

    // matrix/zero_matrix operations

    mat_44 operator+(const zero_44 zero) const;
    mat_44 operator-(const zero_44 zero) const;

    void operator+=(const zero_44 zero);
    void operator-=(const zero_44 zero);

    zero_44 operator*(const zero_44 zero) const;
    zero_43 operator*(const zero_43 zero) const;

    zero_44 trans_mult(const zero_44 zero) const;
    zero_43 trans_mult(const zero_43 zero) const;

    zero_44 mult_trans(const zero_44 zero) const;
    zero_43 mult_trans(const zero_34 zero) const;

    mat_44& operator=(const zero_44 zero);

    void print() const
    {
        get_row0().print();
        get_row1().print();
        get_row2().print();
        get_row3().print();
    }

    void print(const char* mat_name) const
    {
        printf("%s:\n", mat_name);
        print();
    }
};

/********************************************
 * transform_t - a transform type consisting of
 * 3 vectors and a point
 */

class transform_t {
public:
    vector_t col0;
    vector_t col1;
    vector_t col2;
    point_t col3;

    transform_t() {}

    transform_t(const transform_t& mat)
    {
        col0 = mat.col0;
        col1 = mat.col1;
        col2 = mat.col2;
        col3 = mat.col3;
    }

    void
    set_rotation(mat_33 rotation)
    {
        col0 = vector_t(rotation.col0);
        col1 = vector_t(rotation.col1);
        col2 = vector_t(rotation.col2);
    }

    template <class translate_type>
    void
    set_translation(translate_type translation)
    {
        col3 = point_t(translation);
    }

    template <class translate_type>
    void
    set(mat_33 rotation, translate_type translation)
    {
        set_rotation(rotation);
        set_translation(translation);
    }

    template <class translate_type>
    transform_t(const mat_33& rotation, translate_type translation)
    {
        set(rotation, translation);
    }

    template <class translate_type>
    transform_t(vec_4 quat, translate_type translation)
    {
        set(mat_33(quat), translation);
    }

    transform_t(const vector_t col_0, const vector_t col_1,
        const vector_t col_2, const point_t col_3)
    {
        col0 = col_0;
        col1 = col_1;
        col2 = col_2;
        col3 = col_3;
    }

    void set_identity()
    {
        asm(" ### transform_t::set_identity ### \n"
            "vsub	%[col0], %[col0], %[col0] \n"
            "vsub	%[col1], %[col1], %[col1] \n"
            "vmr32	%[col2], vf00 \n"
            "vmove	%[col3], vf00 \n"
            "vaddw.x	%[col0], vf00, vf00 \n"
            "vaddw.y	%[col1], vf00, vf00 \n"
            : [col0] "=j"(col0),
            [col1] "=j"(col1),
            [col2] "=j"(col2),
            [col3] "=j"(col3));
    }

    void set_zero()
    {
        asm(" ### transform_t::set_zero ### \n"
            "vsub	%[col0], %[col0], %[col0] \n"
            "vsub	%[col1], %[col1], %[col1] \n"
            "vsub	%[col2], %[col2], %[col2] \n"
            "vsub	%[col3], %[col3], %[col3] \n"
            : [col0] "=j"(col0),
            [col1] "=j"(col1),
            [col2] "=j"(col2),
            [col3] "=j"(col3));
    }

    void
    set_rotate_x(float angle)
    {
        float cs = cosf(angle);
        float sn = sinf(angle);
        col0.set(1, 0, 0);
        col1.set(0, cs, sn);
        col2.set(0, -sn, cs);
        col3.set_zero();
    }

    void
    set_rotate_y(float angle)
    {
        float cs = cosf(angle);
        float sn = sinf(angle);
        col0.set(cs, 0, -sn);
        col1.set(0, 1, 0);
        col2.set(sn, 0, cs);
        col3.set_zero();
    }

    void
    set_rotate_z(float angle)
    {
        float cs = cosf(angle);
        float sn = sinf(angle);
        col0.set(cs, sn, 0);
        col1.set(-sn, cs, 0);
        col2.set(0, 0, 1);
        col3.set_zero();
    }

    void set_col0(vector_t new_col) { col0 = new_col; }
    void set_col1(vector_t new_col) { col1 = new_col; }
    void set_col2(vector_t new_col) { col2 = new_col; }
    void set_col3(point_t new_col) { col3 = new_col; }

    void set_row0(vec_4 new_row)
    {
        asm(
            " ### transform_t::set_row0 ### \n"
            "vaddx.x	%[col0], vf00, %[new_row] \n"
            "vaddy.x	%[col1], vf00, %[new_row] \n"
            "vaddz.x	%[col2], vf00, %[new_row] \n"
            "vaddw.x	%[col3], vf00, %[new_row] \n"
            : [col0] "+j"(col0),
            [col1] "+j"(col1),
            [col2] "+j"(col2),
            [col3] "+j"(col3)
            : [new_row] "j"(new_row));
    }

    void set_row1(vec_4 new_row)
    {
        asm(
            " ### transform_t::set_row1 ### \n"
            "vaddx.y	%[col0], vf00, %[new_row] \n"
            "vaddy.y	%[col1], vf00, %[new_row] \n"
            "vaddz.y	%[col2], vf00, %[new_row] \n"
            "vaddw.y	%[col3], vf00, %[new_row] \n"
            : [col0] "+j"(col0),
            [col1] "+j"(col1),
            [col2] "+j"(col2),
            [col3] "+j"(col3)
            : [new_row] "j"(new_row));
    }

    void set_row2(vec_4 new_row)
    {
        asm(
            " ### transform_t::set_row2 ### \n"
            "vaddx.z	%[col0], vf00, %[new_row] \n"
            "vaddy.z	%[col1], vf00, %[new_row] \n"
            "vaddz.z	%[col2], vf00, %[new_row] \n"
            "vaddw.z	%[col3], vf00, %[new_row] \n"
            : [col0] "+j"(col0),
            [col1] "+j"(col1),
            [col2] "+j"(col2),
            [col3] "+j"(col3)
            : [new_row] "j"(new_row));
    }

    vector_t get_col0() const { return col0; }
    vector_t get_col1() const { return col1; }
    vector_t get_col2() const { return col2; }
    point_t get_col3() const { return col3; }

    vec_4 get_row0() const
    {
        vec128_t c0 = col0.vec128, c1 = col1.vec128, c2 = col2.vec128, c3 = col3.vec128;
        vec128_t row;
        asm __volatile__ (" ### transform_t::get_row0 ### \n"
            "qmtc2 %[c0], $vf1  \n"
            "qmtc2 %[c1], $vf2  \n"
            "qmtc2 %[c2], $vf3  \n"
            "qmtc2 %[c3], $vf4  \n"
            "vaddx.x $vf5, $vf0, $vf1 \n"
            "vaddx.y $vf5, $vf0, $vf2 \n"
            "vaddx.z $vf5, $vf0, $vf3 \n"
            "vmulx.w $vf5, $vf0, $vf4 \n"
            "qmfc2 %[row], $vf5  \n"
            : [row] "=r"(row)
            : [c0] "r"(c0), [c1] "r"(c1), [c2] "r"(c2), [c3] "r"(c3));
        return vec_4(row);
    }

    vec_4 get_row1() const
    {
        vec128_t c0 = col0.vec128, c1 = col1.vec128, c2 = col2.vec128, c3 = col3.vec128;
        vec128_t row;
        asm __volatile__ (" ### transform_t::get_row1 ### \n"
            "qmtc2 %[c0], $vf1  \n"
            "qmtc2 %[c1], $vf2  \n"
            "qmtc2 %[c2], $vf3  \n"
            "qmtc2 %[c3], $vf4  \n"
            "vaddy.x $vf5, $vf0, $vf1 \n"
            "vaddy.y $vf5, $vf0, $vf2 \n"
            "vaddy.z $vf5, $vf0, $vf3 \n"
            "vmuly.w $vf5, $vf0, $vf4 \n"
            "qmfc2 %[row], $vf5  \n"
            : [row] "=r"(row)
            : [c0] "r"(c0), [c1] "r"(c1), [c2] "r"(c2), [c3] "r"(c3));
        return vec_4(row);
    }

    vec_4 get_row2() const
    {
        vec128_t row;
        asm __volatile__ (" ### transform_t::get_row2 ### \n"
            "lqc2 $vf1, 0(%[T])   \n"
            "lqc2 $vf2, 16(%[T])  \n"
            "lqc2 $vf3, 32(%[T])  \n"
            "lqc2 $vf4, 48(%[T])  \n"
            "vaddz.x $vf5, $vf0, $vf1 \n"
            "vaddz.y $vf5, $vf0, $vf2 \n"
            "vaddz.z $vf5, $vf0, $vf3 \n"
            "vmulz.w $vf5, $vf0, $vf4 \n"
            "qmfc2 %[row], $vf5  \n"
            : [row] "=r"(row)
            : [T] "r"(this)
            : "memory");
        return vec_4(row);
    }

    vec_4 get_row3() const
    {
        vec128_t row;
        asm(" ### transform_t::get_row3 ### \n"
            "vmove %0, vf00 \n"
            : "=j"(row));
        return vec_4(row);
    }

    // inverses

    transform_t
    inverse() const;

    void
    inverse_in_place()
    {
        *this = inverse();
    }

    transform_t
    orthonormal_inverse() const;

    void
    orthonormal_inverse_in_place()
    {
        asm __volatile__ (
            "### transform_t::orthonormal_inverse_in_place ### \n"
            "lqc2 $vf1, 0(%[T])    \n"
            "lqc2 $vf2, 16(%[T])   \n"
            "lqc2 $vf3, 32(%[T])   \n"
            "lqc2 $vf4, 48(%[T])   \n"
            "vadd.xz   $vf5, $vf0, $vf2  \n"
            "vaddx.y   $vf5, $vf0, $vf3  \n"
            "vaddy.x   $vf2, $vf0, $vf1  \n"
            "vaddy.z   $vf2, $vf0, $vf3  \n"
            "vaddz.x   $vf3, $vf0, $vf1  \n"
            "vaddy.z   $vf1, $vf0, $vf5  \n"
            "vaddx.y   $vf1, $vf0, $vf5  \n"
            "vaddz.y   $vf3, $vf0, $vf5  \n"
            "vsuba     $ACC, $vf0, $vf0  \n"
            "vmsubay   $ACC, $vf2, $vf4  \n"
            "vmsubax   $ACC, $vf1, $vf4  \n"
            "vmsubz    $vf4, $vf3, $vf4  \n"
            "sqc2 $vf1, 0(%[T])    \n"
            "sqc2 $vf2, 16(%[T])   \n"
            "sqc2 $vf3, 32(%[T])   \n"
            "sqc2 $vf4, 48(%[T])   \n"
            : "=r"(vu0_ACC)
            : [T] "r"(this)
            : "memory");
    }

    // matrix/vector operations

    vec_4
    operator*(vec_4 vec) const
    {
        vec128_t v = vec.vec128;
        vec128_t result;
        asm __volatile__ (
            " ### transform_t * vec_4 ### \n"
            "lqc2 $vf1, 0(%[T])    \n"
            "lqc2 $vf2, 16(%[T])   \n"
            "lqc2 $vf3, 32(%[T])   \n"
            "lqc2 $vf4, 48(%[T])   \n"
            "qmtc2 %[v],  $vf5     \n"
            "vsuba     $ACC, $vf0, $vf0  \n"
            "vmulax    $ACC, $vf1, $vf5  \n"
            "vmadday   $ACC, $vf2, $vf5  \n"
            "vmaddaz   $ACC, $vf3, $vf5  \n"
            "vmaddw    $vf6, $vf4, $vf5  \n"
            "qmfc2 %[result], $vf6       \n"
            : [result] "=&r"(result), [acc] "=r"(vu0_ACC)
            : [T] "r"(this), [v] "r"(v)
            : "memory");
        return vec_4(result);
    }

    vector_t
    operator*(vector_t vec) const
    {
        vec128_t v = vec.vec128;
        vec128_t result;
        asm __volatile__ (
            " ### transform_t * vector_t ### \n"
            "lqc2 $vf1, 0(%[T])    \n"
            "lqc2 $vf2, 16(%[T])   \n"
            "lqc2 $vf3, 32(%[T])   \n"
            "qmtc2 %[v],  $vf4     \n"
            "vsuba     $ACC, $vf0, $vf0  \n"
            "vmulax    $ACC, $vf1, $vf4  \n"
            "vmadday   $ACC, $vf2, $vf4  \n"
            "vmaddz    $vf5, $vf3, $vf4  \n"
            "qmfc2 %[result], $vf5       \n"
            : [result] "=&r"(result), [acc] "=r"(vu0_ACC)
            : [T] "r"(this), [v] "r"(v)
            : "memory");
        return vector_t(result);
    }

    point_t
    operator*(point_t pt) const
    {
        vec128_t p = pt.vec128;
        vec128_t result;
        asm __volatile__ (
            " ### transform_t * point_t ### \n"
            "lqc2 $vf1, 0(%[T])    \n"
            "lqc2 $vf2, 16(%[T])   \n"
            "lqc2 $vf3, 32(%[T])   \n"
            "lqc2 $vf4, 48(%[T])   \n"
            "qmtc2 %[p],  $vf5     \n"
            "vsuba     $ACC, $vf0, $vf0  \n"
            "vmulax    $ACC, $vf1, $vf5  \n"
            "vmadday   $ACC, $vf2, $vf5  \n"
            "vmaddaz   $ACC, $vf3, $vf5  \n"
            "vmaddw    $vf6, $vf4, $vf0  \n"
            "qmfc2 %[result], $vf6       \n"
            : [result] "=&r"(result), [acc] "=r"(vu0_ACC)
            : [T] "r"(this), [p] "r"(p)
            : "memory");
        return point_t(result);
    }

    // matrix/matrix operations

    transform_t operator*(const transform_t& mat) const;
    mat_44 operator*(const mat_44& mat) const;
    mat_43 operator*(const mat_43& mat) const;

    void operator=(const transform_t& xform)
    {
        col0 = xform.col0;
        col1 = xform.col1;
        col2 = xform.col2;
        col3 = xform.col3;
    }

    void print() const
    {
        get_row0().print();
        get_row1().print();
        get_row2().print();
        get_row3().print();
    }

    void print(const char* mat_name) const
    {
        printf("%s:\n", mat_name);
        print();
    }
};

/********************************************
 * matrix operations
 */

// vec_3

inline mat_33
    vec_xyz::operator~() const
{
    mat_33 result;
    vec128_t v = vec128;
    asm __volatile__ ("### make tilde matrix from vec_xyz ### \n"
        "qmtc2 %[v], $vf4              \n"
        "vsub     $vf1, $vf0, $vf0     \n"
        "vsub     $vf2, $vf0, $vf0     \n"
        "vmr32    $vf3, $vf0           \n"
        "vaddw.x  $vf1, $vf0, $vf0     \n"
        "vaddw.y  $vf2, $vf0, $vf0     \n"
        "vopmula  $ACC, $vf4, $vf1     \n"
        "vopmsub  $vf1, $vf1, $vf4     \n"
        "vopmula  $ACC, $vf4, $vf2     \n"
        "vopmsub  $vf2, $vf2, $vf4     \n"
        "vopmula  $ACC, $vf4, $vf3     \n"
        "vopmsub  $vf3, $vf3, $vf4     \n"
        "qmfc2 %[r0], $vf1             \n"
        "qmfc2 %[r1], $vf2             \n"
        "qmfc2 %[r2], $vf3             \n"
        : [r0] "=&r"(result.col0), [r1] "=&r"(result.col1), [r2] "=&r"(result.col2), "=r"(vu0_ACC)
        : [v] "r"(v));
    return result;
}

inline mat_33
vec_xyz::tilde_mult(const mat_33& mat) const
{
    mat_33 result;
    result.col0 = this->cross(mat.col0);
    result.col1 = this->cross(mat.col1);
    result.col2 = this->cross(mat.col2);
    return result;
}

inline mat_34
vec_xyz::tilde_mult(const mat_34& mat) const
{
    mat_34 result;
    result.col0 = this->cross(mat.col0);
    result.col1 = this->cross(mat.col1);
    result.col2 = this->cross(mat.col2);
    result.col3 = this->cross(mat.col3);
    return result;
}

inline vec_xyz
    vec_xyz::operator*(const mat_33& mat) const
{
    vec128_t result;
    vec128_t col0_val = mat.col0.vec128;
    vec128_t col1_val = mat.col1.vec128;
    vec128_t col2_val = mat.col2.vec128;
    vec128_t vec_val = vec128;
    asm __volatile__(
        "### vec_xyz (row) * mat_33 ### \n"
        "qmtc2      %[col0_val], $vf1        \n"
        "qmtc2      %[col1_val], $vf2        \n"
        "qmtc2      %[col2_val], $vf3        \n"
        "qmtc2      %[vec_val], $vf4         \n"
        "vmaxw      $vf5, $vf0, $vf0         \n"
        "vmul       $vf6, $vf4, $vf1         \n"
        "vmul       $vf7, $vf4, $vf2         \n"
        "vmul       $vf8, $vf4, $vf3         \n"
        "vadday.x   $ACC, $vf6, $vf6         \n"
        "vmaddz.x   $vf9, $vf5, $vf6         \n"
        "vaddax.y   $ACC, $vf7, $vf7         \n"
        "vmaddz.y   $vf9, $vf5, $vf7         \n"
        "vaddax.z   $ACC, $vf8, $vf8         \n"
        "vmaddy.z   $vf9, $vf5, $vf8         \n"
        "qmfc2      %[result], $vf9          \n"
        : [result] "=&r"(result), [acc] "=r"(vu0_ACC)
        : [col0_val] "r"(col0_val), [col1_val] "r"(col1_val),
          [col2_val] "r"(col2_val), [vec_val] "r"(vec_val)
        : "memory");
    return vec_xyz(result);
}

inline vec_xyzw
    vec_xyz::operator*(const mat_34& mat) const
{
    vec128_t result;
    vec128_t col0_val = mat.col0.vec128;
    vec128_t col1_val = mat.col1.vec128;
    vec128_t col2_val = mat.col2.vec128;
    vec128_t col3_val = mat.col3.vec128;
    vec128_t vec_val = vec128;
    asm __volatile__(
        "### vec_xyz (row) * mat_34 ### \n"
        "qmtc2      %[col0_val], $vf1        \n"
        "qmtc2      %[col1_val], $vf2        \n"
        "qmtc2      %[col2_val], $vf3        \n"
        "qmtc2      %[col3_val], $vf4        \n"
        "qmtc2      %[vec_val], $vf5         \n"
        "vmaxw      $vf6, $vf0, $vf0         \n"
        "vmul       $vf7, $vf5, $vf1         \n"
        "vmul       $vf8, $vf5, $vf2         \n"
        "vmul       $vf9, $vf5, $vf3         \n"
        "vmul       $vf10, $vf5, $vf4        \n"
        "vmulx.w    $vf10, $vf0, $vf10       \n"
        "vadday.x   $ACC, $vf7, $vf7         \n"
        "vmaddz.x   $vf11, $vf6, $vf7        \n"
        "vaddax.y   $ACC, $vf8, $vf8         \n"
        "vmaddz.y   $vf11, $vf6, $vf8        \n"
        "vaddax.z   $ACC, $vf9, $vf9         \n"
        "vmaddy.z   $vf11, $vf6, $vf9        \n"
        "vadday.w   $ACC, $vf10, $vf10       \n"
        "vmaddz.w   $vf11, $vf6, $vf10       \n"
        "qmfc2      %[result], $vf11         \n"
        : [result] "=&r"(result), [acc] "=r"(vu0_ACC)
        : [col0_val] "r"(col0_val), [col1_val] "r"(col1_val),
          [col2_val] "r"(col2_val), [col3_val] "r"(col3_val), [vec_val] "r"(vec_val)
        : "memory");
    return vec_xyzw(result);
}

inline mat_33
vec_xyz::tensor_mult(const vec_3 vec) const
{
    mat_33 result;
    result.col0 = vec * vec_x(*this);
    result.col1 = vec * vec_y(*this);
    result.col2 = vec * vec_z(*this);
    return result;
}

inline mat_43
vec_xyz::tensor_mult(const vec_4 vec) const
{
    mat_43 result;
    result.col0 = vec * vec_x(*this);
    result.col1 = vec * vec_y(*this);
    result.col2 = vec * vec_z(*this);
    return result;
}

// vec_4

inline vec_xyz
    vec_xyzw::operator*(const mat_43& mat) const
{
    vec128_t result;
    vec128_t col0_val = mat.col0.vec128;
    vec128_t col1_val = mat.col1.vec128;
    vec128_t col2_val = mat.col2.vec128;
    vec128_t vec_val = vec128;
    asm __volatile__(
        "### vec_xyzw (row) * mat_43 ### \n"
        "qmtc2      %[col0_val], $vf1        \n"
        "qmtc2      %[col1_val], $vf2        \n"
        "qmtc2      %[col2_val], $vf3        \n"
        "qmtc2      %[vec_val], $vf4         \n"
        "vmaxw      $vf5, $vf0, $vf0         \n"
        "vmul       $vf6, $vf4, $vf1         \n"
        "vmul       $vf7, $vf4, $vf2         \n"
        "vmul       $vf8, $vf4, $vf3         \n"
        "vadday.x   $ACC, $vf6, $vf6         \n"
        "vmaddaz.x  $ACC, $vf5, $vf6         \n"
        "vmaddw.x   $vf9, $vf5, $vf6         \n"
        "vaddax.y   $ACC, $vf7, $vf7         \n"
        "vmaddaz.y  $ACC, $vf5, $vf7         \n"
        "vmaddw.y   $vf9, $vf5, $vf7         \n"
        "vaddax.z   $ACC, $vf8, $vf8         \n"
        "vmadday.z  $ACC, $vf5, $vf8         \n"
        "vmaddw.z   $vf9, $vf5, $vf8         \n"
        "qmfc2      %[result], $vf9          \n"
        : [result] "=&r"(result), [acc] "=r"(vu0_ACC)
        : [col0_val] "r"(col0_val), [col1_val] "r"(col1_val),
          [col2_val] "r"(col2_val), [vec_val] "r"(vec_val)
        : "memory");
    return vec_xyz(result);
}

inline vec_xyzw
    vec_xyzw::operator*(const mat_44& mat) const
{
    vec128_t result;
    vec128_t col0_val = mat.col0.vec128;
    vec128_t col1_val = mat.col1.vec128;
    vec128_t col2_val = mat.col2.vec128;
    vec128_t col3_val = mat.col3.vec128;
    vec128_t vec_val = vec128;
    asm __volatile__(
        "### vec_xyzw (row) * mat_44 ### \n"
        "qmtc2      %[col0_val], $vf1        \n"
        "qmtc2      %[col1_val], $vf2        \n"
        "qmtc2      %[col2_val], $vf3        \n"
        "qmtc2      %[col3_val], $vf4        \n"
        "qmtc2      %[vec_val], $vf5         \n"
        "vmaxw      $vf6, $vf0, $vf0         \n"
        "vmul       $vf7, $vf5, $vf1         \n"
        "vmul       $vf8, $vf5, $vf2         \n"
        "vmul       $vf9, $vf5, $vf3         \n"
        "vmul       $vf10, $vf5, $vf4        \n"
        "vadday.x   $ACC, $vf7, $vf7         \n"
        "vmaddaz.x  $ACC, $vf6, $vf7         \n"
        "vmaddw.x   $vf11, $vf6, $vf7        \n"
        "vaddax.y   $ACC, $vf8, $vf8         \n"
        "vmaddaz.y  $ACC, $vf6, $vf8         \n"
        "vmaddw.y   $vf11, $vf6, $vf8        \n"
        "vaddax.z   $ACC, $vf9, $vf9         \n"
        "vmadday.z  $ACC, $vf6, $vf9         \n"
        "vmaddw.z   $vf11, $vf6, $vf9        \n"
        "vaddax.w   $ACC, $vf10, $vf10       \n"
        "vmadday.w  $ACC, $vf6, $vf10        \n"
        "vmaddz.w   $vf11, $vf6, $vf10       \n"
        "qmfc2      %[result], $vf11         \n"
        : [result] "=&r"(result), [acc] "=r"(vu0_ACC)
        : [col0_val] "r"(col0_val), [col1_val] "r"(col1_val),
          [col2_val] "r"(col2_val), [col3_val] "r"(col3_val), [vec_val] "r"(vec_val)
        : "memory");
    return vec_xyzw(result);
}

inline mat_34
vec_xyzw::tensor_mult(const vec_3 vec) const
{
    mat_34 result;
    result.col0 = vec * vec_x(*this);
    result.col1 = vec * vec_y(*this);
    result.col2 = vec * vec_z(*this);
    result.col3 = vec * vec_w(*this);
    return result;
}

inline mat_44
vec_xyzw::tensor_mult(const vec_4 vec) const
{
    mat_44 result;
    result.col0 = vec * vec_x(*this);
    result.col1 = vec * vec_y(*this);
    result.col2 = vec * vec_z(*this);
    result.col3 = vec * vec_w(*this);
    return result;
}

// mat_33

// explicit constructors

inline mat_33::mat_33(const mat_43& mat)
{
    col0 = vec_3(mat.col0);
    col1 = vec_3(mat.col1);
    col2 = vec_3(mat.col2);
}

inline mat_33::mat_33(const mat_34& mat)
{
    col0 = mat.col0;
    col1 = mat.col1;
    col2 = mat.col2;
}

inline mat_33::mat_33(const mat_44& mat)
{
    col0 = vec_3(mat.col0);
    col1 = vec_3(mat.col1);
    col2 = vec_3(mat.col2);
}

// negate

inline mat_33
mat_33::operator-() const
{
    mat_33 result;
    result.col0 = -col0;
    result.col1 = -col1;
    result.col2 = -col2;
    return result;
}

// transpose & inverse

inline mat_33
mat_33::transpose() const
{
    mat_33 result;
    vec128_t temp0, temp1, temp2, temp3;

    asm("### mat_33::transpose ### \n"
        "pextlw	%[temp0], %[col1], %[col0] \n"
        "pextuw	%[temp1], %[col1], %[col0] \n"
        "pextlw	%[temp2], $0, %[col2] \n"
        "pextuw	%[temp3], $0, %[col2] \n"
        : [temp0] "=&r"(temp0),
        [temp1] "=&r"(temp1),
        [temp2] "=&r"(temp2),
        [temp3] "=&r"(temp3)
        : [col0] "r"(col0),
        [col1] "r"(col1),
        [col2] "r"(col2));

    asm("pcpyld	%[res0], %[temp2], %[temp0] \n"
        "pcpyud	%[res1], %[temp0], %[temp2] \n"
        "pcpyld	%[res2], %[temp3], %[temp1] \n"
        : [res0] "=&r"(result.col0),
        [res1] "=&r"(result.col1),
        [res2] "=&r"(result.col2)
        : [temp0] "r"(temp0),
        [temp1] "r"(temp1),
        [temp2] "r"(temp2),
        [temp3] "r"(temp3));
    return result;
}

inline mat_33
mat_33::inverse() const
{
    mat_33 result;
    vec128_t c0 = col0.vec128, c1 = col1.vec128, c2 = col2.vec128;

    asm __volatile__ ("### mat_33::inverse ### \n"
        "qmtc2 %[c0], $vf1                              \n"
        "qmtc2 %[c1], $vf2                              \n"
        "qmtc2 %[c2], $vf3                              \n"
        "vopmula.xyz $ACC, $vf1, $vf2  # inv2 = col0 x col1 \n"
        "vopmsub.xyz $vf6, $vf2, $vf1                   \n"
        "vopmula.xyz $ACC, $vf2, $vf3  # inv0 = col1 x col2 \n"
        "vopmsub.xyz $vf4, $vf3, $vf2                   \n"
        "vmul        $vf9, $vf3, $vf6  # det components     \n"
        "vaddw.x     $vf8, $vf0, $vf0  # temp.x = 1.0       \n"
        "vopmula.xyz $ACC, $vf3, $vf1  # inv1 = col2 x col0 \n"
        "vopmsub.xyz $vf5, $vf1, $vf3                   \n"
        "vadday.x    $ACC, $vf9, $vf9                   \n"
        "vmaddz.x    $vf9, $vf8, $vf9  # det = det.x+det.y+det.z \n"
        "vaddx.y     $vf8, $vf0, $vf6  # transpose           \n"
        "vadd.xz     $vf8, $vf0, $vf5                   \n"
        "vaddy.x     $vf5, $vf0, $vf4                   \n"
        "vdiv        $Q, $vf0w, $vf9x   # Q = 1/det          \n"
        "vaddy.z     $vf5, $vf0, $vf6                   \n"
        "vaddz.x     $vf6, $vf0, $vf4                   \n"
        "vaddy.z     $vf4, $vf0, $vf8                   \n"
        "vaddx.y     $vf4, $vf0, $vf8                   \n"
        "vaddz.y     $vf6, $vf0, $vf8                   \n"
        "vwaitq                                         \n"
        "vmulq.xyz   $vf5, $vf5, $Q                      \n"
        "vmulq.xyz   $vf4, $vf4, $Q     # scale by 1/det     \n"
        "vmulq.xyz   $vf6, $vf6, $Q                      \n"
        "qmfc2 %[r0], $vf4                              \n"
        "qmfc2 %[r1], $vf5                              \n"
        "qmfc2 %[r2], $vf6                              \n"
        : [r0] "=&r"(result.col0), [r1] "=&r"(result.col1), [r2] "=&r"(result.col2), "=r"(vu0_ACC)
        : [c0] "r"(c0), [c1] "r"(c1), [c2] "r"(c2));
    return result;
}

// matrix/scalar operations

inline mat_33
    mat_33::operator*(float scale) const
{
    mat_33 result;
    vec_x vec(scale);

    result.col0 = col0 * vec;
    result.col1 = col1 * vec;
    result.col2 = col2 * vec;
    return result;
}

inline mat_33
    mat_33::operator*(const vec_x scale) const
{
    mat_33 result;
    result.col0 = col0 * scale;
    result.col1 = col1 * scale;
    result.col2 = col2 * scale;
    return result;
}

inline mat_33
    mat_33::operator*(const vec_y scale) const
{
    mat_33 result;
    result.col0 = col0 * scale;
    result.col1 = col1 * scale;
    result.col2 = col2 * scale;
    return result;
}

inline mat_33
    mat_33::operator*(const vec_z scale) const
{
    mat_33 result;
    result.col0 = col0 * scale;
    result.col1 = col1 * scale;
    result.col2 = col2 * scale;
    return result;
}

inline mat_33
    mat_33::operator*(const vec_w scale) const
{
    mat_33 result;
    result.col0 = col0 * scale;
    result.col1 = col1 * scale;
    result.col2 = col2 * scale;
    return result;
}

// matrix/vector operations

inline mat_33
mat_33::mult_tilde(vec_3 vec) const
{
    mat_33 result;
    vec128_t c0 = col0.vec128, c1 = col1.vec128, c2 = col2.vec128, v = vec.vec128;
    asm __volatile__ ("### mat_33 mult_tilde vec_3 ### \n"
        "qmtc2 %[c0], $vf1  \n"
        "qmtc2 %[c1], $vf2  \n"
        "qmtc2 %[c2], $vf3  \n"
        "qmtc2 %[v],  $vf4  \n"
        "vmulaz $ACC, $vf2, $vf4 \n"
        "vmsuby $vf5, $vf3, $vf4 \n"
        "vmulax $ACC, $vf3, $vf4 \n"
        "vmsubz $vf6, $vf1, $vf4 \n"
        "vmulay $ACC, $vf1, $vf4 \n"
        "vmsubx $vf7, $vf2, $vf4 \n"
        "qmfc2 %[r0], $vf5  \n"
        "qmfc2 %[r1], $vf6  \n"
        "qmfc2 %[r2], $vf7  \n"
        : [r0] "=&r"(result.col0), [r1] "=&r"(result.col1), [r2] "=&r"(result.col2), "=r"(vu0_ACC)
        : [c0] "r"(c0), [c1] "r"(c1), [c2] "r"(c2), [v] "r"(v));
    return result;
}

// matrix/matrix operations

inline mat_33
mat_33::operator+(const mat_33& mat) const
{
    mat_33 result;
    result.col0 = col0 + mat.get_col0();
    result.col1 = col1 + mat.get_col1();
    result.col2 = col2 + mat.get_col2();
    return result;
}

inline mat_33
mat_33::operator-(const mat_33& mat) const
{
    mat_33 result;
    result.col0 = col0 - mat.get_col0();
    result.col1 = col1 - mat.get_col1();
    result.col2 = col2 - mat.get_col2();
    return result;
}

inline mat_33
    mat_33::operator*(const mat_33& mat) const
{
    mat_33 result;
    result.col0 = *this * mat.get_col0();
    result.col1 = *this * mat.get_col1();
    result.col2 = *this * mat.get_col2();
    return result;
}

inline mat_34
    mat_33::operator*(const mat_34& mat) const
{
    mat_34 result;
    result.col0 = *this * mat.get_col0();
    result.col1 = *this * mat.get_col1();
    result.col2 = *this * mat.get_col2();
    result.col3 = *this * mat.get_col3();
    return result;
}

inline mat_33
mat_33::trans_mult(const mat_33& mat) const
{
    mat_33 result;
    result.col0 = this->trans_mult(mat.get_col0());
    result.col1 = this->trans_mult(mat.get_col1());
    result.col2 = this->trans_mult(mat.get_col2());
    return result;
}

inline mat_34
mat_33::trans_mult(const mat_34& mat) const
{
    mat_34 result;
    result.col0 = this->trans_mult(mat.get_col0());
    result.col1 = this->trans_mult(mat.get_col1());
    result.col2 = this->trans_mult(mat.get_col2());
    result.col3 = this->trans_mult(mat.get_col3());
    return result;
}

inline mat_33
mat_33::mult_trans(const mat_33& mat) const
{
    mat_33 result;
    result.col0 = this->mult_trans_col0(mat);
    result.col1 = this->mult_trans_col1(mat);
    result.col2 = this->mult_trans_col2(mat);
    return result;
}

inline mat_34
mat_33::mult_trans(const mat_43& mat) const
{
    mat_34 result;
    result.col0 = this->mult_trans_col0(mat);
    result.col1 = this->mult_trans_col1(mat);
    result.col2 = this->mult_trans_col2(mat);
    result.col3 = this->mult_trans_col3(mat);
    return result;
}
// mat_44

// explicit constructors

inline mat_44::mat_44(const transform_t& mat)
{
    col0 = vec_4(mat.col0);
    col1 = vec_4(mat.col1);
    col2 = vec_4(mat.col2);
    col3 = vec_4(mat.col3);
}

// negate

inline mat_44
mat_44::operator-() const
{
    mat_44 result;
    result.col0 = -col0;
    result.col1 = -col1;
    result.col2 = -col2;
    result.col3 = -col3;
    return result;
}

// transpose

inline mat_44
mat_44::transpose() const
{
    mat_44 result;
    vec128_t temp0, temp1, temp2, temp3;

    asm("### mat_44::transpose ### \n"
        "pextlw	%[temp0], %[col1], %[col0] \n"
        "pextuw	%[temp1], %[col1], %[col0] \n"
        "pextlw	%[temp2], %[col3], %[col2] \n"
        "pextuw	%[temp3], %[col3], %[col2] \n"
        : [temp0] "=&r"(temp0),
        [temp1] "=&r"(temp1),
        [temp2] "=&r"(temp2),
        [temp3] "=&r"(temp3)
        : [col0] "r"(col0),
        [col1] "r"(col1),
        [col2] "r"(col2),
        [col3] "r"(col3));

    asm("pcpyld	%[res0], %[temp2], %[temp0] \n"
        "pcpyud	%[res1], %[temp0], %[temp2] \n"
        "pcpyld	%[res2], %[temp3], %[temp1] \n"
        "pcpyud	%[res3], %[temp1], %[temp3] \n"
        : [res0] "=&r"(result.col0),
        [res1] "=&r"(result.col1),
        [res2] "=&r"(result.col2),
        [res3] "=&r"(result.col3)
        : [temp0] "r"(temp0),
        [temp1] "r"(temp1),
        [temp2] "r"(temp2),
        [temp3] "r"(temp3));
    return result;
}

// matrix/scalar operations

inline mat_44
    mat_44::operator*(float scale) const
{
    mat_44 result;
    vec_x vec(scale);

    result.col0 = col0 * vec;
    result.col1 = col1 * vec;
    result.col2 = col2 * vec;
    result.col3 = col3 * vec;
    return result;
}

inline mat_44
    mat_44::operator*(const vec_x scale) const
{
    mat_44 result;
    result.col0 = col0 * scale;
    result.col1 = col1 * scale;
    result.col2 = col2 * scale;
    result.col3 = col3 * scale;
    return result;
}

inline mat_44
    mat_44::operator*(const vec_y scale) const
{
    mat_44 result;
    result.col0 = col0 * scale;
    result.col1 = col1 * scale;
    result.col2 = col2 * scale;
    result.col3 = col3 * scale;
    return result;
}

inline mat_44
    mat_44::operator*(const vec_z scale) const
{
    mat_44 result;
    result.col0 = col0 * scale;
    result.col1 = col1 * scale;
    result.col2 = col2 * scale;
    result.col3 = col3 * scale;
    return result;
}

inline mat_44
    mat_44::operator*(const vec_w scale) const
{
    mat_44 result;
    result.col0 = col0 * scale;
    result.col1 = col1 * scale;
    result.col2 = col2 * scale;
    result.col3 = col3 * scale;
    return result;
}

// matrix/matrix operations

inline mat_44
mat_44::operator+(const mat_44& mat) const
{
    mat_44 result;
    result.col0 = col0 + mat.get_col0();
    result.col1 = col1 + mat.get_col1();
    result.col2 = col2 + mat.get_col2();
    result.col3 = col3 + mat.get_col3();
    return result;
}

inline mat_44
mat_44::operator-(const mat_44& mat) const
{
    mat_44 result;
    result.col0 = col0 - mat.get_col0();
    result.col1 = col1 - mat.get_col1();
    result.col2 = col2 - mat.get_col2();
    result.col3 = col3 - mat.get_col3();
    return result;
}

inline mat_44
    mat_44::operator*(const mat_44& mat) const
{
    mat_44 result;
    result.col0 = *this * mat.get_col0();
    result.col1 = *this * mat.get_col1();
    result.col2 = *this * mat.get_col2();
    result.col3 = *this * mat.get_col3();
    return result;
}

inline mat_43
    mat_44::operator*(const mat_43& mat) const
{
    mat_43 result;
    result.col0 = *this * mat.get_col0();
    result.col1 = *this * mat.get_col1();
    result.col2 = *this * mat.get_col2();
    return result;
}

inline mat_44
    mat_44::operator*(const transform_t& mat) const
{
    mat_44 result;
    result.col0 = *this * mat.get_col0();
    result.col1 = *this * mat.get_col1();
    result.col2 = *this * mat.get_col2();
    result.col3 = *this * mat.get_col3();
    return result;
}

inline mat_44
mat_44::trans_mult(const mat_44& mat) const
{
    mat_44 result;
    result.col0 = this->trans_mult(mat.get_col0());
    result.col1 = this->trans_mult(mat.get_col1());
    result.col2 = this->trans_mult(mat.get_col2());
    result.col3 = this->trans_mult(mat.get_col3());
    return result;
}

inline mat_43
mat_44::trans_mult(const mat_43& mat) const
{
    mat_43 result;
    result.col0 = this->trans_mult(mat.get_col0());
    result.col1 = this->trans_mult(mat.get_col1());
    result.col2 = this->trans_mult(mat.get_col2());
    return result;
}

inline mat_44
mat_44::trans_mult(const transform_t& mat) const
{
    mat_44 result;
    result.col0 = this->trans_mult(mat.get_col0());
    result.col1 = this->trans_mult(mat.get_col1());
    result.col2 = this->trans_mult(mat.get_col2());
    result.col3 = this->trans_mult(mat.get_col3());
    return result;
}

inline mat_44
mat_44::mult_trans(const mat_44& mat) const
{
    mat_44 result;
    result.col0 = this->mult_trans_col0(mat);
    result.col1 = this->mult_trans_col1(mat);
    result.col2 = this->mult_trans_col2(mat);
    result.col3 = this->mult_trans_col3(mat);
    return result;
}

inline mat_43
mat_44::mult_trans(const mat_34& mat) const
{
    mat_43 result;
    result.col0 = this->mult_trans_col0(mat);
    result.col1 = this->mult_trans_col1(mat);
    result.col2 = this->mult_trans_col2(mat);
    return result;
}

// mat_43

// explicit constructors

inline mat_43::mat_43(const mat_44& mat)
{
    col0 = mat.col0;
    col1 = mat.col1;
    col2 = mat.col2;
}

// negate

inline mat_43
mat_43::operator-() const
{
    mat_43 result;
    result.col0 = -col0;
    result.col1 = -col1;
    result.col2 = -col2;
    return result;
}

// transpose

inline mat_34
mat_43::transpose() const
{
    mat_34 result;
    vec128_t temp0, temp1, temp2, temp3;

    asm("### mat_43::transpose ### \n"
        "pextlw	%[temp0], %[col1], %[col0] \n"
        "pextuw	%[temp1], %[col1], %[col0] \n"
        "pextlw	%[temp2], $0, %[col2] \n"
        "pextuw	%[temp3], $0, %[col2] \n"
        : [temp0] "=&r"(temp0),
        [temp1] "=&r"(temp1),
        [temp2] "=&r"(temp2),
        [temp3] "=&r"(temp3)
        : [col0] "r"(col0),
        [col1] "r"(col1),
        [col2] "r"(col2));

    asm("pcpyld	%[res0], %[temp2], %[temp0] \n"
        "pcpyud	%[res1], %[temp0], %[temp2] \n"
        "pcpyld	%[res2], %[temp3], %[temp1] \n"
        "pcpyud	%[res3], %[temp1], %[temp3] \n"
        : [res0] "=&r"(result.col0),
        [res1] "=&r"(result.col1),
        [res2] "=&r"(result.col2),
        [res3] "=&r"(result.col3)
        : [temp0] "r"(temp0),
        [temp1] "r"(temp1),
        [temp2] "r"(temp2),
        [temp3] "r"(temp3));
    return result;
}

// matrix/scalar operations

inline mat_43
    mat_43::operator*(float scale) const
{
    mat_43 result;
    vec_x vec(scale);

    result.col0 = col0 * vec;
    result.col1 = col1 * vec;
    result.col2 = col2 * vec;
    return result;
}

inline mat_43
    mat_43::operator*(const vec_x scale) const
{
    mat_43 result;
    result.col0 = col0 * scale;
    result.col1 = col1 * scale;
    result.col2 = col2 * scale;
    return result;
}

inline mat_43
    mat_43::operator*(const vec_y scale) const
{
    mat_43 result;
    result.col0 = col0 * scale;
    result.col1 = col1 * scale;
    result.col2 = col2 * scale;
    return result;
}

inline mat_43
    mat_43::operator*(const vec_z scale) const
{
    mat_43 result;
    result.col0 = col0 * scale;
    result.col1 = col1 * scale;
    result.col2 = col2 * scale;
    return result;
}

inline mat_43
    mat_43::operator*(const vec_w scale) const
{
    mat_43 result;
    result.col0 = col0 * scale;
    result.col1 = col1 * scale;
    result.col2 = col2 * scale;
    return result;
}

// matrix/vector operations

inline mat_43
mat_43::mult_tilde(vec_3 vec) const
{
    mat_43 result;
    vec128_t c0 = col0.vec128, c1 = col1.vec128, c2 = col2.vec128, v = vec.vec128;
    asm __volatile__ ("### mat_43 mult_tilde vec_3 ### \n"
        "qmtc2 %[c0], $vf1  \n"
        "qmtc2 %[c1], $vf2  \n"
        "qmtc2 %[c2], $vf3  \n"
        "qmtc2 %[v],  $vf4  \n"
        "vmulaz $ACC, $vf2, $vf4 \n"
        "vmsuby $vf5, $vf3, $vf4 \n"
        "vmulax $ACC, $vf3, $vf4 \n"
        "vmsubz $vf6, $vf1, $vf4 \n"
        "vmulay $ACC, $vf1, $vf4 \n"
        "vmsubx $vf7, $vf2, $vf4 \n"
        "qmfc2 %[r0], $vf5  \n"
        "qmfc2 %[r1], $vf6  \n"
        "qmfc2 %[r2], $vf7  \n"
        : [r0] "=&r"(result.col0), [r1] "=&r"(result.col1), [r2] "=&r"(result.col2), "=r"(vu0_ACC)
        : [c0] "r"(c0), [c1] "r"(c1), [c2] "r"(c2), [v] "r"(v));
    return result;
}

// matrix/matrix operations

inline mat_43
mat_43::operator+(const mat_43& mat) const
{
    mat_43 result;
    result.col0 = col0 + mat.get_col0();
    result.col1 = col1 + mat.get_col1();
    result.col2 = col2 + mat.get_col2();
    return result;
}

inline mat_43
mat_43::operator-(const mat_43& mat) const
{
    mat_43 result;
    result.col0 = col0 - mat.get_col0();
    result.col1 = col1 - mat.get_col1();
    result.col2 = col2 - mat.get_col2();
    return result;
}

inline mat_43
    mat_43::operator*(const mat_33& mat) const
{
    mat_43 result;
    result.col0 = *this * mat.get_col0();
    result.col1 = *this * mat.get_col1();
    result.col2 = *this * mat.get_col2();
    return result;
}

inline mat_44
    mat_43::operator*(const mat_34& mat) const
{
    mat_44 result;
    result.col0 = *this * mat.get_col0();
    result.col1 = *this * mat.get_col1();
    result.col2 = *this * mat.get_col2();
    result.col3 = *this * mat.get_col3();
    return result;
}

inline mat_33
mat_43::trans_mult(const mat_43& mat) const
{
    mat_33 result;
    result.col0 = this->trans_mult(mat.get_col0());
    result.col1 = this->trans_mult(mat.get_col1());
    result.col2 = this->trans_mult(mat.get_col2());
    return result;
}

inline mat_34
mat_43::trans_mult(const mat_44& mat) const
{
    mat_34 result;
    result.col0 = this->trans_mult(mat.get_col0());
    result.col1 = this->trans_mult(mat.get_col1());
    result.col2 = this->trans_mult(mat.get_col2());
    result.col3 = this->trans_mult(mat.get_col3());
    return result;
}

inline mat_34
mat_43::trans_mult(const transform_t& mat) const
{
    mat_34 result;
    result.col0 = this->trans_mult(mat.get_col0());
    result.col1 = this->trans_mult(mat.get_col1());
    result.col2 = this->trans_mult(mat.get_col2());
    result.col3 = this->trans_mult(mat.get_col3());
    return result;
}

inline mat_44
mat_43::mult_trans(const mat_43& mat) const
{
    mat_44 result;
    result.col0 = this->mult_trans_col0(mat);
    result.col1 = this->mult_trans_col1(mat);
    result.col2 = this->mult_trans_col2(mat);
    result.col3 = this->mult_trans_col3(mat);
    return result;
}

inline mat_43
mat_43::mult_trans(const mat_33& mat) const
{
    mat_43 result;
    result.col0 = this->mult_trans_col0(mat);
    result.col1 = this->mult_trans_col1(mat);
    result.col2 = this->mult_trans_col2(mat);
    return result;
}

// mat_34

// explicit constructors

inline mat_34::mat_34(const mat_44& mat)
{
    col0 = vec_3(mat.col0);
    col1 = vec_3(mat.col1);
    col2 = vec_3(mat.col2);
    col3 = vec_3(mat.col3);
}

// negate

inline mat_34
mat_34::operator-() const
{
    mat_34 result;
    result.col0 = -col0;
    result.col1 = -col1;
    result.col2 = -col2;
    result.col3 = -col3;
    return result;
}

// transpose

inline mat_43
mat_34::transpose() const
{
    mat_43 result;
    vec128_t temp0, temp1, temp2, temp3;

    asm("### mat_34::transpose ### \n"
        "pextlw	%[temp0], %[col1], %[col0] \n"
        "pextuw	%[temp1], %[col1], %[col0] \n"
        "pextlw	%[temp2], %[col3], %[col2] \n"
        "pextuw	%[temp3], %[col3], %[col2] \n"
        : [temp0] "=&r"(temp0),
        [temp1] "=&r"(temp1),
        [temp2] "=&r"(temp2),
        [temp3] "=&r"(temp3)
        : [col0] "r"(col0),
        [col1] "r"(col1),
        [col2] "r"(col2),
        [col3] "r"(col3));

    asm("pcpyld	%[res0], %[temp2], %[temp0] \n"
        "pcpyud	%[res1], %[temp0], %[temp2] \n"
        "pcpyld	%[res2], %[temp3], %[temp1] \n"
        : [res0] "=&r"(result.col0),
        [res1] "=&r"(result.col1),
        [res2] "=&r"(result.col2)
        : [temp0] "r"(temp0),
        [temp1] "r"(temp1),
        [temp2] "r"(temp2),
        [temp3] "r"(temp3));
    return result;
}

// matrix/scalar operations

inline mat_34
    mat_34::operator*(float scale) const
{
    mat_34 result;
    vec_x vec(scale);

    result.col0 = col0 * vec;
    result.col1 = col1 * vec;
    result.col2 = col2 * vec;
    result.col3 = col3 * vec;
    return result;
}

inline mat_34
    mat_34::operator*(const vec_x scale) const
{
    mat_34 result;
    result.col0 = col0 * scale;
    result.col1 = col1 * scale;
    result.col2 = col2 * scale;
    result.col3 = col3 * scale;
    return result;
}

inline mat_34
    mat_34::operator*(const vec_y scale) const
{
    mat_34 result;
    result.col0 = col0 * scale;
    result.col1 = col1 * scale;
    result.col2 = col2 * scale;
    result.col3 = col3 * scale;
    return result;
}

inline mat_34
    mat_34::operator*(const vec_z scale) const
{
    mat_34 result;
    result.col0 = col0 * scale;
    result.col1 = col1 * scale;
    result.col2 = col2 * scale;
    result.col3 = col3 * scale;
    return result;
}

inline mat_34
    mat_34::operator*(const vec_w scale) const
{
    mat_34 result;
    result.col0 = col0 * scale;
    result.col1 = col1 * scale;
    result.col2 = col2 * scale;
    result.col3 = col3 * scale;
    return result;
}

// matrix/matrix operations

inline mat_34
mat_34::operator+(const mat_34& mat) const
{
    mat_34 result;
    result.col0 = col0 + mat.get_col0();
    result.col1 = col1 + mat.get_col1();
    result.col2 = col2 + mat.get_col2();
    result.col3 = col3 + mat.get_col3();
    return result;
}

inline mat_34
mat_34::operator-(const mat_34& mat) const
{
    mat_34 result;
    result.col0 = col0 - mat.get_col0();
    result.col1 = col1 - mat.get_col1();
    result.col2 = col2 - mat.get_col2();
    result.col3 = col3 - mat.get_col3();
    return result;
}

inline mat_33
    mat_34::operator*(const mat_43& mat) const
{
    mat_33 result;
    result.col0 = *this * mat.get_col0();
    result.col1 = *this * mat.get_col1();
    result.col2 = *this * mat.get_col2();
    return result;
}

inline mat_34
    mat_34::operator*(const mat_44& mat) const
{
    mat_34 result;
    result.col0 = *this * mat.get_col0();
    result.col1 = *this * mat.get_col1();
    result.col2 = *this * mat.get_col2();
    result.col3 = *this * mat.get_col3();
    return result;
}

inline mat_34
    mat_34::operator*(const transform_t& mat) const
{
    mat_34 result;
    result.col0 = *this * mat.get_col0();
    result.col1 = *this * mat.get_col1();
    result.col2 = *this * mat.get_col2();
    result.col3 = *this * mat.get_col3();
    return result;
}

inline mat_43
mat_34::trans_mult(const mat_33& mat) const
{
    mat_43 result;
    result.col0 = this->trans_mult(mat.get_col0());
    result.col1 = this->trans_mult(mat.get_col1());
    result.col2 = this->trans_mult(mat.get_col2());
    return result;
}

inline mat_44
mat_34::trans_mult(const mat_34& mat) const
{
    mat_44 result;
    result.col0 = this->trans_mult(mat.get_col0());
    result.col1 = this->trans_mult(mat.get_col1());
    result.col2 = this->trans_mult(mat.get_col2());
    result.col3 = this->trans_mult(mat.get_col3());
    return result;
}

inline mat_33
mat_34::mult_trans(const mat_34& mat) const
{
    mat_33 result;
    result.col0 = this->mult_trans_col0(mat);
    result.col1 = this->mult_trans_col1(mat);
    result.col2 = this->mult_trans_col2(mat);
    return result;
}

inline mat_34
mat_34::mult_trans(const mat_44& mat) const
{
    mat_34 result;
    result.col0 = this->mult_trans_col0(mat);
    result.col1 = this->mult_trans_col1(mat);
    result.col2 = this->mult_trans_col2(mat);
    result.col3 = this->mult_trans_col3(mat);
    return result;
}

// transform_t

// inverses

inline transform_t
transform_t::inverse() const
{
    transform_t result;

    asm __volatile__ ("### transform_t::inverse ### \n"
        "lqc2 $vf1, 0(%[T])                             \n"
        "lqc2 $vf2, 16(%[T])                            \n"
        "lqc2 $vf3, 32(%[T])                            \n"
        "lqc2 $vf4, 48(%[T])                            \n"
        "vopmula.xyz $ACC, $vf1, $vf2  # inv2 = col0 x col1 \n"
        "vopmsub.xyz $vf7, $vf2, $vf1                   \n"
        "vopmula.xyz $ACC, $vf2, $vf3  # inv0 = col1 x col2 \n"
        "vopmsub.xyz $vf5, $vf3, $vf2                   \n"
        "vmul        $vf9, $vf3, $vf7  # det components     \n"
        "vaddw.x     $vf8, $vf0, $vf0  # temp.x = 1.0       \n"
        "vopmula.xyz $ACC, $vf3, $vf1  # inv1 = col2 x col0 \n"
        "vopmsub.xyz $vf6, $vf1, $vf3                   \n"
        "vadday.x    $ACC, $vf9, $vf9                   \n"
        "vmaddz.x    $vf9, $vf8, $vf9  # det = det.x+det.y+det.z \n"
        "vaddx.y     $vf8, $vf0, $vf7  # transpose           \n"
        "vadd.xz     $vf8, $vf0, $vf6                   \n"
        "vaddy.x     $vf6, $vf0, $vf5                   \n"
        "vdiv        $Q, $vf0w, $vf9x   # Q = 1/det          \n"
        "vaddy.z     $vf6, $vf0, $vf7                   \n"
        "vaddz.x     $vf7, $vf0, $vf5                   \n"
        "vaddy.z     $vf5, $vf0, $vf8                   \n"
        "vaddx.y     $vf5, $vf0, $vf8                   \n"
        "vaddz.y     $vf7, $vf0, $vf8                   \n"
        "vsuba       $ACC, $vf0, $vf0  # compute -R^T*t      \n"
        "vmsubay     $ACC, $vf6, $vf4                   \n"
        "vmsubax     $ACC, $vf5, $vf4                   \n"
        "vmsubz      $vf9, $vf7, $vf4                   \n"
        "vwaitq                                         \n"
        "vmulq.xyz   $vf5, $vf5, $Q     # scale by 1/det     \n"
        "vmulq.xyz   $vf6, $vf6, $Q                      \n"
        "vmulq.xyz   $vf7, $vf7, $Q                      \n"
        "vmulq.xyz   $vf9, $vf9, $Q                      \n"
        "sqc2 $vf5, 0(%[R])                              \n"
        "sqc2 $vf6, 16(%[R])                             \n"
        "sqc2 $vf7, 32(%[R])                             \n"
        "sqc2 $vf9, 48(%[R])                             \n"
        : "=r"(vu0_ACC)
        : [T] "r"(this), [R] "r"(&result)
        : "memory");
    return result;
}

inline transform_t
transform_t::orthonormal_inverse() const
{
    transform_t result;
    asm __volatile__ ("### transform_t::orthonormal_inverse ### \n"
        "lqc2 $vf1, 0(%[T])    \n"
        "lqc2 $vf2, 16(%[T])   \n"
        "lqc2 $vf3, 32(%[T])   \n"
        "lqc2 $vf4, 48(%[T])   \n"
        "vadd.x    $vf5, $vf0, $vf1  \n"
        "vadd.y    $vf6, $vf0, $vf2  \n"
        "vadd.z    $vf7, $vf0, $vf3  \n"
        "vaddx.y   $vf5, $vf0, $vf2  \n"
        "vaddy.x   $vf6, $vf0, $vf1  \n"
        "vaddz.x   $vf7, $vf0, $vf1  \n"
        "vaddx.z   $vf5, $vf0, $vf3  \n"
        "vaddy.z   $vf6, $vf0, $vf3  \n"
        "vaddz.y   $vf7, $vf0, $vf2  \n"
        "vsuba     $ACC, $vf0, $vf0  \n"
        "vmsubax   $ACC, $vf5, $vf4  \n"
        "vmsubay   $ACC, $vf6, $vf4  \n"
        "vmsubz    $vf8, $vf7, $vf4  \n"
        "sqc2 $vf5, 0(%[R])   \n"
        "sqc2 $vf6, 16(%[R])  \n"
        "sqc2 $vf7, 32(%[R])  \n"
        "sqc2 $vf8, 48(%[R])  \n"
        : "=r"(vu0_ACC)
        : [T] "r"(this), [R] "r"(&result)
        : "memory");
    return result;
}

// matrix/matrix operations

inline transform_t
    transform_t::operator*(const transform_t& mat) const
{
    transform_t result;
    result.col0 = *this * mat.get_col0();
    result.col1 = *this * mat.get_col1();
    result.col2 = *this * mat.get_col2();
    result.col3 = *this * mat.get_col3();
    return result;
}

inline mat_44
    transform_t::operator*(const mat_44& mat) const
{
    mat_44 result;
    result.col0 = *this * mat.get_col0();
    result.col1 = *this * mat.get_col1();
    result.col2 = *this * mat.get_col2();
    result.col3 = *this * mat.get_col3();
    return result;
}

inline mat_43
    transform_t::operator*(const mat_43& mat) const
{
    mat_43 result;
    result.col0 = *this * mat.get_col0();
    result.col1 = *this * mat.get_col1();
    result.col2 = *this * mat.get_col2();
    return result;
}

#endif // matrix_h
