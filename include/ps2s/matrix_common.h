/*	  Copyright (C) 2000,2001,2002  Sony Computer Entertainment America

       	  This file is subject to the terms and conditions of the GNU Lesser
	  General Public License Version 2.1. See the file "COPYING" in the
	  main directory of this archive for more details.                             */

#ifndef matrix_common_h
#define matrix_common_h

#include "ps2s/vector.h"

// mat_x3_template - template for 3 column matrices

template <class column_type>
class mat_x3_template {
public:
    column_type col0, col1, col2;

    mat_x3_template() {}

    mat_x3_template(const mat_x3_template& mat)
    {
        col0 = mat.col0;
        col1 = mat.col1;
        col2 = mat.col2;
    }

    mat_x3_template(const column_type col_0, const column_type col_1, const column_type col_2)
    {
        col0 = col_0;
        col1 = col_1;
        col2 = col_2;
    }

    void set_zero()
    {
        asm __volatile__(
            "vsub       $vf1, $vf0, $vf0         \n"
            "vsub       $vf2, $vf0, $vf0         \n"
            "vsub       $vf3, $vf0, $vf0         \n"
            "qmfc2      %[col0], $vf1           \n"
            "qmfc2      %[col1], $vf2            \n"
            "qmfc2      %[col2], $vf3            \n"
            : [col0] "=r"(col0.vec128), [col1] "=r"(col1.vec128), [col2] "=r"(col2.vec128)
            :
            : "memory");
    }

    void set_col0(column_type new_col) { col0 = new_col; }
    void set_col1(column_type new_col) { col1 = new_col; }
    void set_col2(column_type new_col) { col2 = new_col; }

    void set_row0(vec_3 new_row)
    {
        vec128_t new_row_val = new_row.vec128;
        asm __volatile__(
            "qmtc2      %[col0], $vf1            \n"
            "qmtc2      %[col1], $vf2            \n"
            "qmtc2      %[col2], $vf3            \n"
            "qmtc2      %[new_row_val], $vf4    \n"
            "vaddx.x    $vf1, $vf0, $vf4         \n"
            "vaddy.x    $vf2, $vf0, $vf4         \n"
            "vaddz.x    $vf3, $vf0, $vf4         \n"
            "qmfc2      %[col0], $vf1            \n"
            "qmfc2      %[col1], $vf2            \n"
            "qmfc2      %[col2], $vf3            \n"
            : [col0] "+r"(col0.vec128), [col1] "+r"(col1.vec128), [col2] "+r"(col2.vec128)
            : [new_row_val] "r"(new_row_val)
            : "memory");
    }

    void set_row1(vec_3 new_row)
    {
        vec128_t new_row_val = new_row.vec128;
        asm __volatile__(
            "qmtc2      %[col0], $vf1            \n"
            "qmtc2      %[col1], $vf2            \n"
            "qmtc2      %[col2], $vf3            \n"
            "qmtc2      %[new_row_val], $vf4    \n"
            "vaddx.y    $vf1, $vf0, $vf4         \n"
            "vaddy.y    $vf2, $vf0, $vf4         \n"
            "vaddz.y    $vf3, $vf0, $vf4         \n"
            "qmfc2      %[col0], $vf1            \n"
            "qmfc2      %[col1], $vf2            \n"
            "qmfc2      %[col2], $vf3            \n"
            : [col0] "+r"(col0.vec128), [col1] "+r"(col1.vec128), [col2] "+r"(col2.vec128)
            : [new_row_val] "r"(new_row_val)
            : "memory");
    }

    void set_row2(vec_3 new_row)
    {
        vec128_t new_row_val = new_row.vec128;
        asm __volatile__(
            "qmtc2      %[col0], $vf1            \n"
            "qmtc2      %[col1], $vf2            \n"
            "qmtc2      %[col2], $vf3            \n"
            "qmtc2      %[new_row_val], $vf4    \n"
            "vaddx.z    $vf1, $vf0, $vf4         \n"
            "vaddy.z    $vf2, $vf0, $vf4         \n"
            "vaddz.z    $vf3, $vf0, $vf4         \n"
            "qmfc2      %[col0], $vf1            \n"
            "qmfc2      %[col1], $vf2            \n"
            "qmfc2      %[col2], $vf3            \n"
            : [col0] "+r"(col0.vec128), [col1] "+r"(col1.vec128), [col2] "+r"(col2.vec128)
            : [new_row_val] "r"(new_row_val)
            : "memory");
    }

