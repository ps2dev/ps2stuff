/*
 * cpu_compat_types.h - CPU-only implementations of VU0 vector/matrix types.
 *
 * Provides the same class names and API as vector.h/matrix.h but uses
 * standard C++ float arithmetic.  Compile with -DUSE_CPU_COMPAT to select
 * this path; the 12 VU0-specific tests (accumulator + DMA copy) are then
 * excluded from the test binary at compile time.
 */

#pragma once

#include <math.h>

/* ========================================================================
 * Forward declarations
 * ======================================================================== */

struct vec_xyz;
struct vec_xyzw;
struct mat_33;
struct mat_34;
struct mat_43;
struct mat_44;
struct transform_t;

/* ========================================================================
 * Single-component wrappers   (vec_x, vec_y, vec_z, vec_w)
 *
 * Used by ASSERT_VEC_EQ: (float)vec_x(v) extracts the x component and
 * converts to float.
 * ======================================================================== */

struct vec_x {
    float v;
    explicit vec_x(float f) : v(f) {}
    vec_x(const vec_xyz& t);
    vec_x(const vec_xyzw& t);
    operator float() const { return v; }
};

struct vec_y {
    float v;
    explicit vec_y(float f) : v(f) {}
    vec_y(const vec_xyz& t);
    vec_y(const vec_xyzw& t);
    operator float() const { return v; }
};

struct vec_z {
    float v;
    explicit vec_z(float f) : v(f) {}
    vec_z(const vec_xyz& t);
    vec_z(const vec_xyzw& t);
    operator float() const { return v; }
};

struct vec_w {
    float v;
    explicit vec_w(float f) : v(f) {}
    vec_w(const vec_xyz& t);
    vec_w(const vec_xyzw& t);
    operator float() const { return v; }
};

/* ========================================================================
 * vec_xyz  (3D float vector)
 * ======================================================================== */

struct vec_xyz {
    float x, y, z;

    vec_xyz() : x(0.0f), y(0.0f), z(0.0f) {}
    vec_xyz(float x_, float y_, float z_) : x(x_), y(y_), z(z_) {}
    // Conversion from vec_xyzw (drops w)
    vec_xyz(const vec_xyzw& v);

    void set(float s) { x = s; y = s; z = s; }
    void set(const vec_xyz& v) { x = v.x; y = v.y; z = v.z; }
    void set(float x_, float y_, float z_) { x = x_; y = y_; z = z_; }
    void set_zero() { x = 0.0f; y = 0.0f; z = 0.0f; }

    vec_xyz operator+(const vec_xyz& r) const { return {x+r.x, y+r.y, z+r.z}; }
    vec_xyz operator-(const vec_xyz& r) const { return {x-r.x, y-r.y, z-r.z}; }
    vec_xyz operator*(const vec_xyz& r) const { return {x*r.x, y*r.y, z*r.z}; }
    vec_xyz operator/(const vec_xyz& r) const { return {x/r.x, y/r.y, z/r.z}; }
    vec_xyz operator*(float s)          const { return {x*s,   y*s,   z*s};   }
    vec_xyz operator/(float s)          const { float r=1.0f/s; return {x*r, y*r, z*r}; }
    vec_xyz operator-()                 const { return {-x, -y, -z}; }

    vec_xyz& operator+=(const vec_xyz& r) { x+=r.x; y+=r.y; z+=r.z; return *this; }
    vec_xyz& operator-=(const vec_xyz& r) { x-=r.x; y-=r.y; z-=r.z; return *this; }
    vec_xyz& operator*=(const vec_xyz& r) { x*=r.x; y*=r.y; z*=r.z; return *this; }

    vec_xyz abs()             const { return {fabsf(x), fabsf(y), fabsf(z)}; }
    vec_xyz max(const vec_xyz& r) const {
        return {x > r.x ? x : r.x,
                y > r.y ? y : r.y,
                z > r.z ? z : r.z};
    }
    vec_xyz min(const vec_xyz& r) const {
        return {x < r.x ? x : r.x,
                y < r.y ? y : r.y,
                z < r.z ? z : r.z};
    }
    vec_xyz sign() const {
        return {x > 0.0f ? 1.0f : (x < 0.0f ? -1.0f : 0.0f),
                y > 0.0f ? 1.0f : (y < 0.0f ? -1.0f : 0.0f),
                z > 0.0f ? 1.0f : (z < 0.0f ? -1.0f : 0.0f)};
    }
    vec_xyz one_over() const { return {1.0f/x, 1.0f/y, 1.0f/z}; }

    float dot(const vec_xyz& r) const { return x*r.x + y*r.y + z*r.z; }
    float length_sqr()           const { return x*x + y*y + z*z; }
    float length()               const { return sqrtf(length_sqr()); }

    vec_xyz normalized() const {
        float len = length();
        if (len == 0.0f) return {0.0f, 0.0f, 0.0f};
        float inv = 1.0f / len;
        return {x*inv, y*inv, z*inv};
    }
    vec_xyz& normalize() { *this = normalized(); return *this; }