    void set_row3(vec_3 new_row)
    {
        vec128_t new_row_val = new_row.vec128;
        asm __volatile__(
            "qmtc2      %[col0], $vf1            \n"
            "qmtc2      %[col1], $vf2            \n"
            "qmtc2      %[col2], $vf3            \n"
            "qmtc2      %[new_row_val], $vf4    \n"
            "vmulx.w    $vf1, $vf0, $vf4         \n"
            "vmuly.w    $vf2, $vf0, $vf4         \n"
            "vmulz.w    $vf3, $vf0, $vf4         \n"
            "qmfc2      %[col0], $vf1            \n"
            "qmfc2      %[col1], $vf2            \n"
            "qmfc2      %[col2], $vf3            \n"
            : [col0] "+r"(col0.vec128), [col1] "+r"(col1.vec128), [col2] "+r"(col2.vec128)
            : [new_row_val] "r"(new_row_val)
            : "memory");
    }

    column_type get_col0() const { return col0; }
    column_type get_col1() const { return col1; }
    column_type get_col2() const { return col2; }

    vec_3 get_row0() const
    {
        vec128_t row;
        vec128_t col0_val = col0.vec128;
        vec128_t col1_val = col1.vec128;
        vec128_t col2_val = col2.vec128;
        asm __volatile__(
            "qmtc2      %[col0_val], $vf1        \n"
            "qmtc2      %[col1_val], $vf2        \n"
            "qmtc2      %[col2_val], $vf3        \n"
            "vsub       $vf4, $vf0, $vf0         \n"
            "vaddx.x $vf4, $vf0, $vf1        \n"
            "vaddx.y $vf4, $vf0, $vf2        \n"
            "vaddx.z $vf4, $vf0, $vf3        \n"
            "qmfc2      %[row], $vf4             \n"
            : [row] "=&r"(row)
            : [col0_val] "r"(col0_val), [col1_val] "r"(col1_val), [col2_val] "r"(col2_val)
            : "memory");
        return vec_3(row);
    }

    vec_3 get_row1() const
    {
        vec128_t row;
        vec128_t col0_val = col0.vec128;
        vec128_t col1_val = col1.vec128;
        vec128_t col2_val = col2.vec128;
        asm __volatile__(
            "qmtc2      %[col0_val], $vf1        \n"
            "qmtc2      %[col1_val], $vf2        \n"
            "qmtc2      %[col2_val], $vf3        \n"
            "vsub       $vf4, $vf0, $vf0         \n"
            "vaddy.x $vf4, $vf0, $vf1        \n"
            "vaddy.y $vf4, $vf0, $vf2        \n"
            "vaddy.z $vf4, $vf0, $vf3        \n"
            "qmfc2      %[row], $vf4             \n"
            : [row] "=&r"(row)
            : [col0_val] "r"(col0_val), [col1_val] "r"(col1_val), [col2_val] "r"(col2_val)
            : "memory");
        return vec_3(row);
    }

    vec_3 get_row2() const
    {
        vec128_t row;
        vec128_t col0_val = col0.vec128;
        vec128_t col1_val = col1.vec128;
        vec128_t col2_val = col2.vec128;
        asm __volatile__(
            "qmtc2      %[col0_val], $vf1        \n"
            "qmtc2      %[col1_val], $vf2        \n"
            "qmtc2      %[col2_val], $vf3        \n"
            "vsub       $vf4, $vf0, $vf0         \n"
            "vaddz.x $vf4, $vf0, $vf1        \n"
            "vaddz.y $vf4, $vf0, $vf2        \n"
            "vaddz.z $vf4, $vf0, $vf3        \n"
            "qmfc2      %[row], $vf4             \n"
            : [row] "=&r"(row)
            : [col0_val] "r"(col0_val), [col1_val] "r"(col1_val), [col2_val] "r"(col2_val)
            : "memory");
        return vec_3(row);
    }

    vec_3 get_row3() const
    {
        vec128_t row;
        vec128_t col0_val = col0.vec128;
        vec128_t col1_val = col1.vec128;
        vec128_t col2_val = col2.vec128;
        asm __volatile__(
            "qmtc2      %[col0_val], $vf1        \n"
            "qmtc2      %[col1_val], $vf2        \n"
            "qmtc2      %[col2_val], $vf3        \n"
            "vsub       $vf4, $vf0, $vf0         \n"
            "vaddw.x $vf4, $vf0, $vf1        \n"
            "vaddw.y $vf4, $vf0, $vf2        \n"
            "vaddw.z $vf4, $vf0, $vf3        \n"
            "qmfc2      %[row], $vf4             \n"
            : [row] "=&r"(row)
            : [col0_val] "r"(col0_val), [col1_val] "r"(col1_val), [col2_val] "r"(col2_val)
            : "memory");
        return vec_3(row);
    }

    column_type
    operator*(vec_3 vec) const
    {
        vec128_t result;
        vec128_t vec_val = vec.vec128;
        vec128_t col0_val = col0.vec128;
        vec128_t col1_val = col1.vec128;
        vec128_t col2_val = col2.vec128;
        asm __volatile__(
            "qmtc2      %[col0_val], $vf1        \n"
            "qmtc2      %[col1_val], $vf2        \n"
            "qmtc2      %[col2_val], $vf3        \n"
            "qmtc2      %[vec_val], $vf4         \n"
            "vsuba      $ACC, $vf0, $vf0         \n"
            "vmulax     $ACC, $vf1, $vf4         \n"
            "vmadday    $ACC, $vf2, $vf4         \n"
            "vmaddz     $vf5, $vf3, $vf4         \n"
            "qmfc2      %[result], $vf5          \n"
            : [result] "=&r"(result), [acc] "=r"(vu0_ACC)
            : [vec_val] "r"(vec_val), [col0_val] "r"(col0_val), [col1_val] "r"(col1_val), [col2_val] "r"(col2_val)
            : "memory");
        return column_type(result);
    }

    template <class mat_column_type>
    column_type
    mult_trans_col0(const mat_x3_template<mat_column_type>& mat) const
    {
        vec128_t result;
        vec128_t col0_val = col0.vec128;
        vec128_t col1_val = col1.vec128;
        vec128_t col2_val = col2.vec128;
        vec128_t mat0_val = mat.col0.vec128;
        vec128_t mat1_val = mat.col1.vec128;
        vec128_t mat2_val = mat.col2.vec128;
        asm __volatile__(
            "qmtc2      %[col0_val], $vf1        \n"
            "qmtc2      %[col1_val], $vf2        \n"
            "qmtc2      %[col2_val], $vf3        \n"
            "qmtc2      %[mat0_val], $vf4        \n"
            "qmtc2      %[mat1_val], $vf5        \n"
            "qmtc2      %[mat2_val], $vf6        \n"
            "vsuba      $ACC, $vf0, $vf0         \n"
            "vmulax     $ACC, $vf1, $vf4         \n"
            "vmaddax    $ACC, $vf2, $vf5         \n"
            "vmaddx     $vf7, $vf3, $vf6         \n"
            "qmfc2      %[result], $vf7          \n"
            : [result] "=&r"(result), [acc] "=r"(vu0_ACC)
            : [col0_val] "r"(col0_val), [col1_val] "r"(col1_val), [col2_val] "r"(col2_val),
              [mat0_val] "r"(mat0_val), [mat1_val] "r"(mat1_val), [mat2_val] "r"(mat2_val)
            : "memory");
        return column_type(result);
    }

    template <class mat_column_type>
    column_type
    mult_trans_col1(const mat_x3_template<mat_column_type>& mat) const
    {
        vec128_t result;
        vec128_t col0_val = col0.vec128;
        vec128_t col1_val = col1.vec128;
        vec128_t col2_val = col2.vec128;
        vec128_t mat0_val = mat.col0.vec128;
        vec128_t mat1_val = mat.col1.vec128;
        vec128_t mat2_val = mat.col2.vec128;
        asm __volatile__(
            "qmtc2      %[col0_val], $vf1        \n"
            "qmtc2      %[col1_val], $vf2        \n"
            "qmtc2      %[col2_val], $vf3        \n"
            "qmtc2      %[mat0_val], $vf4        \n"
            "qmtc2      %[mat1_val], $vf5        \n"
            "qmtc2      %[mat2_val], $vf6        \n"
            "vsuba      $ACC, $vf0, $vf0         \n"
            "vmulay     $ACC, $vf1, $vf4         \n"
            "vmadday    $ACC, $vf2, $vf5         \n"
            "vmaddy     $vf7, $vf3, $vf6         \n"
            "qmfc2      %[result], $vf7          \n"
            : [result] "=&r"(result), [acc] "=r"(vu0_ACC)
            : [col0_val] "r"(col0_val), [col1_val] "r"(col1_val), [col2_val] "r"(col2_val),
              [mat0_val] "r"(mat0_val), [mat1_val] "r"(mat1_val), [mat2_val] "r"(mat2_val)
            : "memory");
        return column_type(result);
    }

    template <class mat_column_type>
    column_type
    mult_trans_col2(const mat_x3_template<mat_column_type>& mat) const
    {
        vec128_t result;
        vec128_t col0_val = col0.vec128;
        vec128_t col1_val = col1.vec128;
        vec128_t col2_val = col2.vec128;
        vec128_t mat0_val = mat.col0.vec128;
        vec128_t mat1_val = mat.col1.vec128;
        vec128_t mat2_val = mat.col2.vec128;
        asm __volatile__(
            "qmtc2      %[col0_val], $vf1        \n"
            "qmtc2      %[col1_val], $vf2        \n"
            "qmtc2      %[col2_val], $vf3        \n"
            "qmtc2      %[mat0_val], $vf4        \n"
            "qmtc2      %[mat1_val], $vf5        \n"
            "qmtc2      %[mat2_val], $vf6        \n"
            "vsuba      $ACC, $vf0, $vf0         \n"
            "vmulaz     $ACC, $vf1, $vf4         \n"
            "vmaddaz    $ACC, $vf2, $vf5         \n"
            "vmaddz     $vf7, $vf3, $vf6         \n"
            "qmfc2      %[result], $vf7          \n"
            : [result] "=&r"(result), [acc] "=r"(vu0_ACC)
            : [col0_val] "r"(col0_val), [col1_val] "r"(col1_val), [col2_val] "r"(col2_val),
              [mat0_val] "r"(mat0_val), [mat1_val] "r"(mat1_val), [mat2_val] "r"(mat2_val)
            : "memory");
        return column_type(result);
    }