    vec_xyz cross(const vec_xyz& r) const {
        return {y*r.z - z*r.y,
                z*r.x - x*r.z,
                x*r.y - y*r.x};
    }

    int is_zero() const { return (x == 0.0f && y == 0.0f && z == 0.0f) ? 1 : 0; }

    vec_xyz set_length(float len) const { return normalized() * len; }

    vec_xyz truncate_length(float maxLen) const {
        float len = length();
        if (len <= maxLen || len == 0.0f) return *this;
        float scale = maxLen / len;
        return {x*scale, y*scale, z*scale};
    }

    vec_xyz interpolate(float alpha, const vec_xyz& other) const {
        return {x + alpha*(other.x - x),
                y + alpha*(other.y - y),
                z + alpha*(other.z - z)};
    }

    float distance_from(const vec_xyz& r) const { return (*this - r).length(); }

    // Parallel component of *this along unit vector basis
    vec_xyz parallel_component(const vec_xyz& unitBasis) const {
        return unitBasis * dot(unitBasis);
    }
    // Perpendicular component of *this to unit vector basis
    vec_xyz perpendicular_component(const vec_xyz& unitBasis) const {
        return *this - parallel_component(unitBasis);
    }

    // Index access
    float& operator[](int i)       { return (&x)[i]; }
    float  operator[](int i) const { return (&x)[i]; }
};

// Scalar-first multiply
inline vec_xyz operator*(float s, const vec_xyz& v) { return v * s; }

typedef vec_xyz vec_3;

/* ========================================================================
 * vec_xyzw  (4D float vector)
 * ======================================================================== */

struct vec_xyzw {
    float x, y, z, w;

    vec_xyzw() : x(0.0f), y(0.0f), z(0.0f), w(0.0f) {}
    vec_xyzw(float x_, float y_, float z_, float w_) : x(x_), y(y_), z(z_), w(w_) {}
    // 3-arg ctor sets w=0
    vec_xyzw(float x_, float y_, float z_) : x(x_), y(y_), z(z_), w(0.0f) {}
    // Construct from vec_xyz (w=0)
    vec_xyzw(const vec_xyz& v) : x(v.x), y(v.y), z(v.z), w(0.0f) {}

    void set(float x_, float y_, float z_, float w_) { x=x_; y=y_; z=z_; w=w_; }
    void set(float x_, float y_, float z_) { x=x_; y=y_; z=z_; }
    void set_zero() { x=0.0f; y=0.0f; z=0.0f; w=0.0f; }

    vec_xyzw operator+(const vec_xyzw& r) const { return {x+r.x, y+r.y, z+r.z, w+r.w}; }
    vec_xyzw operator-(const vec_xyzw& r) const { return {x-r.x, y-r.y, z-r.z, w-r.w}; }
    vec_xyzw operator*(const vec_xyzw& r) const { return {x*r.x, y*r.y, z*r.z, w*r.w}; }
    vec_xyzw operator*(float s)           const { return {x*s, y*s, z*s, w*s}; }
    vec_xyzw operator-()                  const { return {-x, -y, -z, -w}; }

    vec_xyzw& operator+=(const vec_xyzw& r) { x+=r.x; y+=r.y; z+=r.z; w+=r.w; return *this; }
    vec_xyzw& operator-=(const vec_xyzw& r) { x-=r.x; y-=r.y; z-=r.z; w-=r.w; return *this; }

    float dot(const vec_xyzw& r) const { return x*r.x + y*r.y + z*r.z + w*r.w; }
    float dot3(const vec_xyz& r)  const { return x*r.x + y*r.y + z*r.z; }

    vec_xyzw normalized3() const {
        float len = sqrtf(x*x + y*y + z*z);
        if (len == 0.0f) return *this;
        float inv = 1.0f / len;
        return {x*inv, y*inv, z*inv, w};
    }
    vec_xyzw& normalize3() { *this = normalized3(); return *this; }

    // Index access
    float& operator[](int i)       { return (&x)[i]; }
    float  operator[](int i) const { return (&x)[i]; }
};

inline vec_xyzw operator*(float s, const vec_xyzw& v) { return v * s; }

typedef vec_xyzw vec_4;

/* ========================================================================
 * vector_t  (direction: treat w as 0)
 * point_t   (position:  treat w as 1)
 * Both stored as vec_xyz; w is implicit.
 * ======================================================================== */

struct vector_t : public vec_xyz {
    vector_t() : vec_xyz() {}
    vector_t(float x_, float y_, float z_) : vec_xyz(x_, y_, z_) {}
    vector_t(const vec_xyz& v) : vec_xyz(v) {}
};

struct point_t : public vec_xyz {
    point_t() : vec_xyz() {}
    point_t(float x_, float y_, float z_) : vec_xyz(x_, y_, z_) {}
    point_t(const vec_xyz& v) : vec_xyz(v) {}
};

/* ========================================================================
 * Deferred single-component conversions (need full vec_xyz/vec_xyzw)
 * ======================================================================== */

inline vec_x::vec_x(const vec_xyz&  t) : v(t.x) {}
inline vec_x::vec_x(const vec_xyzw& t) : v(t.x) {}
inline vec_y::vec_y(const vec_xyz&  t) : v(t.y) {}
inline vec_y::vec_y(const vec_xyzw& t) : v(t.y) {}
inline vec_z::vec_z(const vec_xyz&  t) : v(t.z) {}
inline vec_z::vec_z(const vec_xyzw& t) : v(t.z) {}
inline vec_w::vec_w(const vec_xyz&  /*t*/) : v(0.0f) {}
inline vec_w::vec_w(const vec_xyzw& t)     : v(t.w) {}

inline vec_xyz::vec_xyz(const vec_xyzw& v) : x(v.x), y(v.y), z(v.z) {}

/* ========================================================================
 * mat_33  (3×3 column-major matrix)
 *
 * Storage: col0, col1, col2  (each vec_xyz = one column)
 * Matrix M acts on column vector v as:  M*v = col0*v.x + col1*v.y + col2*v.z
 * ======================================================================== */

struct mat_33 {
    vec_xyz col0, col1, col2;

    // Default constructor – initialises to identity
    mat_33() { set_identity(); }

    // Construct from 3 column vectors
    mat_33(const vec_xyz& c0, const vec_xyz& c1, const vec_xyz& c2)
        : col0(c0), col1(c1), col2(c2) {}

    // Construct from unit quaternion (x,y,z,w)
    explicit mat_33(const vec_xyzw& q) {
        float qx = q.x, qy = q.y, qz = q.z, qw = q.w;
        col0 = { 1.0f-2.0f*(qy*qy+qz*qz),  2.0f*(qx*qy+qz*qw),  2.0f*(qx*qz-qy*qw) };
        col1 = { 2.0f*(qx*qy-qz*qw),        1.0f-2.0f*(qx*qx+qz*qz),  2.0f*(qy*qz+qx*qw) };
        col2 = { 2.0f*(qx*qz+qy*qw),        2.0f*(qy*qz-qx*qw),  1.0f-2.0f*(qx*qx+qy*qy) };
    }

    void set_zero()     { col0.set_zero(); col1.set_zero(); col2.set_zero(); }
    void set_identity() { col0={1,0,0}; col1={0,1,0}; col2={0,0,1}; }
    void set_scale(const vec_xyz& s) {
        col0={s.x,0,0}; col1={0,s.y,0}; col2={0,0,s.z};
    }

    // Row access (each row is the i-th elements of all columns)
    void set_row0(const vec_xyz& r) { col0.x=r.x; col1.x=r.y; col2.x=r.z; }
    void set_row1(const vec_xyz& r) { col0.y=r.x; col1.y=r.y; col2.y=r.z; }
    void set_row2(const vec_xyz& r) { col0.z=r.x; col1.z=r.y; col2.z=r.z; }

    vec_xyz get_row0() const { return {col0.x, col1.x, col2.x}; }
    vec_xyz get_row1() const { return {col0.y, col1.y, col2.y}; }
    vec_xyz get_row2() const { return {col0.z, col1.z, col2.z}; }

    vec_xyz get_col0() const { return col0; }
    vec_xyz get_col1() const { return col1; }
    vec_xyz get_col2() const { return col2; }

    // Matrix-vector product: M * v
    vec_xyz operator*(const vec_xyz& v) const {
        return col0*v.x + col1*v.y + col2*v.z;
    }

    // Transpose-multiply: M^T * v = (dot(col0,v), dot(col1,v), dot(col2,v))
    vec_xyz trans_mult(const vec_xyz& v) const {
        return {col0.dot(v), col1.dot(v), col2.dot(v)};
    }

    mat_33 transpose() const {
        mat_33 r;
        r.col0 = get_row0(); r.col1 = get_row1(); r.col2 = get_row2();
        return r;
    }

    mat_33 operator-() const {
        mat_33 r; r.col0=-col0; r.col1=-col1; r.col2=-col2; return r;
    }

    mat_33 operator+(const mat_33& m) const {
        mat_33 r; r.col0=col0+m.col0; r.col1=col1+m.col1; r.col2=col2+m.col2; return r;
    }
    mat_33 operator-(const mat_33& m) const {
        mat_33 r; r.col0=col0-m.col0; r.col1=col1-m.col1; r.col2=col2-m.col2; return r;
    }

    // Matrix product: (A*B).col_i = A * B.col_i
    mat_33 operator*(const mat_33& m) const {
        mat_33 r;
        r.col0 = *this * m.col0;
        r.col1 = *this * m.col1;
        r.col2 = *this * m.col2;
        return r;
    }

    // Transpose-multiply: M^T * A
    mat_33 trans_mult(const mat_33& m) const {
        // (M^T * A).col_i = M^T * A.col_i = (dot(col0,A.col_i), dot(col1,A.col_i), dot(col2,A.col_i))
        mat_33 r;
        r.col0 = trans_mult(m.col0);
        r.col1 = trans_mult(m.col1);
        r.col2 = trans_mult(m.col2);
        return r;
    }