    template <class mat_column_type>
    column_type
    mult_trans_col3(const mat_x3_template<mat_column_type>& mat) const
    {
        vec128_t result;
        vec128_t col0_val = col0.vec128;
        vec128_t col1_val = col1.vec128;
        vec128_t col2_val = col2.vec128;
        vec128_t mat0_val = mat.col0.vec128;
        vec128_t mat1_val = mat.col1.vec128;
        vec128_t mat2_val = mat.col2.vec128;
        asm __volatile__(
            "qmtc2      %[col0_val], $vf1        \n"
            "qmtc2      %[col1_val], $vf2        \n"
            "qmtc2      %[col2_val], $vf3        \n"
            "qmtc2      %[mat0_val], $vf4        \n"
            "qmtc2      %[mat1_val], $vf5        \n"
            "qmtc2      %[mat2_val], $vf6        \n"
            "vsuba      $ACC, $vf0, $vf0         \n"
            "vmulaw     $ACC, $vf1, $vf4         \n"
            "vmaddaw    $ACC, $vf2, $vf5         \n"
            "vmaddw     $vf7, $vf3, $vf6         \n"
            "qmfc2      %[result], $vf7          \n"
            : [result] "=&r"(result), [acc] "=r"(vu0_ACC)
            : [col0_val] "r"(col0_val), [col1_val] "r"(col1_val), [col2_val] "r"(col2_val),
              [mat0_val] "r"(mat0_val), [mat1_val] "r"(mat1_val), [mat2_val] "r"(mat2_val)
            : "memory");
        return column_type(result);
    }
};

// mat_x4_template - template for 4 column matrices

template <class column_type>
class mat_x4_template {
public:
    column_type col0, col1, col2, col3;

    mat_x4_template() {}

    mat_x4_template(const mat_x4_template& mat)
    {
        col0 = mat.col0;
        col1 = mat.col1;
        col2 = mat.col2;
        col3 = mat.col3;
    }

    mat_x4_template(const column_type col_0, const column_type col_1,
        const column_type col_2, const column_type col_3)
    {
        col0 = col_0;
        col1 = col_1;
        col2 = col_2;
        col3 = col_3;
    }

    void set_zero()
    {
        asm __volatile__(
            "vsub       $vf1, $vf0, $vf0         \n"
            "vsub       $vf2, $vf0, $vf0         \n"
            "vsub       $vf3, $vf0, $vf0         \n"
            "vsub       $vf4, $vf0, $vf0         \n"
            "qmfc2      %[col0], $vf1            \n"
            "qmfc2      %[col1], $vf2            \n"
            "qmfc2      %[col2], $vf3            \n"
            "qmfc2      %[col3], $vf4            \n"
            : [col0] "=r"(col0.vec128), [col1] "=r"(col1.vec128), [col2] "=r"(col2.vec128), [col3] "=r"(col3.vec128)
            :
            : "memory");
    }

    void set_col0(column_type new_col) { col0 = new_col; }
    void set_col1(column_type new_col) { col1 = new_col; }
    void set_col2(column_type new_col) { col2 = new_col; }
    void set_col3(column_type new_col) { col3 = new_col; }

    void set_row0(vec_4 new_row)
    {
        vec128_t new_row_val = new_row.vec128;
        asm __volatile__(
            "qmtc2      %[col0], $vf1            \n"
            "qmtc2      %[col1], $vf2            \n"
            "qmtc2      %[col2], $vf3            \n"
            "qmtc2      %[col3], $vf4            \n"
            "qmtc2      %[new_row_val], $vf5     \n"
            "vaddx.x    $vf1, $vf0, $vf5         \n"
            "vaddy.x    $vf2, $vf0, $vf5         \n"
            "vaddz.x    $vf3, $vf0, $vf5         \n"
            "vaddw.x    $vf4, $vf0, $vf5         \n"
            "qmfc2      %[col0], $vf1            \n"
            "qmfc2      %[col1], $vf2            \n"
            "qmfc2      %[col2], $vf3            \n"
            "qmfc2      %[col3], $vf4            \n"
            : [col0] "+r"(col0.vec128), [col1] "+r"(col1.vec128), [col2] "+r"(col2.vec128), [col3] "+r"(col3.vec128)
            : [new_row_val] "r"(new_row_val)
            : "memory");
    }