    void set_rotate_x(float a) {
        float c=cosf(a), s=sinf(a);
        col0={1,0,0}; col1={0,c,s}; col2={0,-s,c};
    }
    void set_rotate_y(float a) {
        float c=cosf(a), s=sinf(a);
        col0={c,0,-s}; col1={0,1,0}; col2={s,0,c};
    }
    void set_rotate_z(float a) {
        float c=cosf(a), s=sinf(a);
        col0={c,s,0}; col1={-s,c,0}; col2={0,0,1};
    }

    // mult_tilde: M * [v]× where [v]× is the skew-symmetric matrix
    // [v]×: col0=(0,vz,-vy), col1=(-vz,0,vx), col2=(vy,-vx,0)
    mat_33 mult_tilde(const vec_xyz& v) const {
        mat_33 r;
        r.col0 = *this * vec_xyz( 0.0f,  v.z, -v.y);
        r.col1 = *this * vec_xyz(-v.z,  0.0f,  v.x);
        r.col2 = *this * vec_xyz( v.y, -v.x,  0.0f);
        return r;
    }

    // 3×3 inverse via cofactors (Cramer's rule).
    // Column-major names: a=col0.x=M[0][0], d=col0.y=M[1][0], g=col0.z=M[2][0]
    //                     b=col1.x=M[0][1], e=col1.y=M[1][1], h=col1.z=M[2][1]
    //                     c=col2.x=M[0][2], f=col2.y=M[1][2], i=col2.z=M[2][2]
    // inv.col_k = (C[0][k], C[1][k], C[2][k]) / det  (adjugate row k)
    mat_33 inverse() const {
        float a=col0.x, d=col0.y, g=col0.z;
        float b=col1.x, e=col1.y, h=col1.z;
        float c=col2.x, f=col2.y, ii=col2.z;
        float det = a*(e*ii - f*h) - b*(d*ii - f*g) + c*(d*h - e*g);
        float inv_det = 1.0f / det;
        mat_33 r;
        r.col0.x = ( e*ii - f*h)  * inv_det;
        r.col0.y = -(d*ii - f*g)  * inv_det;
        r.col0.z = ( d*h  - e*g)  * inv_det;
        r.col1.x = -(b*ii - c*h)  * inv_det;
        r.col1.y = ( a*ii - c*g)  * inv_det;
        r.col1.z = -(a*h  - b*g)  * inv_det;
        r.col2.x = ( b*f  - c*e)  * inv_det;
        r.col2.y = -(a*f  - c*d)  * inv_det;
        r.col2.z = ( a*e  - b*d)  * inv_det;
        return r;
    }
};

// Skew-symmetric matrix operator: ~v creates [v]×
inline mat_33 operator~(const vec_xyz& v) {
    mat_33 r;
    r.col0 = { 0.0f,  v.z, -v.y};
    r.col1 = {-v.z,  0.0f,  v.x};
    r.col2 = { v.y,  -v.x, 0.0f};
    return r;
}

// Row-vector * matrix: v^T * M = M^T * v
inline vec_xyz operator*(const vec_xyz& v, const mat_33& m) {
    return {v.dot(m.col0), v.dot(m.col1), v.dot(m.col2)};
}

/* ========================================================================
 * mat_43  (4 rows × 3 columns, column-major)
 *
 * Storage: col0, col1, col2  (each vec_xyzw = column with 4 floats)
 * Product: mat_43 * vec_3 = vec_4  (result = col0*v.x + col1*v.y + col2*v.z)
 * ======================================================================== */

struct mat_43 {
    vec_xyzw col0, col1, col2;

    mat_43() { set_zero(); }
    mat_43(const vec_xyzw& c0, const vec_xyzw& c1, const vec_xyzw& c2)
        : col0(c0), col1(c1), col2(c2) {}

    void set_zero() { col0.set_zero(); col1.set_zero(); col2.set_zero(); }

    vec_xyz get_row0() const { return {col0.x, col1.x, col2.x}; }
    vec_xyz get_row1() const { return {col0.y, col1.y, col2.y}; }
    vec_xyz get_row2() const { return {col0.z, col1.z, col2.z}; }
    vec_xyz get_row3() const { return {col0.w, col1.w, col2.w}; }

    vec_xyzw get_col0() const { return col0; }
    vec_xyzw get_col1() const { return col1; }
    vec_xyzw get_col2() const { return col2; }

    // mat_43 * vec_3 = vec_4
    vec_xyzw operator*(const vec_xyz& v) const {
        vec_xyzw r;
        r.x = col0.x*v.x + col1.x*v.y + col2.x*v.z;
        r.y = col0.y*v.x + col1.y*v.y + col2.y*v.z;
        r.z = col0.z*v.x + col1.z*v.y + col2.z*v.z;
        r.w = col0.w*v.x + col1.w*v.y + col2.w*v.z;
        return r;
    }

    // Transpose-multiply: (mat_43)^T * vec_4 = vec_3
    // (mat_43^T).row_i = col_i, so result_i = dot(col_i, v)
    vec_xyz trans_mult(const vec_xyzw& v) const {
        return {col0.dot(v), col1.dot(v), col2.dot(v)};
    }
    vec_xyz trans_mult(const vector_t& v) const {
        vec_xyzw v4(v.x, v.y, v.z, 0.0f);
        return trans_mult(v4);
    }
    vec_xyz trans_mult(const point_t& v) const {
        vec_xyzw v4(v.x, v.y, v.z, 1.0f);
        return trans_mult(v4);
    }

    mat_34 transpose() const; // defined after mat_34 is complete

    mat_43 operator-() const {
        return {-col0, -col1, -col2};
    }
    mat_43 operator+(const mat_43& m) const {
        return {col0+m.col0, col1+m.col1, col2+m.col2};
    }
    mat_43 operator-(const mat_43& m) const {
        return {col0-m.col0, col1-m.col1, col2-m.col2};
    }

    // mat_43 * mat_33 = mat_43
    // result.col_i = (mat_43) * (mat_33).col_i
    mat_43 operator*(const mat_33& m) const {
        mat_43 r;
        r.col0 = *this * m.col0;
        r.col1 = *this * m.col1;
        r.col2 = *this * m.col2;
        return r;
    }

    // mult_tilde: (mat_43) * [v]× where [v]× cols are vec_3
    // [v]×: col0=(0,vz,-vy), col1=(-vz,0,vx), col2=(vy,-vx,0)
    mat_43 mult_tilde(const vec_xyz& v) const {
        mat_43 r;
        r.col0 = *this * vec_xyz( 0.0f,  v.z, -v.y);
        r.col1 = *this * vec_xyz(-v.z,  0.0f,  v.x);
        r.col2 = *this * vec_xyz( v.y, -v.x,  0.0f);
        return r;
    }
};

/* ========================================================================
 * mat_34  (3 rows × 4 columns, column-major)
 *
 * Storage: col0, col1, col2, col3  (each vec_xyz = column with 3 floats)
 * Product: mat_34 * vec_4 = vec_3  (result = col0*v.x + col1*v.y + col2*v.z + col3*v.w)
 * ======================================================================== */

struct mat_34 {
    vec_xyz col0, col1, col2, col3;

    mat_34() { set_zero(); }
    mat_34(const vec_xyz& c0, const vec_xyz& c1, const vec_xyz& c2, const vec_xyz& c3)
        : col0(c0), col1(c1), col2(c2), col3(c3) {}

    void set_zero() { col0.set_zero(); col1.set_zero(); col2.set_zero(); col3.set_zero(); }

    vec_xyzw get_row0() const { return {col0.x, col1.x, col2.x, col3.x}; }
    vec_xyzw get_row1() const { return {col0.y, col1.y, col2.y, col3.y}; }
    vec_xyzw get_row2() const { return {col0.z, col1.z, col2.z, col3.z}; }

    // mat_34 * vec_4 = vec_3
    vec_xyz operator*(const vec_xyzw& v) const {
        return col0*v.x + col1*v.y + col2*v.z + col3*v.w;
    }
    vec_xyz operator*(const vector_t& v) const {
        return col0*v.x + col1*v.y + col2*v.z;
    }
    vec_xyz operator*(const point_t& v) const {
        return col0*v.x + col1*v.y + col2*v.z + col3;
    }

    // Transpose-multiply: (mat_34)^T * vec_3 = vec_4
    // (mat_34^T).row_i = col_i, so result_i = dot(col_i, v)
    vec_xyzw trans_mult(const vec_xyz& v) const {
        return {col0.dot(v), col1.dot(v), col2.dot(v), col3.dot(v)};
    }

    mat_43 transpose() const; // defined after mat_43 is complete

    mat_34 operator-() const {
        return {-col0, -col1, -col2, -col3};
    }
    mat_34 operator+(const mat_34& m) const {
        return {col0+m.col0, col1+m.col1, col2+m.col2, col3+m.col3};
    }
    mat_34 operator-(const mat_34& m) const {
        return {col0-m.col0, col1-m.col1, col2-m.col2, col3-m.col3};
    }

    // mat_34 * mat_43 = mat_33
    // result.col_i = (mat_34) * (mat_43).col_i   [mat_34 * vec_4]
    mat_33 operator*(const mat_43& m) const {
        mat_33 r;
        r.col0 = *this * m.col0;
        r.col1 = *this * m.col1;
        r.col2 = *this * m.col2;
        return r;
    }

    // mat_34 * mat_44 = mat_34
    mat_34 operator*(const mat_44& m) const; // defined after mat_44
};

/* ========================================================================
 * mat_43::transpose() returns mat_34 – defined here after mat_34 is declared
 * ======================================================================== */

inline mat_34 mat_43_to_mat_34(const mat_43& m) {
    mat_34 r;
    // Row i of mat_43 = (col0[i], col1[i], col2[i])
    // Col j of mat_34 (= row j of mat_43):
    r.col0 = { m.col0.x, m.col1.x, m.col2.x };  // row0 as vec_3
    r.col1 = { m.col0.y, m.col1.y, m.col2.y };  // row1 as vec_3
    r.col2 = { m.col0.z, m.col1.z, m.col2.z };  // row2 as vec_3
    r.col3 = { m.col0.w, m.col1.w, m.col2.w };  // row3 as vec_3
    return r;
}