    void set_row1(vec_4 new_row)
    {
        vec128_t new_row_val = new_row.vec128;
        asm __volatile__(
            "qmtc2      %[col0], $vf1            \n"
            "qmtc2      %[col1], $vf2            \n"
            "qmtc2      %[col2], $vf3            \n"
            "qmtc2      %[col3], $vf4            \n"
            "qmtc2      %[new_row_val], $vf5     \n"
            "vaddx.y    $vf1, $vf0, $vf5         \n"
            "vaddy.y    $vf2, $vf0, $vf5         \n"
            "vaddz.y    $vf3, $vf0, $vf5         \n"
            "vaddw.y    $vf4, $vf0, $vf5         \n"
            "qmfc2      %[col0], $vf1            \n"
            "qmfc2      %[col1], $vf2            \n"
            "qmfc2      %[col2], $vf3            \n"
            "qmfc2      %[col3], $vf4            \n"
            : [col0] "+r"(col0.vec128), [col1] "+r"(col1.vec128), [col2] "+r"(col2.vec128), [col3] "+r"(col3.vec128)
            : [new_row_val] "r"(new_row_val)
            : "memory");
    }

    void set_row2(vec_4 new_row)
    {
        vec128_t new_row_val = new_row.vec128;
        asm __volatile__(
            "qmtc2      %[col0], $vf1            \n"
            "qmtc2      %[col1], $vf2            \n"
            "qmtc2      %[col2], $vf3            \n"
            "qmtc2      %[col3], $vf4            \n"
            "qmtc2      %[new_row_val], $vf5     \n"
            "vaddx.z    $vf1, $vf0, $vf5         \n"
            "vaddy.z    $vf2, $vf0, $vf5         \n"
            "vaddz.z    $vf3, $vf0, $vf5         \n"
            "vaddw.z    $vf4, $vf0, $vf5         \n"
            "qmfc2      %[col0], $vf1            \n"
            "qmfc2      %[col1], $vf2            \n"
            "qmfc2      %[col2], $vf3            \n"
            "qmfc2      %[col3], $vf4            \n"
            : [col0] "+r"(col0.vec128), [col1] "+r"(col1.vec128), [col2] "+r"(col2.vec128), [col3] "+r"(col3.vec128)
            : [new_row_val] "r"(new_row_val)
            : "memory");
    }

    void set_row3(vec_4 new_row)
    {
        vec128_t new_row_val = new_row.vec128;
        asm __volatile__(
            "qmtc2      %[col0], $vf1            \n"
            "qmtc2      %[col1], $vf2            \n"
            "qmtc2      %[col2], $vf3            \n"
            "qmtc2      %[col3], $vf4            \n"
            "qmtc2      %[new_row_val], $vf5     \n"
            "vmulx.w    $vf1, $vf0, $vf5         \n"
            "vmuly.w    $vf2, $vf0, $vf5         \n"
            "vmulz.w    $vf3, $vf0, $vf5         \n"
            "vmulw.w    $vf4, $vf0, $vf5         \n"
            "qmfc2      %[col0], $vf1            \n"
            "qmfc2      %[col1], $vf2            \n"
            "qmfc2      %[col2], $vf3            \n"
            "qmfc2      %[col3], $vf4            \n"
            : [col0] "+r"(col0.vec128), [col1] "+r"(col1.vec128), [col2] "+r"(col2.vec128), [col3] "+r"(col3.vec128)
            : [new_row_val] "r"(new_row_val)
            : "memory");
    }

    column_type get_col0() const { return col0; }
    column_type get_col1() const { return col1; }
    column_type get_col2() const { return col2; }
    column_type get_col3() const { return col3; }

    vec_4 get_row0() const
    {
        vec128_t row;
        vec128_t col0_val = col0.vec128;
        vec128_t col1_val = col1.vec128;
        vec128_t col2_val = col2.vec128;
        vec128_t col3_val = col3.vec128;
        asm __volatile__(
            "qmtc2      %[col0_val], $vf1        \n"
            "qmtc2      %[col1_val], $vf2        \n"
            "qmtc2      %[col2_val], $vf3        \n"
            "qmtc2      %[col3_val], $vf4        \n"
            "vsub       $vf5, $vf0, $vf0         \n"
            "vaddx.x $vf5, $vf0, $vf1        \n"
            "vaddx.y $vf5, $vf0, $vf2        \n"
            "vaddx.z $vf5, $vf0, $vf3        \n"
            "vmulx.w $vf5, $vf0, $vf4        \n"
            "qmfc2      %[row], $vf5             \n"
            : [row] "=&r"(row)
            : [col0_val] "r"(col0_val), [col1_val] "r"(col1_val), [col2_val] "r"(col2_val), [col3_val] "r"(col3_val)
            : "memory");
        return vec_4(row);
    }