inline mat_43 mat_34_to_mat_43(const mat_34& m) {
    mat_43 r;
    // Transpose of mat_34 (3x4) = mat_43 (4x3)
    // col0 of result = row0 of mat_34 = (col0.x, col1.x, col2.x, col3.x)
    r.col0 = { m.col0.x, m.col1.x, m.col2.x, m.col3.x };
    r.col1 = { m.col0.y, m.col1.y, m.col2.y, m.col3.y };
    r.col2 = { m.col0.z, m.col1.z, m.col2.z, m.col3.z };
    return r;
}

// Row-vector * mat_43: vec_4^T * mat_43 = vec_3
inline vec_xyz operator*(const vec_xyzw& v, const mat_43& m) {
    return {v.dot(m.col0), v.dot(m.col1), v.dot(m.col2)};
}

// Row-vector * mat_34: vec_3^T * mat_34 = vec_4
inline vec_xyzw operator*(const vec_xyz& v, const mat_34& m) {
    return {v.dot(m.col0), v.dot(m.col1), v.dot(m.col2), v.dot(m.col3)};
}

/* ========================================================================
 * mat_44  (4×4 column-major matrix)
 * ======================================================================== */

struct mat_44 {
    vec_xyzw col0, col1, col2, col3;

    mat_44() { set_identity(); }

    // Construct from 4 column vectors
    mat_44(const vec_xyzw& c0, const vec_xyzw& c1, const vec_xyzw& c2, const vec_xyzw& c3)
        : col0(c0), col1(c1), col2(c2), col3(c3) {}

    void set_zero()     { col0.set_zero(); col1.set_zero(); col2.set_zero(); col3.set_zero(); }
    void set_identity() {
        col0={1,0,0,0}; col1={0,1,0,0}; col2={0,0,1,0}; col3={0,0,0,1};
    }
    void set_scale(const vec_xyz& s) {
        col0={s.x,0,0,0}; col1={0,s.y,0,0}; col2={0,0,s.z,0}; col3={0,0,0,1};
    }
    void set_translate(const vec_xyz& t) {
        col0={1,0,0,0}; col1={0,1,0,0}; col2={0,0,1,0}; col3={t.x,t.y,t.z,1};
    }

    void set_row0(const vec_xyzw& r) { col0.x=r.x; col1.x=r.y; col2.x=r.z; col3.x=r.w; }
    void set_row1(const vec_xyzw& r) { col0.y=r.x; col1.y=r.y; col2.y=r.z; col3.y=r.w; }
    void set_row2(const vec_xyzw& r) { col0.z=r.x; col1.z=r.y; col2.z=r.z; col3.z=r.w; }
    void set_row3(const vec_xyzw& r) { col0.w=r.x; col1.w=r.y; col2.w=r.z; col3.w=r.w; }

    vec_xyzw get_row0() const { return {col0.x, col1.x, col2.x, col3.x}; }
    vec_xyzw get_row1() const { return {col0.y, col1.y, col2.y, col3.y}; }
    vec_xyzw get_row2() const { return {col0.z, col1.z, col2.z, col3.z}; }
    vec_xyzw get_row3() const { return {col0.w, col1.w, col2.w, col3.w}; }

    vec_xyzw get_col0() const { return col0; }
    vec_xyzw get_col1() const { return col1; }
    vec_xyzw get_col2() const { return col2; }
    vec_xyzw get_col3() const { return col3; }

    // M * v
    vec_xyzw operator*(const vec_xyzw& v) const {
        vec_xyzw r;
        r.x = col0.x*v.x + col1.x*v.y + col2.x*v.z + col3.x*v.w;
        r.y = col0.y*v.x + col1.y*v.y + col2.y*v.z + col3.y*v.w;
        r.z = col0.z*v.x + col1.z*v.y + col2.z*v.z + col3.z*v.w;
        r.w = col0.w*v.x + col1.w*v.y + col2.w*v.z + col3.w*v.w;
        return r;
    }

    // Transpose-multiply: M^T * v
    vec_xyzw trans_mult(const vec_xyzw& v) const {
        return {col0.dot(v), col1.dot(v), col2.dot(v), col3.dot(v)};
    }
    vec_xyzw trans_mult(const vector_t& v) const {
        vec_xyzw v4(v.x, v.y, v.z, 0.0f);
        return trans_mult(v4);
    }
    vec_xyzw trans_mult(const point_t& v) const {
        vec_xyzw v4(v.x, v.y, v.z, 1.0f);
        return trans_mult(v4);
    }

    mat_44 transpose() const {
        mat_44 r;
        r.col0={col0.x,col1.x,col2.x,col3.x};
        r.col1={col0.y,col1.y,col2.y,col3.y};
        r.col2={col0.z,col1.z,col2.z,col3.z};
        r.col3={col0.w,col1.w,col2.w,col3.w};
        return r;
    }

    mat_44 operator-() const {
        mat_44 r; r.col0=-col0; r.col1=-col1; r.col2=-col2; r.col3=-col3; return r;
    }
    mat_44 operator+(const mat_44& m) const {
        mat_44 r; r.col0=col0+m.col0; r.col1=col1+m.col1;
                  r.col2=col2+m.col2; r.col3=col3+m.col3; return r;
    }
    mat_44 operator-(const mat_44& m) const {
        mat_44 r; r.col0=col0-m.col0; r.col1=col1-m.col1;
                  r.col2=col2-m.col2; r.col3=col3-m.col3; return r;
    }

    // mat_44 * mat_44
    mat_44 operator*(const mat_44& m) const {
        mat_44 r;
        r.col0 = *this * m.col0;
        r.col1 = *this * m.col1;
        r.col2 = *this * m.col2;
        r.col3 = *this * m.col3;
        return r;
    }

    // Transpose-multiply: M^T * A
    mat_44 trans_mult(const mat_44& m) const {
        mat_44 r;
        r.col0 = trans_mult(m.col0);
        r.col1 = trans_mult(m.col1);
        r.col2 = trans_mult(m.col2);
        r.col3 = trans_mult(m.col3);
        return r;
    }

    void set_rotate_x(float a) {
        float c=cosf(a), s=sinf(a);
        col0={1,0,0,0}; col1={0,c,s,0}; col2={0,-s,c,0}; col3={0,0,0,1};
    }
    void set_rotate_y(float a) {
        float c=cosf(a), s=sinf(a);
        col0={c,0,-s,0}; col1={0,1,0,0}; col2={s,0,c,0}; col3={0,0,0,1};
    }
    void set_rotate_z(float a) {
        float c=cosf(a), s=sinf(a);
        col0={c,s,0,0}; col1={-s,c,0,0}; col2={0,0,1,0}; col3={0,0,0,1};
    }

    // mat_44 * mat_43 = mat_43
    // result.col_i = (mat_44) * (mat_43).col_i   [mat_44 * vec_4]
    mat_43 operator*(const mat_43& m) const {
        mat_43 r;
        r.col0 = *this * m.col0;
        r.col1 = *this * m.col1;
        r.col2 = *this * m.col2;
        return r;
    }
};

// Row-vector * mat_44: vec_4^T * mat_44 = vec_4
inline vec_xyzw operator*(const vec_xyzw& v, const mat_44& m) {
    return {v.dot(m.col0), v.dot(m.col1), v.dot(m.col2), v.dot(m.col3)};
}

// mat_34 * mat_44 = mat_34 (defined here now that mat_44 is complete)
inline mat_34 mat_34_mult_mat_44(const mat_34& a, const mat_44& b) {
    // result.col_i = a * b.col_i   [mat_34 * vec_4]
    mat_34 r;
    r.col0 = a * b.col0;
    r.col1 = a * b.col1;
    r.col2 = a * b.col2;
    r.col3 = a * b.col3;
    return r;
}

/* ========================================================================
 * mat_43::transpose / mat_34::transpose – deferred method bodies
 * ======================================================================== */

// These must live outside the struct bodies since both mat_43 and mat_34
// need to be complete before defining each other's transpose.

/* ========================================================================
 * transform_t  (rigid body transform: 3×3 rotation + 3D translation)
 *
 * Stored as 4×4 column-major matrix:
 *   col0, col1, col2 = rotation columns (w=0)
 *   col3             = translation (w=1)
 * ======================================================================== */

struct transform_t {
    vec_xyzw col0, col1, col2, col3;

    transform_t() { set_identity(); }

    void set_identity() {
        col0={1,0,0,0}; col1={0,1,0,0}; col2={0,0,1,0}; col3={0,0,0,1};
    }
    void set_zero() {
        col0.set_zero(); col1.set_zero(); col2.set_zero(); col3.set_zero();
    }

    // Row access (includes translation column)
    vec_xyzw get_row0() const { return {col0.x, col1.x, col2.x, col3.x}; }
    vec_xyzw get_row1() const { return {col0.y, col1.y, col2.y, col3.y}; }
    vec_xyzw get_row2() const { return {col0.z, col1.z, col2.z, col3.z}; }
    vec_xyzw get_row3() const { return {col0.w, col1.w, col2.w, col3.w}; }

    // Apply to 4D vector: result = R*v.xyz + t*v.w
    vec_xyzw operator*(const vec_xyzw& v) const {
        vec_xyzw r;
        r.x = col0.x*v.x + col1.x*v.y + col2.x*v.z + col3.x*v.w;
        r.y = col0.y*v.x + col1.y*v.y + col2.y*v.z + col3.y*v.w;
        r.z = col0.z*v.x + col1.z*v.y + col2.z*v.z + col3.z*v.w;
        r.w = col0.w*v.x + col1.w*v.y + col2.w*v.z + col3.w*v.w;
        return r;
    }