    vec_4 get_row1() const
    {
        vec128_t row;
        vec128_t col0_val = col0.vec128;
        vec128_t col1_val = col1.vec128;
        vec128_t col2_val = col2.vec128;
        vec128_t col3_val = col3.vec128;
        asm __volatile__(
            "qmtc2      %[col0_val], $vf1        \n"
            "qmtc2      %[col1_val], $vf2        \n"
            "qmtc2      %[col2_val], $vf3        \n"
            "qmtc2      %[col3_val], $vf4        \n"
            "vsub       $vf5, $vf0, $vf0         \n"
            "vaddy.x $vf5, $vf0, $vf1        \n"
            "vaddy.y $vf5, $vf0, $vf2        \n"
            "vaddy.z $vf5, $vf0, $vf3        \n"
            "vmuly.w $vf5, $vf0, $vf4        \n"
            "qmfc2      %[row], $vf5             \n"
            : [row] "=&r"(row)
            : [col0_val] "r"(col0_val), [col1_val] "r"(col1_val), [col2_val] "r"(col2_val), [col3_val] "r"(col3_val)
            : "memory");
        return vec_4(row);
    }

    vec_4 get_row2() const
    {
        vec128_t row;
        vec128_t col0_val = col0.vec128;
        vec128_t col1_val = col1.vec128;
        vec128_t col2_val = col2.vec128;
        vec128_t col3_val = col3.vec128;
        asm __volatile__(
            "qmtc2      %[col0_val], $vf1        \n"
            "qmtc2      %[col1_val], $vf2        \n"
            "qmtc2      %[col2_val], $vf3        \n"
            "qmtc2      %[col3_val], $vf4        \n"
            "vsub       $vf5, $vf0, $vf0         \n"
            "vaddz.x $vf5, $vf0, $vf1        \n"
            "vaddz.y $vf5, $vf0, $vf2        \n"
            "vaddz.z $vf5, $vf0, $vf3        \n"
            "vmulz.w $vf5, $vf0, $vf4        \n"
            "qmfc2      %[row], $vf5             \n"
            : [row] "=&r"(row)
            : [col0_val] "r"(col0_val), [col1_val] "r"(col1_val), [col2_val] "r"(col2_val), [col3_val] "r"(col3_val)
            : "memory");
        return vec_4(row);
    }

    vec_4 get_row3() const
    {
        vec128_t row;
        vec128_t col0_val = col0.vec128;
        vec128_t col1_val = col1.vec128;
        vec128_t col2_val = col2.vec128;
        vec128_t col3_val = col3.vec128;
        asm __volatile__(
            "qmtc2      %[col0_val], $vf1        \n"
            "qmtc2      %[col1_val], $vf2        \n"
            "qmtc2      %[col2_val], $vf3        \n"
            "qmtc2      %[col3_val], $vf4        \n"
            "vsub       $vf5, $vf0, $vf0         \n"
            "vaddw.x $vf5, $vf0, $vf1        \n"
            "vaddw.y $vf5, $vf0, $vf2        \n"
            "vaddw.z $vf5, $vf0, $vf3        \n"
            "vmulw.w    $vf5, $vf0, $vf4        \n"
            "qmfc2      %[row], $vf5             \n"
            : [row] "=&r"(row)
            : [col0_val] "r"(col0_val), [col1_val] "r"(col1_val), [col2_val] "r"(col2_val), [col3_val] "r"(col3_val)
            : "memory");
        return vec_4(row);
    }

    column_type
    operator*(vec_4 vec) const
    {
        vec128_t result;
        vec128_t vec_val = vec.vec128;
        vec128_t col0_val = col0.vec128;
        vec128_t col1_val = col1.vec128;
        vec128_t col2_val = col2.vec128;
        vec128_t col3_val = col3.vec128;
        asm __volatile__(
            "qmtc2      %[col0_val], $vf1        \n"
            "qmtc2      %[col1_val], $vf2        \n"
            "qmtc2      %[col2_val], $vf3        \n"
            "qmtc2      %[col3_val], $vf4        \n"
            "qmtc2      %[vec_val], $vf5         \n"
            "vsuba      $ACC, $vf0, $vf0         \n"
            "vmulax     $ACC, $vf1, $vf5         \n"
            "vmadday    $ACC, $vf2, $vf5         \n"
            "vmaddaz    $ACC, $vf3, $vf5         \n"
            "vmaddw     $vf6, $vf4, $vf5         \n"
            "qmfc2      %[result], $vf6          \n"
            : [result] "=&r"(result), [acc] "=r"(vu0_ACC)
            : [vec_val] "r"(vec_val), [col0_val] "r"(col0_val), [col1_val] "r"(col1_val), [col2_val] "r"(col2_val), [col3_val] "r"(col3_val)
            : "memory");
        return column_type(result);
    }

    column_type
    operator*(vector_t vec) const
    {
        vec128_t result;
        vec128_t vec_val = vec.vec128;
        vec128_t col0_val = col0.vec128;
        vec128_t col1_val = col1.vec128;
        vec128_t col2_val = col2.vec128;
        asm __volatile__(
            "qmtc2      %[col0_val], $vf1        \n"
            "qmtc2      %[col1_val], $vf2        \n"
            "qmtc2      %[col2_val], $vf3        \n"
            "qmtc2      %[vec_val], $vf4         \n"
            "vsuba      $ACC, $vf0, $vf0         \n"
            "vmulax     $ACC, $vf1, $vf4         \n"
            "vmadday    $ACC, $vf2, $vf4         \n"
            "vmaddz     $vf5, $vf3, $vf4         \n"
            "qmfc2      %[result], $vf5          \n"
            : [result] "=&r"(result), [acc] "=r"(vu0_ACC)
            : [vec_val] "r"(vec_val), [col0_val] "r"(col0_val), [col1_val] "r"(col1_val), [col2_val] "r"(col2_val)
            : "memory");
        return column_type(result);
    }

    column_type
    operator*(point_t pt) const
    {
        vec128_t result;
        vec128_t pt_val = pt.vec128;
        vec128_t col0_val = col0.vec128;
        vec128_t col1_val = col1.vec128;
        vec128_t col2_val = col2.vec128;
        vec128_t col3_val = col3.vec128;
        asm __volatile__(
            "qmtc2      %[col0_val], $vf1        \n"
            "qmtc2      %[col1_val], $vf2        \n"
            "qmtc2      %[col2_val], $vf3        \n"
            "qmtc2      %[col3_val], $vf4        \n"
            "qmtc2      %[pt_val], $vf5          \n"
            "vsuba      $ACC, $vf0, $vf0         \n"
            "vmulax     $ACC, $vf1, $vf5         \n"
            "vmadday    $ACC, $vf2, $vf5         \n"
            "vmaddaz    $ACC, $vf3, $vf5         \n"
            "vmaddw     $vf6, $vf4, $vf0         \n"
            "qmfc2      %[result], $vf6          \n"
            : [result] "=&r"(result), [acc] "=r"(vu0_ACC)
            : [pt_val] "r"(pt_val), [col0_val] "r"(col0_val), [col1_val] "r"(col1_val), [col2_val] "r"(col2_val), [col3_val] "r"(col3_val)
            : "memory");
        return column_type(result);
    }

    template <class mat_column_type>
    column_type
    mult_trans_col0(const mat_x4_template<mat_column_type>& mat) const
    {
        vec128_t result;
        vec128_t col0_val = col0.vec128;
        vec128_t col1_val = col1.vec128;
        vec128_t col2_val = col2.vec128;
        vec128_t col3_val = col3.vec128;
        vec128_t mat0_val = mat.col0.vec128;
        vec128_t mat1_val = mat.col1.vec128;
        vec128_t mat2_val = mat.col2.vec128;
        vec128_t mat3_val = mat.col3.vec128;
        asm __volatile__(
            "qmtc2      %[col0_val], $vf1        \n"
            "qmtc2      %[col1_val], $vf2        \n"
            "qmtc2      %[col2_val], $vf3        \n"
            "qmtc2      %[col3_val], $vf4        \n"
            "qmtc2      %[mat0_val], $vf5        \n"
            "qmtc2      %[mat1_val], $vf6        \n"
            "qmtc2      %[mat2_val], $vf7        \n"
            "qmtc2      %[mat3_val], $vf8        \n"
            "vsuba      $ACC, $vf0, $vf0         \n"
            "vmulax     $ACC, $vf1, $vf5         \n"
            "vmaddax    $ACC, $vf2, $vf6         \n"
            "vmaddax    $ACC, $vf3, $vf7         \n"
            "vmaddx     $vf9, $vf4, $vf8         \n"
            "qmfc2      %[result], $vf9          \n"
            : [result] "=&r"(result), [acc] "=r"(vu0_ACC)
            : [col0_val] "r"(col0_val), [col1_val] "r"(col1_val), [col2_val] "r"(col2_val), [col3_val] "r"(col3_val),
              [mat0_val] "r"(mat0_val), [mat1_val] "r"(mat1_val), [mat2_val] "r"(mat2_val), [mat3_val] "r"(mat3_val)
            : "memory");
        return column_type(result);
    }