    // Apply to direction (w=0): rotation only
    vector_t operator*(const vector_t& v) const {
        return { col0.x*v.x + col1.x*v.y + col2.x*v.z,
                 col0.y*v.x + col1.y*v.y + col2.y*v.z,
                 col0.z*v.x + col1.z*v.y + col2.z*v.z };
    }

    // Apply to point (w=1): rotation + translation
    point_t operator*(const point_t& v) const {
        return { col0.x*v.x + col1.x*v.y + col2.x*v.z + col3.x,
                 col0.y*v.x + col1.y*v.y + col2.y*v.z + col3.y,
                 col0.z*v.x + col1.z*v.y + col2.z*v.z + col3.z };
    }

    // Compose transforms
    transform_t operator*(const transform_t& b) const {
        transform_t r;
        r.col0 = {col0.x*b.col0.x + col1.x*b.col0.y + col2.x*b.col0.z,
                  col0.y*b.col0.x + col1.y*b.col0.y + col2.y*b.col0.z,
                  col0.z*b.col0.x + col1.z*b.col0.y + col2.z*b.col0.z,
                  0.0f};
        r.col1 = {col0.x*b.col1.x + col1.x*b.col1.y + col2.x*b.col1.z,
                  col0.y*b.col1.x + col1.y*b.col1.y + col2.y*b.col1.z,
                  col0.z*b.col1.x + col1.z*b.col1.y + col2.z*b.col1.z,
                  0.0f};
        r.col2 = {col0.x*b.col2.x + col1.x*b.col2.y + col2.x*b.col2.z,
                  col0.y*b.col2.x + col1.y*b.col2.y + col2.y*b.col2.z,
                  col0.z*b.col2.x + col1.z*b.col2.y + col2.z*b.col2.z,
                  0.0f};
        r.col3 = {col0.x*b.col3.x + col1.x*b.col3.y + col2.x*b.col3.z + col3.x,
                  col0.y*b.col3.x + col1.y*b.col3.y + col2.y*b.col3.z + col3.y,
                  col0.z*b.col3.x + col1.z*b.col3.y + col2.z*b.col3.z + col3.z,
                  1.0f};
        return r;
    }

    // Orthonormal inverse: inv = { R^T, -R^T*t, 0,0,0,1 }
    // inv.col_i (rotation part) = (col0[i], col1[i], col2[i], 0)  [transpose of rotation cols]
    // inv.col3 = (-dot(col0.xyz, col3.xyz), -dot(col1.xyz, col3.xyz), -dot(col2.xyz, col3.xyz), 1)
    transform_t orthonormal_inverse() const {
        transform_t r;
        // Transposed rotation
        r.col0 = {col0.x, col1.x, col2.x, 0.0f};
        r.col1 = {col0.y, col1.y, col2.y, 0.0f};
        r.col2 = {col0.z, col1.z, col2.z, 0.0f};
        // Translation: -R^T * t
        float tx = col3.x, ty = col3.y, tz = col3.z;
        r.col3 = {-(col0.x*tx + col0.y*ty + col0.z*tz),
                  -(col1.x*tx + col1.y*ty + col1.z*tz),
                  -(col2.x*tx + col2.y*ty + col2.z*tz),
                  1.0f};
        return r;
    }

    void orthonormal_inverse_in_place() {
        *this = orthonormal_inverse();
    }

    // General inverse (uses orthonormal_inverse since transform_t is always rigid)
    transform_t inverse() const {
        return orthonormal_inverse();
    }

    void set_rotation(const mat_33& rot) {
        col0 = {rot.col0.x, rot.col0.y, rot.col0.z, 0.0f};
        col1 = {rot.col1.x, rot.col1.y, rot.col1.z, 0.0f};
        col2 = {rot.col2.x, rot.col2.y, rot.col2.z, 0.0f};
    }
    void set_translation(const vec_xyz& t) {
        col3 = {t.x, t.y, t.z, 1.0f};
    }
    void set_rotate_x(float a) {
        float c=cosf(a), s=sinf(a);
        col0={1,0,0,0}; col1={0,c,s,0}; col2={0,-s,c,0}; col3={0,0,0,1};
    }
    void set_rotate_y(float a) {
        float c=cosf(a), s=sinf(a);
        col0={c,0,-s,0}; col1={0,1,0,0}; col2={s,0,c,0}; col3={0,0,0,1};
    }
    void set_rotate_z(float a) {
        float c=cosf(a), s=sinf(a);
        col0={c,s,0,0}; col1={-s,c,0,0}; col2={0,0,1,0}; col3={0,0,0,1};
    }
};

/* ========================================================================
 * Deferred member function bodies (all types now complete)
 * ======================================================================== */

// mat_43::transpose() -> mat_34
inline mat_34 mat_43::transpose() const { return mat_43_to_mat_34(*this); }

// mat_34::transpose() -> mat_43
inline mat_43 mat_34::transpose() const { return mat_34_to_mat_43(*this); }

// mat_34 * mat_44 -> mat_34
inline mat_34 mat_34::operator*(const mat_44& m) const {
    return mat_34_mult_mat_44(*this, m);
}