    template <class mat_column_type>
    column_type
    mult_trans_col1(const mat_x4_template<mat_column_type>& mat) const
    {
        vec128_t result;
        vec128_t col0_val = col0.vec128;
        vec128_t col1_val = col1.vec128;
        vec128_t col2_val = col2.vec128;
        vec128_t col3_val = col3.vec128;
        vec128_t mat0_val = mat.col0.vec128;
        vec128_t mat1_val = mat.col1.vec128;
        vec128_t mat2_val = mat.col2.vec128;
        vec128_t mat3_val = mat.col3.vec128;
        asm __volatile__(
            "qmtc2      %[col0_val], $vf1        \n"
            "qmtc2      %[col1_val], $vf2        \n"
            "qmtc2      %[col2_val], $vf3        \n"
            "qmtc2      %[col3_val], $vf4        \n"
            "qmtc2      %[mat0_val], $vf5        \n"
            "qmtc2      %[mat1_val], $vf6        \n"
            "qmtc2      %[mat2_val], $vf7        \n"
            "qmtc2      %[mat3_val], $vf8        \n"
            "vsuba      $ACC, $vf0, $vf0         \n"
            "vmulay     $ACC, $vf1, $vf5         \n"
            "vmadday    $ACC, $vf2, $vf6         \n"
            "vmadday    $ACC, $vf3, $vf7         \n"
            "vmaddy     $vf9, $vf4, $vf8         \n"
            "qmfc2      %[result], $vf9          \n"
            : [result] "=&r"(result), [acc] "=r"(vu0_ACC)
            : [col0_val] "r"(col0_val), [col1_val] "r"(col1_val), [col2_val] "r"(col2_val), [col3_val] "r"(col3_val),
              [mat0_val] "r"(mat0_val), [mat1_val] "r"(mat1_val), [mat2_val] "r"(mat2_val), [mat3_val] "r"(mat3_val)
            : "memory");
        return column_type(result);
    }

    template <class mat_column_type>
    column_type
    mult_trans_col2(const mat_x4_template<mat_column_type>& mat) const
    {
        vec128_t result;
        vec128_t col0_val = col0.vec128;
        vec128_t col1_val = col1.vec128;
        vec128_t col2_val = col2.vec128;
        vec128_t col3_val = col3.vec128;
        vec128_t mat0_val = mat.col0.vec128;
        vec128_t mat1_val = mat.col1.vec128;
        vec128_t mat2_val = mat.col2.vec128;
        vec128_t mat3_val = mat.col3.vec128;
        asm __volatile__(
            "qmtc2      %[col0_val], $vf1        \n"
            "qmtc2      %[col1_val], $vf2        \n"
            "qmtc2      %[col2_val], $vf3        \n"
            "qmtc2      %[col3_val], $vf4        \n"
            "qmtc2      %[mat0_val], $vf5        \n"
            "qmtc2      %[mat1_val], $vf6        \n"
            "qmtc2      %[mat2_val], $vf7        \n"
            "qmtc2      %[mat3_val], $vf8        \n"
            "vsuba      $ACC, $vf0, $vf0         \n"
            "vmulaz     $ACC, $vf1, $vf5         \n"
            "vmaddaz    $ACC, $vf2, $vf6         \n"
            "vmaddaz    $ACC, $vf3, $vf7         \n"
            "vmaddz     $vf9, $vf4, $vf8         \n"
            "qmfc2      %[result], $vf9          \n"
            : [result] "=&r"(result), [acc] "=r"(vu0_ACC)
            : [col0_val] "r"(col0_val), [col1_val] "r"(col1_val), [col2_val] "r"(col2_val), [col3_val] "r"(col3_val),
              [mat0_val] "r"(mat0_val), [mat1_val] "r"(mat1_val), [mat2_val] "r"(mat2_val), [mat3_val] "r"(mat3_val)
            : "memory");
        return column_type(result);
    }

    template <class mat_column_type>
    column_type
    mult_trans_col3(const mat_x4_template<mat_column_type>& mat) const
    {
        vec128_t result;
        vec128_t col0_val = col0.vec128;
        vec128_t col1_val = col1.vec128;
        vec128_t col2_val = col2.vec128;
        vec128_t col3_val = col3.vec128;
        vec128_t mat0_val = mat.col0.vec128;
        vec128_t mat1_val = mat.col1.vec128;
        vec128_t mat2_val = mat.col2.vec128;
        vec128_t mat3_val = mat.col3.vec128;
        asm __volatile__(
            "qmtc2      %[col0_val], $vf1        \n"
            "qmtc2      %[col1_val], $vf2        \n"
            "qmtc2      %[col2_val], $vf3        \n"
            "qmtc2      %[col3_val], $vf4        \n"
            "qmtc2      %[mat0_val], $vf5        \n"
            "qmtc2      %[mat1_val], $vf6        \n"
            "qmtc2      %[mat2_val], $vf7        \n"
            "qmtc2      %[mat3_val], $vf8        \n"
            "vsuba      $ACC, $vf0, $vf0         \n"
            "vmulaw     $ACC, $vf1, $vf5         \n"
            "vmaddaw    $ACC, $vf2, $vf6         \n"
            "vmaddaw    $ACC, $vf3, $vf7         \n"
            "vmaddw     $vf9, $vf4, $vf8         \n"
            "qmfc2      %[result], $vf9          \n"
            : [result] "=&r"(result), [acc] "=r"(vu0_ACC)
            : [col0_val] "r"(col0_val), [col1_val] "r"(col1_val), [col2_val] "r"(col2_val), [col3_val] "r"(col3_val),
              [mat0_val] "r"(mat0_val), [mat1_val] "r"(mat1_val), [mat2_val] "r"(mat2_val), [mat3_val] "r"(mat3_val)
            : "memory");
        return column_type(result);
    }
};

#endif // matrix_common_h
