/*	  Copyright (C) 2000,2001,2002  Sony Computer Entertainment America
 *
 *       	  This file is subject to the terms and conditions of the GNU Lesser
 *	  General Public License Version 2.1. See the file "COPYING" in the
 *	  main directory of this archive for more details.                             */

#include <stdio.h>
#include <math.h>
#ifdef USE_CPU_COMPAT
#include <ps2s/cpu_compat_types.h>
#define NO_VU0_VECTORS
#include <ps2s/cpu_vector.h>
#include <ps2s/cpu_matrix.h>
#undef NO_VU0_VECTORS
#else
#include <ps2s/vector.h>
#include <ps2s/matrix.h>
#include <ps2s/vu.h>
#include <ps2s/ps2stuff.h>
// Also exercise the cpu_vec_4 / cpu_mat_44 VU0-asm fast paths.
#include <ps2s/cpu_vector.h>
#include <ps2s/cpu_matrix.h>
#endif
#include <kernel.h>
#include <string.h>

// Test framework macros
#define EPSILON 1e-5f
#define ASSERT_EQ(a, b) \
    do { \
        if (fabsf((a) - (b)) > EPSILON) { \
            printf("FAIL: %s:%d - Expected %.6f, got %.6f\n", __FILE__, __LINE__, (float)(b), (float)(a)); \
            return 1; \
        } \
    } while(0)

#define ASSERT_VEC_EQ(v, x, y, z, w) \
    do { \
        float vx = (float)vec_x(v); \
        float vy = (float)vec_y(v); \
        float vz = (float)vec_z(v); \
        float vw = (float)vec_w(v); \
        if (fabsf(vx - (x)) > EPSILON || \
            fabsf(vy - (y)) > EPSILON || \
            fabsf(vz - (z)) > EPSILON || \
            fabsf(vw - (w)) > EPSILON) { \
            printf("FAIL: %s:%d - Expected (%.6f, %.6f, %.6f, %.6f), got (%.6f, %.6f, %.6f, %.6f)\n", \
                   __FILE__, __LINE__, (float)(x), (float)(y), (float)(z), (float)(w), \
                   vx, vy, vz, vw); \
            return 1; \
        } \
    } while(0)

#define ASSERT_VEC3_EQ(v, x, y, z) \
    do { \
        float vx = (float)vec_x(v); \
        float vy = (float)vec_y(v); \
        float vz = (float)vec_z(v); \
        if (fabsf(vx - (x)) > EPSILON || \
            fabsf(vy - (y)) > EPSILON || \
            fabsf(vz - (z)) > EPSILON) { \
            printf("FAIL: %s:%d - Expected (%.6f, %.6f, %.6f), got (%.6f, %.6f, %.6f)\n", \
                   __FILE__, __LINE__, (float)(x), (float)(y), (float)(z), \
                   vx, vy, vz); \
            return 1; \
        } \
    } while(0)

// CPU legacy type macros (cpu_vec_3, cpu_vec_4 have direct .x/.y/.z/.w members)
#ifdef USE_CPU_COMPAT
#define ASSERT_FLOAT_EQ(a, b) \
    do { \
        if (fabsf((float)(a) - (float)(b)) > EPSILON) { \
            printf("FAIL: %s:%d - Expected %.6f, got %.6f\n", __FILE__, __LINE__, (float)(b), (float)(a)); \
            return 1; \
        } \
    } while(0)
#define ASSERT_CPU_VEC3_EQ(v, ex, ey, ez) \
    do { \
        if (fabsf((v).x - (ex)) > EPSILON || \
            fabsf((v).y - (ey)) > EPSILON || \
            fabsf((v).z - (ez)) > EPSILON) { \
            printf("FAIL: %s:%d - Expected (%.6f, %.6f, %.6f), got (%.6f, %.6f, %.6f)\n", \
                   __FILE__, __LINE__, (float)(ex), (float)(ey), (float)(ez), \
                   (v).x, (v).y, (v).z); \
            return 1; \
        } \
    } while(0)
#define ASSERT_CPU_VEC4_EQ(v, ex, ey, ez, ew) \
    do { \
        if (fabsf((v).x - (ex)) > EPSILON || \
            fabsf((v).y - (ey)) > EPSILON || \
            fabsf((v).z - (ez)) > EPSILON || \
            fabsf((v).w - (ew)) > EPSILON) { \
            printf("FAIL: %s:%d - Expected (%.6f, %.6f, %.6f, %.6f), got (%.6f, %.6f, %.6f, %.6f)\n", \
                   __FILE__, __LINE__, \
                   (float)(ex), (float)(ey), (float)(ez), (float)(ew), \
                   (v).x, (v).y, (v).z, (v).w); \
            return 1; \
        } \
    } while(0)
#endif // USE_CPU_COMPAT

// Assertions for cpu_vec_4 .x/.y/.z/.w (available in both builds — needed
// by the VU0 build to test the new VU0-asm fast paths in cpu_vector.h /
// cpu_matrix.h).
#ifndef USE_CPU_COMPAT
#define ASSERT_FLOAT_EQ(a, b) \
    do { \
        if (fabsf((float)(a) - (float)(b)) > EPSILON) { \
            printf("FAIL: %s:%d - Expected %.6f, got %.6f\n", __FILE__, __LINE__, (float)(b), (float)(a)); \
            return 1; \
        } \
    } while(0)
#define ASSERT_CPU_VEC4_EQ(v, ex, ey, ez, ew) \
    do { \
        if (fabsf((v).x - (ex)) > EPSILON || \
            fabsf((v).y - (ey)) > EPSILON || \
            fabsf((v).z - (ez)) > EPSILON || \
            fabsf((v).w - (ew)) > EPSILON) { \
            printf("FAIL: %s:%d - Expected (%.6f, %.6f, %.6f, %.6f), got (%.6f, %.6f, %.6f, %.6f)\n", \
                   __FILE__, __LINE__, \
                   (float)(ex), (float)(ey), (float)(ez), (float)(ew), \
                   (v).x, (v).y, (v).z, (v).w); \
            return 1; \
        } \
    } while(0)
#endif // !USE_CPU_COMPAT

static int test_count = 0;
static const char* failed_tests[200];
static int failed_count = 0;

#define RUN_TEST(test_func) \
    do { \
        test_count++; \
        printf("Running %s...\n", #test_func); \
        int result = test_func(); \
        if (result != 0) { \
            printf("TEST FAILED: %s\n\n", #test_func); \
            if (failed_count < 200) { \
                failed_tests[failed_count++] = #test_func; \
            } \
        } else { \
            printf("PASS: %s\n\n", #test_func); \
        } \
    } while(0)

// Emulator detection helper
// Returns 1 if the VU multiply-1 bug (1 * X != X) is present (i.e. hardware),
// or 0 if the environment behaves like common emulators.
int isVUMulErrorPresent()
{
    float in = 129.5f;
    float out = 0.0f;
    asm __volatile__(
        "QMTC2 %1, $vf1\n\t"          // Set VF1 to 129.5f
        "VADDw $vf2, $vf0, $vf0w\n\t" // VF2 = vf0[w] = 1
        "VMUL $vf1, $vf2, $vf1\n\t"   // VF1 = 1 * 129.5f
        "QMFC2 %0, $vf1\n\t"          // Load result back
        : "=r"(out)
        : "r"(in)
        : "memory");
    return (in != out);
}

// Global flag set at test startup
int g_vu_mul_emulator_bug = -1;

// Test functions

int test_vec3_addition()
{
    vec_xyz v1(1.0f, 2.0f, 3.0f);
    vec_xyz v2(4.0f, 5.0f, 6.0f);
    vec_xyz result = v1 + v2;
    ASSERT_VEC3_EQ(result, 5.0f, 7.0f, 9.0f);
    return 0;
}

int test_vec3_subtraction()
{
    vec_xyz v1(5.0f, 7.0f, 9.0f);
    vec_xyz v2(1.0f, 2.0f, 3.0f);
    vec_xyz result = v1 - v2;
    ASSERT_VEC3_EQ(result, 4.0f, 5.0f, 6.0f);
    return 0;
}

int test_vec3_multiplication()
{
    vec_xyz v1(2.0f, 3.0f, 4.0f);
    vec_xyz v2(2.0f, 2.0f, 2.0f);
    vec_xyz result = v1 * v2;
    ASSERT_VEC3_EQ(result, 4.0f, 6.0f, 8.0f);
    return 0;
}

int test_vec3_scalar_multiplication()
{
    vec_xyz v1(2.0f, 3.0f, 4.0f);
    vec_xyz result = v1 * 2.0f;
    ASSERT_VEC3_EQ(result, 4.0f, 6.0f, 8.0f);
    return 0;
}

int test_vec3_division()
{
    vec_xyz v1(8.0f, 6.0f, 4.0f);
    vec_xyz v2(2.0f, 2.0f, 2.0f);
    vec_xyz result = v1 / v2;
    ASSERT_VEC3_EQ(result, 4.0f, 3.0f, 2.0f);
    return 0;
}

int test_vec3_negate()
{
    vec_xyz v1(1.0f, 2.0f, 3.0f);
    vec_xyz result = -v1;
    ASSERT_VEC3_EQ(result, -1.0f, -2.0f, -3.0f);
    return 0;
}

int test_vec3_abs()
{
    vec_xyz v1(-1.0f, 2.0f, -3.0f);
    vec_xyz result = v1.abs();
    ASSERT_VEC3_EQ(result, 1.0f, 2.0f, 3.0f);
    return 0;
}

int test_vec3_max()
{
    vec_xyz v1(1.0f, 5.0f, 3.0f);
    vec_xyz v2(4.0f, 2.0f, 6.0f);
    vec_xyz result = v1.max(v2);
    ASSERT_VEC3_EQ(result, 4.0f, 5.0f, 6.0f);
    return 0;
}

int test_vec3_min()
{
    vec_xyz v1(1.0f, 5.0f, 3.0f);
    vec_xyz v2(4.0f, 2.0f, 6.0f);
    vec_xyz result = v1.min(v2);
    ASSERT_VEC3_EQ(result, 1.0f, 2.0f, 3.0f);
    return 0;
}

int test_vec3_normalized()
{
    vec_xyz v1(3.0f, 4.0f, 0.0f);  // Length = 5
    vec_xyz result = v1.normalized();
    float rx = (float)vec_x(result);
    float ry = (float)vec_y(result);
    float rz = (float)vec_z(result);
    float len = sqrtf(rx * rx + ry * ry + rz * rz);
    ASSERT_EQ(len, 1.0f);
    ASSERT_VEC3_EQ(result, 0.6f, 0.8f, 0.0f);
    return 0;
}

int test_vec3_set_zero()
{
    vec_xyz v1(1.0f, 2.0f, 3.0f);
    v1.set_zero();
    ASSERT_VEC3_EQ(v1, 0.0f, 0.0f, 0.0f);
    return 0;
}

int test_vec3_set()
{
    vec_xyz v1;
    v1.set(5.0f);
    ASSERT_VEC3_EQ(v1, 5.0f, 5.0f, 5.0f);
    return 0;
}

int test_vec3_set_vec()
{
    vec_xyz v1(1.0f, 2.0f, 3.0f);
    vec_xyz v2;
    v2.set(v1);
    ASSERT_VEC3_EQ(v2, 1.0f, 2.0f, 3.0f);
    return 0;
}

int test_vec4_addition()
{
    vec_xyzw v1(1.0f, 2.0f, 3.0f, 4.0f);
    vec_xyzw v2(5.0f, 6.0f, 7.0f, 8.0f);
    vec_xyzw result = v1 + v2;
    ASSERT_VEC_EQ(result, 6.0f, 8.0f, 10.0f, 12.0f);
    return 0;
}

int test_vec4_subtraction()
{
    vec_xyzw v1(10.0f, 9.0f, 8.0f, 7.0f);
    vec_xyzw v2(1.0f, 2.0f, 3.0f, 4.0f);
    vec_xyzw result = v1 - v2;
    ASSERT_VEC_EQ(result, 9.0f, 7.0f, 5.0f, 3.0f);
    return 0;
}

int test_vec4_scalar_multiplication()
{
    vec_xyzw v1(1.0f, 2.0f, 3.0f, 4.0f);
    vec_xyzw result = v1 * 3.0f;
    ASSERT_VEC_EQ(result, 3.0f, 6.0f, 9.0f, 12.0f);
    return 0;
}

int test_mat33_set_zero()
{
    mat_33 m;
    m.set_zero();
    vec_xyz col0 = m.get_col0();
    vec_xyz col1 = m.get_col1();
    vec_xyz col2 = m.get_col2();
    ASSERT_VEC3_EQ(col0, 0.0f, 0.0f, 0.0f);
    ASSERT_VEC3_EQ(col1, 0.0f, 0.0f, 0.0f);
    ASSERT_VEC3_EQ(col2, 0.0f, 0.0f, 0.0f);
    return 0;
}

int test_mat33_set_row()
{
    mat_33 m;
    m.set_zero();
    vec_xyz row0(1.0f, 2.0f, 3.0f);
    m.set_row0(row0);
    vec_xyz col0 = m.get_col0();
    vec_xyz col1 = m.get_col1();
    vec_xyz col2 = m.get_col2();
    ASSERT_VEC3_EQ(col0, 1.0f, 0.0f, 0.0f);
    ASSERT_VEC3_EQ(col1, 2.0f, 0.0f, 0.0f);
    ASSERT_VEC3_EQ(col2, 3.0f, 0.0f, 0.0f);
    return 0;
}

int test_mat33_get_row()
{
    mat_33 m;
    m.set_zero();
    vec_xyz row0(1.0f, 2.0f, 3.0f);
    m.set_row0(row0);
    vec_xyz retrieved_row0 = m.get_row0();
    ASSERT_VEC3_EQ(retrieved_row0, 1.0f, 2.0f, 3.0f);
    return 0;
}

int test_mat33_multiply_vector()
{
    mat_33 m;
    m.set_zero();
    m.set_row0(vec_xyz(1.0f, 0.0f, 0.0f));
    m.set_row1(vec_xyz(0.0f, 2.0f, 0.0f));
    m.set_row2(vec_xyz(0.0f, 0.0f, 3.0f));
    
    vec_xyz v(1.0f, 2.0f, 3.0f);
    vec_xyz result = m * v;
    ASSERT_VEC3_EQ(result, 1.0f, 4.0f, 9.0f);
    return 0;
}

int test_mat44_set_zero()
{
    mat_44 m;
    m.set_zero();
    vec_xyzw col0 = m.get_col0();
    vec_xyzw col1 = m.get_col1();
    vec_xyzw col2 = m.get_col2();
    vec_xyzw col3 = m.get_col3();
    ASSERT_VEC_EQ(col0, 0.0f, 0.0f, 0.0f, 0.0f);
    ASSERT_VEC_EQ(col1, 0.0f, 0.0f, 0.0f, 0.0f);
    ASSERT_VEC_EQ(col2, 0.0f, 0.0f, 0.0f, 0.0f);
    ASSERT_VEC_EQ(col3, 0.0f, 0.0f, 0.0f, 0.0f);
    return 0;
}

int test_mat44_set_row()
{
    mat_44 m;
    m.set_zero();
    vec_xyzw row0(1.0f, 2.0f, 3.0f, 4.0f);
    m.set_row0(row0);
    vec_xyzw col0 = m.get_col0();
    vec_xyzw col1 = m.get_col1();
    vec_xyzw col2 = m.get_col2();
    vec_xyzw col3 = m.get_col3();
    ASSERT_VEC_EQ(col0, 1.0f, 0.0f, 0.0f, 0.0f);
    ASSERT_VEC_EQ(col1, 2.0f, 0.0f, 0.0f, 0.0f);
    ASSERT_VEC_EQ(col2, 3.0f, 0.0f, 0.0f, 0.0f);
    ASSERT_VEC_EQ(col3, 4.0f, 0.0f, 0.0f, 0.0f);
    return 0;
}

int test_mat44_multiply_vector()
{
    mat_44 m;
    m.set_zero();
    m.set_row0(vec_xyzw(1.0f, 0.0f, 0.0f, 0.0f));
    m.set_row1(vec_xyzw(0.0f, 2.0f, 0.0f, 0.0f));
    m.set_row2(vec_xyzw(0.0f, 0.0f, 3.0f, 0.0f));
    m.set_row3(vec_xyzw(0.0f, 0.0f, 0.0f, 4.0f));
    
    vec_xyzw v(1.0f, 2.0f, 3.0f, 4.0f);
    vec_xyzw result = m * v;
    ASSERT_VEC_EQ(result, 1.0f, 4.0f, 9.0f, 16.0f);
    return 0;
}

#ifndef USE_CPU_COMPAT
int test_accumulator_operations()
{
    vec_xyz v1(2.0f, 3.0f, 4.0f);
    vec_xyz v2(1.0f, 1.0f, 1.0f);
    
    // Test to_a
    v1.to_a();
    
    // Test mula
    v2.mula(v2);
    
    // Test aadd
    vec_xyz result = v1.aadd();
    // The accumulator should contain v1, so aadd should add v1 to itself
    // This is a basic test to ensure the accumulator functions compile and run
    return 0;
}
#endif // !USE_CPU_COMPAT

// Test normalized with zero vector (branch)
int test_vec3_normalized_zero()
{
    vec_xyz v(0.0f, 0.0f, 0.0f);
    vec_xyz result = v.normalized();
    ASSERT_VEC3_EQ(result, 0.0f, 0.0f, 0.0f);
    return 0;
}

// Test truncate_length branch (vector shorter than max)
int test_vec3_truncate_length_shorter()
{
    vec_xyz v(1.0f, 2.0f, 0.0f);  // Length = sqrt(5) ≈ 2.236
    vec_xyz result = v.truncate_length(5.0f);
    ASSERT_VEC3_EQ(result, 1.0f, 2.0f, 0.0f);  // Should return unchanged
    return 0;
}

// Test truncate_length branch (vector longer than max)
int test_vec3_truncate_length_longer()
{
    vec_xyz v(3.0f, 4.0f, 0.0f);  // Length = 5
    vec_xyz result = v.truncate_length(3.0f);  // Max length = 3
    float len = result.length();
    ASSERT_EQ(len, 3.0f);
    return 0;
}

// Test is_zero
int test_vec3_is_zero()
{
    vec_xyz v1(0.0f, 0.0f, 0.0f);
    vec_xyz v2(1.0f, 0.0f, 0.0f);
    int zero1 = v1.is_zero();
    int zero2 = v2.is_zero();
    if (zero1 != 1) return 1;
    if (zero2 != 0) return 1;
    return 0;
}

// Test sign function
int test_vec3_sign()
{
    vec_xyz v1(5.0f, -3.0f, 0.0f);
    vec_xyz result = v1.sign();
    float rx = (float)vec_x(result);
    float ry = (float)vec_y(result);
    float rz = (float)vec_z(result);
    if (rx < 0.9f || rx > 1.1f) return 1;  // Should be ~1.0
    if (ry > -0.9f || ry < -1.1f) return 1;  // Should be ~-1.0
    if (fabsf(rz) > 0.1f) return 1;  // Should be ~0.0
    return 0;
}

// Test one_over - commented out due to incompatible assembly constraints ("j" constraint)
int test_vec3_one_over()
{
    vec_xyz v(2.0f, 4.0f, 8.0f);
    vec_xyz result = v.one_over();
    float rx = (float)vec_x(result);
    float ry = (float)vec_y(result);
    float rz = (float)vec_z(result);
    ASSERT_EQ(rx, 0.5f);
    ASSERT_EQ(ry, 0.25f);
    ASSERT_EQ(rz, 0.125f);
    return 0;
}

// Test interpolate
int test_vec3_interpolate()
{
    vec_xyz v1(0.0f, 0.0f, 0.0f);
    vec_xyz v2(10.0f, 20.0f, 30.0f);
    vec_xyz result = v1.interpolate(0.5f, v2);
    ASSERT_VEC3_EQ(result, 5.0f, 10.0f, 15.0f);
    return 0;
}

// Test distance_from
int test_vec3_distance_from()
{
    vec_xyz v1(0.0f, 0.0f, 0.0f);
    vec_xyz v2(3.0f, 4.0f, 0.0f);
    float dist = (float)v1.distance_from(v2);
    ASSERT_EQ(dist, 5.0f);
    return 0;
}

// Test set_length
int test_vec3_set_length()
{
    vec_xyz v(3.0f, 4.0f, 0.0f);  // Length = 5
    vec_xyz result = v.set_length(10.0f);  // Set to length 10
    float len = result.length();
    ASSERT_EQ(len, 10.0f);
    ASSERT_VEC3_EQ(result, 6.0f, 8.0f, 0.0f);  // Scaled by 2
    return 0;
}

// Test parallel_component
int test_vec3_parallel_component()
{
    vec_xyz v(3.0f, 4.0f, 0.0f);
    vec_xyz basis(1.0f, 0.0f, 0.0f);  // Unit vector in x direction
    vec_xyz result = v.parallel_component(basis);
    ASSERT_VEC3_EQ(result, 3.0f, 0.0f, 0.0f);
    return 0;
}

// Test perpendicular_component
int test_vec3_perpendicular_component()
{
    vec_xyz v(3.0f, 4.0f, 0.0f);
    vec_xyz basis(1.0f, 0.0f, 0.0f);  // Unit vector in x direction
    vec_xyz result = v.perpendicular_component(basis);
    ASSERT_VEC3_EQ(result, 0.0f, 4.0f, 0.0f);
    return 0;
}

// Test normalize (in-place)
int test_vec3_normalize()
{
    vec_xyz v(3.0f, 4.0f, 0.0f);
    v.normalize();
    float len = v.length();
    ASSERT_EQ(len, 1.0f);
    ASSERT_VEC3_EQ(v, 0.6f, 0.8f, 0.0f);
    return 0;
}

#ifndef USE_CPU_COMPAT
// Test accumulator: to_a and from_a
int test_vec3_to_a_from_a()
{
    vec_xyz v1(2.0f, 3.0f, 4.0f);
    v1.to_a();  // ACC = v1
    vec_xyz v2(0.0f, 0.0f, 0.0f);
    v2.from_a();  // v2 = ACC
    ASSERT_VEC3_EQ(v2, 2.0f, 3.0f, 4.0f);
    return 0;
}

// Test accumulator: aadd
int test_vec3_aadd()
{
    vec_xyz v1(2.0f, 3.0f, 4.0f);
    vec_xyz v2(1.0f, 1.0f, 1.0f);
    v1.to_a();  // ACC = v1
    vec_xyz result = v2.aadd();  // result = ACC + v2 = v1 + v2
    ASSERT_VEC3_EQ(result, 3.0f, 4.0f, 5.0f);
    return 0;
}

// Test accumulator: asub
int test_vec3_asub()
{
    vec_xyz v1(5.0f, 5.0f, 5.0f);
    vec_xyz v2(2.0f, 3.0f, 4.0f);
    v1.to_a();  // ACC = v1
    vec_xyz result = v2.asub();  // result = ACC - v2 = v1 - v2
    ASSERT_VEC3_EQ(result, 3.0f, 2.0f, 1.0f);
    return 0;
}

// Test accumulator: madd
int test_vec3_madd()
{
    vec_xyz v1(2.0f, 3.0f, 4.0f);
    vec_xyz v2(1.0f, 1.0f, 1.0f);
    vec_xyz v3(1.0f, 1.0f, 1.0f);
    v1.to_a();  // ACC = v1
    vec_xyz result = v2.madd(v3);  // result = ACC + v2 * v3 = v1 + v2 * v3
    ASSERT_VEC3_EQ(result, 3.0f, 4.0f, 5.0f);
    return 0;
}

// Test accumulator: msub
int test_vec3_msub()
{
    vec_xyz v1(5.0f, 5.0f, 5.0f);
    vec_xyz v2(2.0f, 2.0f, 2.0f);
    vec_xyz v3(1.0f, 1.0f, 1.0f);
    v1.to_a();  // ACC = v1
    vec_xyz result = v2.msub(v3);  // result = ACC - v2 * v3 = v1 - v2 * v3
    ASSERT_VEC3_EQ(result, 3.0f, 3.0f, 3.0f);
    return 0;
}

// Test accumulator: madda
int test_vec3_madda()
{
    vec_xyz v1(2.0f, 3.0f, 4.0f);
    vec_xyz v2(1.0f, 1.0f, 1.0f);
    v1.to_a();  // ACC = v1
    v2.madda(v2);  // ACC = ACC + v2 * v2 = v1 + v2 * v2
    vec_xyz result;
    result.from_a();  // result = ACC
    ASSERT_VEC3_EQ(result, 3.0f, 4.0f, 5.0f);
    return 0;
}

// Test accumulator: msuba
int test_vec3_msuba()
{
    vec_xyz v1(5.0f, 5.0f, 5.0f);
    vec_xyz v2(2.0f, 2.0f, 2.0f);
    v1.to_a();  // ACC = v1
    v2.msuba(v2);  // ACC = ACC - v2 * v2 = v1 - v2 * v2
    vec_xyz result;
    result.from_a();  // result = ACC
    ASSERT_VEC3_EQ(result, 1.0f, 1.0f, 1.0f);
    return 0;
}

// Test accumulator: aadda - COMMENTED OUT: failing test
int test_vec3_aadda()
{
    vec_xyz v1(2.0f, 3.0f, 4.0f);
    vec_xyz v2(1.0f, 1.0f, 1.0f);
    v1.to_a();  // ACC = v1
    vec_xyz result = v2.aadda();  // result = ACC + v2, ACC = ACC + v2
    ASSERT_VEC3_EQ(result, 3.0f, 4.0f, 5.0f);
    return 0;
}

// Test accumulator: asuba
int test_vec3_asuba()
{
    vec_xyz v1(5.0f, 5.0f, 5.0f);
    vec_xyz v2(2.0f, 3.0f, 4.0f);
    v1.to_a();  // ACC = v1
    vec_xyz result = v2.asuba();  // result = ACC - v2, ACC = ACC - v2
    ASSERT_VEC3_EQ(result, 3.0f, 2.0f, 1.0f);
    return 0;
}
#endif // !USE_CPU_COMPAT

int test_vec3_operator_plus_equals()
{
    vec_xyz v1(1.0f, 2.0f, 3.0f);
    vec_xyz v2(4.0f, 5.0f, 6.0f);
    v1 += v2;
    ASSERT_VEC3_EQ(v1, 5.0f, 7.0f, 9.0f);
    return 0;
}

int test_vec3_operator_minus_equals()
{
    vec_xyz v1(10.0f, 9.0f, 8.0f);
    vec_xyz v2(1.0f, 2.0f, 3.0f);
    v1 -= v2;
    ASSERT_VEC3_EQ(v1, 9.0f, 7.0f, 5.0f);
    return 0;
}

int test_vec3_operator_multiply_equals()
{
    vec_xyz v1(2.0f, 3.0f, 4.0f);
    vec_xyz v2(2.0f, 2.0f, 2.0f);
    v1 *= v2;
    ASSERT_VEC3_EQ(v1, 4.0f, 6.0f, 8.0f);
    return 0;
}

int test_vec3_dot_product()
{
    vec_xyz v1(1.0f, 2.0f, 3.0f);
    vec_xyz v2(4.0f, 5.0f, 6.0f);
    float dot = v1.dot(v2);
    ASSERT_EQ(dot, 32.0f);  // 1*4 + 2*5 + 3*6 = 4 + 10 + 18 = 32
    return 0;
}

int test_vec3_length()
{
    vec_xyz v(3.0f, 4.0f, 0.0f);
    float len = v.length();
    ASSERT_EQ(len, 5.0f);
    return 0;
}

int test_vec3_length_sqr()
{
    vec_xyz v(3.0f, 4.0f, 0.0f);
    float len_sqr = v.length_sqr();
    ASSERT_EQ(len_sqr, 25.0f);
    return 0;
}

int test_vec3_cross_product()
{
    vec_xyz v1(1.0f, 0.0f, 0.0f);
    vec_xyz v2(0.0f, 1.0f, 0.0f);
    vec_xyz result = v1.cross(v2);
    ASSERT_VEC3_EQ(result, 0.0f, 0.0f, 1.0f);
    return 0;
}

// Matrix tests
int test_mat33_set_identity()
{
    mat_33 m;
    m.set_identity();
    vec_xyz col0 = m.get_col0();
    vec_xyz col1 = m.get_col1();
    vec_xyz col2 = m.get_col2();
    ASSERT_VEC3_EQ(col0, 1.0f, 0.0f, 0.0f);
    ASSERT_VEC3_EQ(col1, 0.0f, 1.0f, 0.0f);
    ASSERT_VEC3_EQ(col2, 0.0f, 0.0f, 1.0f);
    return 0;
}

int test_mat33_set_scale()
{
    mat_33 m;
    vec_xyz scale(2.0f, 3.0f, 4.0f);
    m.set_scale(scale);
    vec_xyz col0 = m.get_col0();
    vec_xyz col1 = m.get_col1();
    vec_xyz col2 = m.get_col2();
    ASSERT_VEC3_EQ(col0, 2.0f, 0.0f, 0.0f);
    ASSERT_VEC3_EQ(col1, 0.0f, 3.0f, 0.0f);
    ASSERT_VEC3_EQ(col2, 0.0f, 0.0f, 4.0f);
    return 0;
}

int test_mat33_transpose()
{
    mat_33 m;
    m.set_row0(vec_xyz(1.0f, 2.0f, 3.0f));
    m.set_row1(vec_xyz(4.0f, 5.0f, 6.0f));
    m.set_row2(vec_xyz(7.0f, 8.0f, 9.0f));
    mat_33 transposed = m.transpose();
    vec_xyz row0 = transposed.get_row0();
    vec_xyz row1 = transposed.get_row1();
    vec_xyz row2 = transposed.get_row2();
    ASSERT_VEC3_EQ(row0, 1.0f, 4.0f, 7.0f);
    ASSERT_VEC3_EQ(row1, 2.0f, 5.0f, 8.0f);
    ASSERT_VEC3_EQ(row2, 3.0f, 6.0f, 9.0f);
    return 0;
}

int test_mat33_get_row1()
{
    mat_33 m;
    m.set_zero();
    vec_xyz row1(10.0f, 20.0f, 30.0f);
    m.set_row1(row1);
    vec_xyz retrieved_row1 = m.get_row1();
    ASSERT_VEC3_EQ(retrieved_row1, 10.0f, 20.0f, 30.0f);
    return 0;
}

int test_mat33_get_row2()
{
    mat_33 m;
    m.set_zero();
    vec_xyz row2(100.0f, 200.0f, 300.0f);
    m.set_row2(row2);
    vec_xyz retrieved_row2 = m.get_row2();
    ASSERT_VEC3_EQ(retrieved_row2, 100.0f, 200.0f, 300.0f);
    return 0;
}

int test_mat44_set_identity()
{
    mat_44 m;
    m.set_identity();
    vec_xyzw col0 = m.get_col0();
    vec_xyzw col1 = m.get_col1();
    vec_xyzw col2 = m.get_col2();
    vec_xyzw col3 = m.get_col3();
    ASSERT_VEC_EQ(col0, 1.0f, 0.0f, 0.0f, 0.0f);
    ASSERT_VEC_EQ(col1, 0.0f, 1.0f, 0.0f, 0.0f);
    ASSERT_VEC_EQ(col2, 0.0f, 0.0f, 1.0f, 0.0f);
    ASSERT_VEC_EQ(col3, 0.0f, 0.0f, 0.0f, 1.0f);
    return 0;
}

int test_mat44_set_scale()
{
    mat_44 m;
    vec_xyz scale(2.0f, 3.0f, 4.0f);
    m.set_scale(scale);
    vec_xyzw col0 = m.get_col0();
    vec_xyzw col1 = m.get_col1();
    vec_xyzw col2 = m.get_col2();
    vec_xyzw col3 = m.get_col3();
    ASSERT_VEC_EQ(col0, 2.0f, 0.0f, 0.0f, 0.0f);
    ASSERT_VEC_EQ(col1, 0.0f, 3.0f, 0.0f, 0.0f);
    ASSERT_VEC_EQ(col2, 0.0f, 0.0f, 4.0f, 0.0f);
    ASSERT_VEC_EQ(col3, 0.0f, 0.0f, 0.0f, 1.0f);
    return 0;
}

int test_mat44_set_translate()
{
    mat_44 m;
    vec_xyz trans(10.0f, 20.0f, 30.0f);
    m.set_translate(trans);
    vec_xyzw col3 = m.get_col3();
    ASSERT_VEC_EQ(col3, 10.0f, 20.0f, 30.0f, 1.0f);
    return 0;
}

int test_mat44_get_row1()
{
    mat_44 m;
    m.set_zero();
    vec_xyzw row1(10.0f, 20.0f, 30.0f, 40.0f);
    m.set_row1(row1);
    vec_xyzw retrieved_row1 = m.get_row1();
    ASSERT_VEC_EQ(retrieved_row1, 10.0f, 20.0f, 30.0f, 40.0f);
    return 0;
}

int test_mat44_get_row2()
{
    mat_44 m;
    m.set_zero();
    vec_xyzw row2(100.0f, 200.0f, 300.0f, 400.0f);
    m.set_row2(row2);
    vec_xyzw retrieved_row2 = m.get_row2();
    ASSERT_VEC_EQ(retrieved_row2, 100.0f, 200.0f, 300.0f, 400.0f);
    return 0;
}

int test_mat44_get_row3()
{
    mat_44 m;
    m.set_zero();
    vec_xyzw row3(1000.0f, 2000.0f, 3000.0f, 4000.0f);
    m.set_row3(row3);
    vec_xyzw retrieved_row3 = m.get_row3();
    ASSERT_VEC_EQ(retrieved_row3, 1000.0f, 2000.0f, 3000.0f, 4000.0f);
    return 0;
}

int test_mat44_transpose()
{
    mat_44 m;
    m.set_zero();  // Initialize matrix first
    m.set_row0(vec_xyzw(1.0f, 2.0f, 3.0f, 4.0f));
    m.set_row1(vec_xyzw(5.0f, 6.0f, 7.0f, 8.0f));
    m.set_row2(vec_xyzw(9.0f, 10.0f, 11.0f, 12.0f));
    m.set_row3(vec_xyzw(13.0f, 14.0f, 15.0f, 16.0f));
    mat_44 transposed = m.transpose();
    vec_xyzw row0 = transposed.get_row0();
    vec_xyzw row1 = transposed.get_row1();
    vec_xyzw row2 = transposed.get_row2();
    vec_xyzw row3 = transposed.get_row3();
    ASSERT_VEC_EQ(row0, 1.0f, 5.0f, 9.0f, 13.0f);
    ASSERT_VEC_EQ(row1, 2.0f, 6.0f, 10.0f, 14.0f);
    ASSERT_VEC_EQ(row2, 3.0f, 7.0f, 11.0f, 15.0f);
    ASSERT_VEC_EQ(row3, 4.0f, 8.0f, 12.0f, 16.0f);
    return 0;
}

// Test vec_xyzw normalized3 and normalize3
int test_vec4_normalized3()
{
    vec_xyzw v(3.0f, 4.0f, 0.0f, 0.0f);  // Length = 5
    vec_xyzw result = v.normalized3();
    float rx = (float)vec_x(result);
    float ry = (float)vec_y(result);
    float rz = (float)vec_z(result);
    float len = sqrtf(rx * rx + ry * ry + rz * rz);
    ASSERT_EQ(len, 1.0f);
    return 0;
}

#ifndef USE_CPU_COMPAT
// Test VU0 copy functions
int test_vu0_copy_qwords()
{
    // Test CopyQwordsToVU0 and CopyQwordsFromVU0
    uint128_t src[4];
    uint128_t dst[4];
    
    // Initialize source data
    memset(src, 0, sizeof(src));
    memset(dst, 0, sizeof(dst));
    
    // Set up test data (simple pattern)
    uint32_t* src_ptr = (uint32_t*)src;
    for (int i = 0; i < 16; i++) {
        src_ptr[i] = i + 1;
    }
    
    // Copy to VU0
    VU0::CopyQwordsToVU0(0, src, 4);
    
    // Copy back from VU0
    VU0::CopyQwordsFromVU0(dst, 0, 4);
    
    // Verify data matches
    for (int i = 0; i < 16; i++) {
        if (src_ptr[i] != ((uint32_t*)dst)[i]) {
            return 1;
        }
    }
    
    return 0;
}

int test_vu0_copy_even_qwords()
{
    // Test CopyEvenQwordsToVU0 and CopyEvenQwordsFromVU0
    uint128_t src[4];
    uint128_t dst[4];
    
    // Initialize source data
    memset(src, 0, sizeof(src));
    memset(dst, 0, sizeof(dst));
    
    // Set up test data
    uint32_t* src_ptr = (uint32_t*)src;
    for (int i = 0; i < 16; i++) {
        src_ptr[i] = i + 100;
    }
    
    // Copy even qwords to VU0 (must be even number)
    VU0::CopyEvenQwordsToVU0(0, src, 4);
    
    // Copy back from VU0
    VU0::CopyEvenQwordsFromVU0(dst, 0, 4);
    
    // Verify data matches
    for (int i = 0; i < 16; i++) {
        if (src_ptr[i] != ((uint32_t*)dst)[i]) {
            return 1;
        }
    }
    
    return 0;
}
#endif // !USE_CPU_COMPAT

int test_vec4_normalize3()
{
    vec_xyzw v(3.0f, 4.0f, 0.0f, 5.0f);
    v.normalize3();
    float rx = (float)vec_x(v);
    float ry = (float)vec_y(v);
    float rz = (float)vec_z(v);
    float len = sqrtf(rx * rx + ry * ry + rz * rz);
    ASSERT_EQ(len, 1.0f);
    return 0;
}

// ---- mat_43 tests ----

int test_mat43_multiply_vector()
{
    // mat_43 * vec_3 = vec_4  (column-major: result = col0*v.x + col1*v.y + col2*v.z)
    mat_43 m(vec_4(2.0f, 1.0f, 0.0f, 3.0f),
             vec_4(0.0f, 3.0f, 1.0f, 2.0f),
             vec_4(1.0f, 0.0f, 4.0f, 1.0f));
    vec_3 v(1.0f, 2.0f, 3.0f);
    vec_4 r = m * v;
    // r = col0*1 + col1*2 + col2*3
    //   = (2,1,0,3) + (0,6,2,4) + (3,0,12,3) = (5,7,14,10)
    ASSERT_VEC_EQ(r, 5.0f, 7.0f, 14.0f, 10.0f);
    return 0;
}

// ---- mat_34 tests ----

int test_mat34_multiply_vector()
{
    // mat_34 * vec_4 = vec_3  (result = col0*v.x + col1*v.y + col2*v.z + col3*v.w)
    mat_34 m(vec_3(1.0f, 0.0f, 0.0f),
             vec_3(0.0f, 1.0f, 0.0f),
             vec_3(0.0f, 0.0f, 1.0f),
             vec_3(5.0f, 6.0f, 7.0f));
    vec_4 v(1.0f, 2.0f, 3.0f, 1.0f);
    vec_3 r = m * v;
    // r = (1,0,0)*1 + (0,1,0)*2 + (0,0,1)*3 + (5,6,7)*1 = (6,8,10)
    ASSERT_VEC3_EQ(r, 6.0f, 8.0f, 10.0f);
    return 0;
}

// ---- mat_33 matrix * matrix ----

int test_mat33_multiply_mat33()
{
    // A * I = A
    mat_33 A;
    A.col0.set(1.0f, 2.0f, 3.0f);
    A.col1.set(4.0f, 5.0f, 6.0f);
    A.col2.set(7.0f, 8.0f, 9.0f);
    mat_33 I;
    I.set_identity();
    mat_33 R = A * I;
    // R should equal A
    ASSERT_VEC3_EQ(R.get_row0(), 1.0f, 4.0f, 7.0f);
    ASSERT_VEC3_EQ(R.get_row1(), 2.0f, 5.0f, 8.0f);
    ASSERT_VEC3_EQ(R.get_row2(), 3.0f, 6.0f, 9.0f);
    return 0;
}

int test_mat33_multiply_mat33_nontrivial()
{
    // A * B where neither is identity
    mat_33 A, B;
    A.col0.set(1.0f, 2.0f, 3.0f);
    A.col1.set(4.0f, 5.0f, 6.0f);
    A.col2.set(7.0f, 8.0f, 9.0f);
    B.col0.set(9.0f, 8.0f, 7.0f);
    B.col1.set(6.0f, 5.0f, 4.0f);
    B.col2.set(3.0f, 2.0f, 1.0f);
    mat_33 C = A * B;
    // C_col0 = A * B_col0 = A*(9,8,7) = col0*9+col1*8+col2*7
    //        = (1,2,3)*9+(4,5,6)*8+(7,8,9)*7 = (9+32+49, 18+40+56, 27+48+63) = (90,114,138)
    // C_col1 = A*(6,5,4) = (1,2,3)*6+(4,5,6)*5+(7,8,9)*4 = (6+20+28, 12+25+32, 18+30+36) = (54,69,84)
    // C_col2 = A*(3,2,1) = (1,2,3)*3+(4,5,6)*2+(7,8,9)*1 = (3+8+7, 6+10+8, 9+12+9) = (18,24,30)
    ASSERT_VEC3_EQ(C.get_row0(), 90.0f, 54.0f, 18.0f);
    ASSERT_VEC3_EQ(C.get_row1(), 114.0f, 69.0f, 24.0f);
    ASSERT_VEC3_EQ(C.get_row2(), 138.0f, 84.0f, 30.0f);
    return 0;
}

// ---- mat_44 matrix * matrix ----

int test_mat44_multiply_mat44()
{
    // A * I = A
    mat_44 A;
    A.col0.set(1.0f, 0.0f, 0.0f, 0.0f);
    A.col1.set(0.0f, 2.0f, 0.0f, 0.0f);
    A.col2.set(0.0f, 0.0f, 3.0f, 0.0f);
    A.col3.set(4.0f, 5.0f, 6.0f, 1.0f);
    mat_44 I;
    I.set_identity();
    mat_44 R = A * I;
    ASSERT_VEC_EQ(R.get_row0(), 1.0f, 0.0f, 0.0f, 4.0f);
    ASSERT_VEC_EQ(R.get_row1(), 0.0f, 2.0f, 0.0f, 5.0f);
    ASSERT_VEC_EQ(R.get_row2(), 0.0f, 0.0f, 3.0f, 6.0f);
    ASSERT_VEC_EQ(R.get_row3(), 0.0f, 0.0f, 0.0f, 1.0f);
    return 0;
}

// ---- matrix negate ----

int test_mat33_negate()
{
    mat_33 m;
    m.col0.set(1.0f, 2.0f, 3.0f);
    m.col1.set(4.0f, 5.0f, 6.0f);
    m.col2.set(7.0f, 8.0f, 9.0f);
    mat_33 n = -m;
    ASSERT_VEC3_EQ(n.get_row0(), -1.0f, -4.0f, -7.0f);
    ASSERT_VEC3_EQ(n.get_row1(), -2.0f, -5.0f, -8.0f);
    ASSERT_VEC3_EQ(n.get_row2(), -3.0f, -6.0f, -9.0f);
    return 0;
}

int test_mat44_negate()
{
    mat_44 M;
    M.set_identity();
    M.col3.set(1.0f, 2.0f, 3.0f, 4.0f);
    mat_44 N = -M;
    ASSERT_VEC_EQ(N.get_row0(), -1.0f,  0.0f,  0.0f, -1.0f);
    ASSERT_VEC_EQ(N.get_row1(),  0.0f, -1.0f,  0.0f, -2.0f);
    ASSERT_VEC_EQ(N.get_row2(),  0.0f,  0.0f, -1.0f, -3.0f);
    ASSERT_VEC_EQ(N.get_row3(),  0.0f,  0.0f,  0.0f, -4.0f);
    return 0;
}

// ---- mat_43 basic construction and row access ----

int test_mat43_set_zero()
{
    mat_43 m;
    m.set_zero();
    ASSERT_VEC3_EQ(m.get_row0(), 0.0f, 0.0f, 0.0f);
    ASSERT_VEC3_EQ(m.get_row1(), 0.0f, 0.0f, 0.0f);
    ASSERT_VEC3_EQ(m.get_row2(), 0.0f, 0.0f, 0.0f);
    ASSERT_VEC3_EQ(m.get_row3(), 0.0f, 0.0f, 0.0f);
    return 0;
}

int test_mat43_get_rows()
{
    mat_43 m(vec_4(1.0f, 2.0f, 3.0f, 4.0f),
             vec_4(5.0f, 6.0f, 7.0f, 8.0f),
             vec_4(9.0f, 10.0f, 11.0f, 12.0f));
    // row0 = (col0.x, col1.x, col2.x) = (1,5,9)
    ASSERT_VEC3_EQ(m.get_row0(), 1.0f, 5.0f,  9.0f);
    // row1 = (col0.y, col1.y, col2.y) = (2,6,10)
    ASSERT_VEC3_EQ(m.get_row1(), 2.0f, 6.0f, 10.0f);
    // row2 = (col0.z, col1.z, col2.z) = (3,7,11)
    ASSERT_VEC3_EQ(m.get_row2(), 3.0f, 7.0f, 11.0f);
    // row3 = (col0.w, col1.w, col2.w) = (4,8,12)
    ASSERT_VEC3_EQ(m.get_row3(), 4.0f, 8.0f, 12.0f);
    return 0;
}

// ---- mat_34 basic construction and row access ----

int test_mat34_set_zero()
{
    mat_34 m;
    m.set_zero();
    ASSERT_VEC_EQ(m.get_row0(), 0.0f, 0.0f, 0.0f, 0.0f);
    ASSERT_VEC_EQ(m.get_row1(), 0.0f, 0.0f, 0.0f, 0.0f);
    ASSERT_VEC_EQ(m.get_row2(), 0.0f, 0.0f, 0.0f, 0.0f);
    return 0;
}

int test_mat34_get_rows()
{
    mat_34 m(vec_3(1.0f, 2.0f, 3.0f),
             vec_3(4.0f, 5.0f, 6.0f),
             vec_3(7.0f, 8.0f, 9.0f),
             vec_3(10.0f, 11.0f, 12.0f));
    // row0 = (col0.x, col1.x, col2.x, col3.x) = (1,4,7,10)
    ASSERT_VEC_EQ(m.get_row0(), 1.0f,  4.0f,  7.0f, 10.0f);
    // row1 = (col0.y, col1.y, col2.y, col3.y) = (2,5,8,11)
    ASSERT_VEC_EQ(m.get_row1(), 2.0f,  5.0f,  8.0f, 11.0f);
    // row2 = (col0.z, col1.z, col2.z, col3.z) = (3,6,9,12)
    ASSERT_VEC_EQ(m.get_row2(), 3.0f,  6.0f,  9.0f, 12.0f);
    return 0;
}

// ---- mat_43 transpose ----

int test_mat43_transpose()
{
    // transpose of mat_43 should equal mat_34
    mat_43 m(vec_4(1.0f, 2.0f, 3.0f, 4.0f),
             vec_4(5.0f, 6.0f, 7.0f, 8.0f),
             vec_4(9.0f, 10.0f, 11.0f, 12.0f));
    mat_34 t = m.transpose();
    // t.col_i should be the i-th row of m
    // t.col0 (vec_3) = row0 of m = (1,5,9)
    // t.col1 = row1 = (2,6,10), t.col2 = row2 = (3,7,11), t.col3 = row3 = (4,8,12)
    ASSERT_VEC_EQ(t.get_row0(), 1.0f, 2.0f,  3.0f,  4.0f);
    ASSERT_VEC_EQ(t.get_row1(), 5.0f, 6.0f,  7.0f,  8.0f);
    ASSERT_VEC_EQ(t.get_row2(), 9.0f, 10.0f, 11.0f, 12.0f);
    return 0;
}

// ---- mat_34 transpose ----

int test_mat34_transpose()
{
    mat_34 m(vec_3(1.0f, 2.0f, 3.0f),
             vec_3(4.0f, 5.0f, 6.0f),
             vec_3(7.0f, 8.0f, 9.0f),
             vec_3(10.0f, 11.0f, 12.0f));
    mat_43 t = m.transpose();
    // t row0 = col0 of original m viewed as row = (1,4,7,10)
    // But t is mat_43 (4 rows, 3 cols after transpose)
    // t.get_row0() = (col0.x_of_t, col1.x_of_t, col2.x_of_t)
    // where col_i of t = row_i of m
    // row0 of m = (1,4,7,10) → col0 of t = vec_4(1,4,7,10)
    // row1 of m = (2,5,8,11) → col1 of t
    // row2 of m = (3,6,9,12) → col2 of t
    // So t.get_row0() = (col0.x, col1.x, col2.x) = (1, 2, 3)
    ASSERT_VEC3_EQ(t.get_row0(), 1.0f, 2.0f,  3.0f);
    ASSERT_VEC3_EQ(t.get_row1(), 4.0f, 5.0f,  6.0f);
    ASSERT_VEC3_EQ(t.get_row2(), 7.0f, 8.0f,  9.0f);
    ASSERT_VEC3_EQ(t.get_row3(), 10.0f, 11.0f, 12.0f);
    return 0;
}

// ---- mat_44 rotation ----

// ---- mat_43 trans_mult tests ----

int test_mat43_trans_mult_vec4()
{
    // mat_43 with cols that have non-zero .w component
    mat_43 m(vec_4(1.0f, 0.0f, 0.0f, 2.0f),
             vec_4(0.0f, 1.0f, 0.0f, 3.0f),
             vec_4(0.0f, 0.0f, 1.0f, 4.0f));
    vec_4 v(1.0f, 1.0f, 1.0f, 1.0f);
    vec_3 r = m.trans_mult(v);
    // result.i = dot4(col_i, v)
    // result.x = 1+0+0+2 = 3
    // result.y = 0+1+0+3 = 4
    // result.z = 0+0+1+4 = 5
    ASSERT_VEC3_EQ(r, 3.0f, 4.0f, 5.0f);
    return 0;
}

int test_mat43_trans_mult_vector_t()
{
    // Identity-like mat_43 (col.w = 0)
    mat_43 m(vec_4(1.0f, 0.0f, 0.0f, 0.0f),
             vec_4(0.0f, 1.0f, 0.0f, 0.0f),
             vec_4(0.0f, 0.0f, 1.0f, 0.0f));
    vector_t v(2.0f, 3.0f, 4.0f);
    vec_3 r = m.trans_mult(v);
    // result.i = dot3(col_i.xyz, v.xyz)
    ASSERT_VEC3_EQ(r, 2.0f, 3.0f, 4.0f);
    return 0;
}

int test_mat43_trans_mult_point_t()
{
    // mat_43 with translation in .w
    mat_43 m(vec_4(1.0f, 0.0f, 0.0f, 5.0f),
             vec_4(0.0f, 1.0f, 0.0f, 6.0f),
             vec_4(0.0f, 0.0f, 1.0f, 7.0f));
    point_t p(1.0f, 2.0f, 3.0f);
    vec_3 r = m.trans_mult(p);
    // result.i = dot3(col_i.xyz, p.xyz) + col_i.w
    // result.x = 1 + 5 = 6
    // result.y = 2 + 6 = 8
    // result.z = 3 + 7 = 10
    ASSERT_VEC3_EQ(r, 6.0f, 8.0f, 10.0f);
    return 0;
}

// ---- mat_34 trans_mult ----

int test_mat34_trans_mult_vec3()
{
    // mat_34: identity 3x3 block + col3=(5,6,7)
    mat_34 m(vec_3(1.0f, 0.0f, 0.0f),
             vec_3(0.0f, 1.0f, 0.0f),
             vec_3(0.0f, 0.0f, 1.0f),
             vec_3(5.0f, 6.0f, 7.0f));
    vec_3 v(1.0f, 2.0f, 3.0f);
    vec_4 r = m.trans_mult(v);
    // result.i = dot3(col_i, v)
    // result.x = 1, result.y = 2, result.z = 3, result.w = 5+12+21 = 38
    ASSERT_VEC_EQ(r, 1.0f, 2.0f, 3.0f, 38.0f);
    return 0;
}

// ---- row-vector * matrix tests ----

int test_vec3_multiply_mat33()
{
    // mat_33 that swaps x and z components
    mat_33 m(vec_3(0.0f, 0.0f, 1.0f),
             vec_3(0.0f, 1.0f, 0.0f),
             vec_3(1.0f, 0.0f, 0.0f));
    vec_xyz v(1.0f, 2.0f, 3.0f);
    vec_xyz r = v * m;
    // result.i = dot3(v, col_i)
    // result.x = dot3((1,2,3),(0,0,1)) = 3
    // result.y = dot3((1,2,3),(0,1,0)) = 2
    // result.z = dot3((1,2,3),(1,0,0)) = 1
    ASSERT_VEC3_EQ(r, 3.0f, 2.0f, 1.0f);
    return 0;
}

int test_vec3_multiply_mat34()
{
    // mat_34 identity block + col3=(4,5,6)
    mat_34 m(vec_3(1.0f, 0.0f, 0.0f),
             vec_3(0.0f, 1.0f, 0.0f),
             vec_3(0.0f, 0.0f, 1.0f),
             vec_3(4.0f, 5.0f, 6.0f));
    vec_xyz v(1.0f, 2.0f, 3.0f);
    vec_xyzw r = v * m;
    // result.i = dot3(v, col_i)
    // result.x = 1, result.y = 2, result.z = 3, result.w = 4+10+18 = 32
    ASSERT_VEC_EQ(r, 1.0f, 2.0f, 3.0f, 32.0f);
    return 0;
}

int test_vec4_multiply_mat43()
{
    // Identity mat_43
    mat_43 m(vec_4(1.0f, 0.0f, 0.0f, 0.0f),
             vec_4(0.0f, 1.0f, 0.0f, 0.0f),
             vec_4(0.0f, 0.0f, 1.0f, 0.0f));
    vec_xyzw v(2.0f, 3.0f, 4.0f, 5.0f);
    vec_xyz r = v * m;
    // result.i = dot4(v, col_i); w component of identity cols is 0
    ASSERT_VEC3_EQ(r, 2.0f, 3.0f, 4.0f);
    return 0;
}

int test_vec4_multiply_mat44()
{
    // Diagonal scale mat_44 (2,3,4,1)
    mat_44 m(vec_4(2.0f, 0.0f, 0.0f, 0.0f),
             vec_4(0.0f, 3.0f, 0.0f, 0.0f),
             vec_4(0.0f, 0.0f, 4.0f, 0.0f),
             vec_4(0.0f, 0.0f, 0.0f, 1.0f));
    vec_xyzw v(1.0f, 1.0f, 1.0f, 0.0f);
    vec_xyzw r = v * m;
    ASSERT_VEC_EQ(r, 2.0f, 3.0f, 4.0f, 0.0f);
    return 0;
}

// ---- mat_44 trans_mult tests ----

int test_mat44_trans_mult_vec4()
{
    // Diagonal scale mat_44 (2,3,4,1)
    mat_44 m(vec_4(2.0f, 0.0f, 0.0f, 0.0f),
             vec_4(0.0f, 3.0f, 0.0f, 0.0f),
             vec_4(0.0f, 0.0f, 4.0f, 0.0f),
             vec_4(0.0f, 0.0f, 0.0f, 1.0f));
    vec_4 v(1.0f, 2.0f, 3.0f, 4.0f);
    vec_4 r = m.trans_mult(v);
    // result.i = dot4(col_i, v)
    // result.x = 2*1 = 2, result.y = 3*2 = 6, result.z = 4*3 = 12, result.w = 1*4 = 4
    ASSERT_VEC_EQ(r, 2.0f, 6.0f, 12.0f, 4.0f);
    return 0;
}

int test_mat44_trans_mult_vector_t()
{
    // Diagonal scale + col3=(10,20,30,1)
    mat_44 m(vec_4(2.0f,  0.0f,  0.0f, 0.0f),
             vec_4(0.0f,  3.0f,  0.0f, 0.0f),
             vec_4(0.0f,  0.0f,  4.0f, 0.0f),
             vec_4(10.0f, 20.0f, 30.0f, 1.0f));
    vector_t v(1.0f, 1.0f, 1.0f);
    vec_4 r = m.trans_mult(v);
    // result.i = dot3(col_i.xyz, v.xyz)
    // result.x = 2, result.y = 3, result.z = 4, result.w = 10+20+30 = 60
    ASSERT_VEC_EQ(r, 2.0f, 3.0f, 4.0f, 60.0f);
    return 0;
}

int test_mat44_trans_mult_point_t()
{
    // Identity + col.w translation (5,6,7,0)
    mat_44 m(vec_4(1.0f, 0.0f, 0.0f, 5.0f),
             vec_4(0.0f, 1.0f, 0.0f, 6.0f),
             vec_4(0.0f, 0.0f, 1.0f, 7.0f),
             vec_4(0.0f, 0.0f, 0.0f, 1.0f));
    point_t p(1.0f, 2.0f, 3.0f);
    vec_4 r = m.trans_mult(p);
    // result.i = dot4(col_i, p) where p.w=1
    // result.x = 1+5 = 6, result.y = 2+6 = 8, result.z = 3+7 = 10, result.w = 1
    ASSERT_VEC_EQ(r, 6.0f, 8.0f, 10.0f, 1.0f);
    return 0;
}

int test_mat44_set_rotate_x()
{
    // 90-degree rotation around X: y→z, z→-y
    mat_44 M;
    M.set_rotate_x(3.14159265358979f / 2.0f);
    // M * (1,0,0,0) = (1,0,0,0) (x-axis unchanged)
    vec_4 ax(1.0f, 0.0f, 0.0f, 0.0f);
    vec_4 rx = M * ax;
    ASSERT_VEC_EQ(rx, 1.0f, 0.0f, 0.0f, 0.0f);
    // M * (0,1,0,0) = (0,0,1,0) (y→z)
    vec_4 ay(0.0f, 1.0f, 0.0f, 0.0f);
    vec_4 ry = M * ay;
    ASSERT_VEC_EQ(ry, 0.0f, 0.0f, 1.0f, 0.0f);
    return 0;
}

int test_mat44_set_rotate_z()
{
    // 90-degree rotation around Z: x→y, y→-x
    mat_44 M;
    M.set_rotate_z(3.14159265358979f / 2.0f);
    // M * (1,0,0,0) = (0,1,0,0) (x→y)
    vec_4 ax(1.0f, 0.0f, 0.0f, 0.0f);
    vec_4 rx = M * ax;
    ASSERT_VEC_EQ(rx, 0.0f, 1.0f, 0.0f, 0.0f);
    return 0;
}

// ---- mat_33 trans_mult(vec_3) ----

int test_mat33_trans_mult_vec3()
{
    // M^T * v = (dot(col0,v), dot(col1,v), dot(col2,v))
    // M: diagonal scale col0=(1,0,0), col1=(0,2,0), col2=(0,0,3)
    mat_33 M;
    M.col0.set(1.0f, 0.0f, 0.0f);
    M.col1.set(0.0f, 2.0f, 0.0f);
    M.col2.set(0.0f, 0.0f, 3.0f);
    vec_3 v(1.0f, 1.0f, 1.0f);
    vec_3 r = M.trans_mult(v);
    ASSERT_VEC3_EQ(r, 1.0f, 2.0f, 3.0f);
    return 0;
}

// ---- mat_33 rotation tests ----

int test_mat33_set_rotate_x()
{
    // 90-degree rotation around X: y→z, z→-y
    mat_33 M;
    M.set_rotate_x(3.14159265358979f / 2.0f);
    vec_3 ry = M * vec_3(0.0f, 1.0f, 0.0f);
    ASSERT_VEC3_EQ(ry, 0.0f, 0.0f, 1.0f);
    vec_3 rz = M * vec_3(0.0f, 0.0f, 1.0f);
    ASSERT_VEC3_EQ(rz, 0.0f, -1.0f, 0.0f);
    return 0;
}

int test_mat33_set_rotate_y()
{
    // 90-degree rotation around Y: x→-z, z→x
    mat_33 M;
    M.set_rotate_y(3.14159265358979f / 2.0f);
    vec_3 rx = M * vec_3(1.0f, 0.0f, 0.0f);
    ASSERT_VEC3_EQ(rx, 0.0f, 0.0f, -1.0f);
    vec_3 rz = M * vec_3(0.0f, 0.0f, 1.0f);
    ASSERT_VEC3_EQ(rz, 1.0f, 0.0f, 0.0f);
    return 0;
}

int test_mat33_set_rotate_z()
{
    // 90-degree rotation around Z: x→y, y→-x
    mat_33 M;
    M.set_rotate_z(3.14159265358979f / 2.0f);
    vec_3 rx = M * vec_3(1.0f, 0.0f, 0.0f);
    ASSERT_VEC3_EQ(rx, 0.0f, 1.0f, 0.0f);
    vec_3 ry = M * vec_3(0.0f, 1.0f, 0.0f);
    ASSERT_VEC3_EQ(ry, -1.0f, 0.0f, 0.0f);
    return 0;
}

// ---- mat_33 add/subtract ----

int test_mat33_add()
{
    mat_33 A, B;
    A.col0.set(1.0f, 2.0f, 3.0f);
    A.col1.set(4.0f, 5.0f, 6.0f);
    A.col2.set(7.0f, 8.0f, 9.0f);
    B.col0.set(9.0f, 8.0f, 7.0f);
    B.col1.set(6.0f, 5.0f, 4.0f);
    B.col2.set(3.0f, 2.0f, 1.0f);
    mat_33 C = A + B;
    ASSERT_VEC3_EQ(C.get_row0(), 10.0f, 10.0f, 10.0f);
    ASSERT_VEC3_EQ(C.get_row1(), 10.0f, 10.0f, 10.0f);
    ASSERT_VEC3_EQ(C.get_row2(), 10.0f, 10.0f, 10.0f);
    return 0;
}

int test_mat33_subtract()
{
    mat_33 A, B;
    A.col0.set(5.0f, 5.0f, 5.0f);
    A.col1.set(5.0f, 5.0f, 5.0f);
    A.col2.set(5.0f, 5.0f, 5.0f);
    B.col0.set(1.0f, 2.0f, 3.0f);
    B.col1.set(4.0f, 5.0f, 6.0f);
    B.col2.set(7.0f, 8.0f, 9.0f);
    mat_33 C = A - B;
    // C.col0=(4,3,2), col1=(1,0,-1), col2=(-2,-3,-4)
    ASSERT_VEC3_EQ(C.get_row0(), 4.0f, 1.0f, -2.0f);
    ASSERT_VEC3_EQ(C.get_row1(), 3.0f, 0.0f, -3.0f);
    ASSERT_VEC3_EQ(C.get_row2(), 2.0f, -1.0f, -4.0f);
    return 0;
}

// ---- mat_33 trans_mult(mat_33) ----

int test_mat33_trans_mult_mat33()
{
    // M.trans_mult(A) = M^T * A; for M=diag(2,3,4), result col_i = (2*A.col_i.x, 3*A.col_i.y, 4*A.col_i.z)
    mat_33 M;
    M.col0.set(2.0f, 0.0f, 0.0f);
    M.col1.set(0.0f, 3.0f, 0.0f);
    M.col2.set(0.0f, 0.0f, 4.0f);
    mat_33 I;
    I.set_identity();
    mat_33 R = M.trans_mult(I);
    // R.col_i = trans_mult(e_i): col0=(2,0,0), col1=(0,3,0), col2=(0,0,4)
    ASSERT_VEC3_EQ(R.get_row0(), 2.0f, 0.0f, 0.0f);
    ASSERT_VEC3_EQ(R.get_row1(), 0.0f, 3.0f, 0.0f);
    ASSERT_VEC3_EQ(R.get_row2(), 0.0f, 0.0f, 4.0f);
    return 0;
}

// ---- mat_33 from quaternion ----

int test_mat33_from_quat()
{
    // Identity quaternion (0,0,0,1) → identity matrix
    vec_4 q_id(0.0f, 0.0f, 0.0f, 1.0f);
    mat_33 M_id(q_id);
    ASSERT_VEC3_EQ(M_id.get_row0(), 1.0f, 0.0f, 0.0f);
    ASSERT_VEC3_EQ(M_id.get_row1(), 0.0f, 1.0f, 0.0f);
    ASSERT_VEC3_EQ(M_id.get_row2(), 0.0f, 0.0f, 1.0f);
    // Quaternion for 90° around Z: (0, 0, sin45, cos45) → rotate x→y, y→-x
    float s = sinf(3.14159265358979f / 4.0f);
    float c = cosf(3.14159265358979f / 4.0f);
    vec_4 q_z(0.0f, 0.0f, s, c);
    mat_33 M_z(q_z);
    // row0 = (0,-1,0), row1 = (1,0,0), row2 = (0,0,1)
    ASSERT_VEC3_EQ(M_z.get_row0(), 0.0f, -1.0f, 0.0f);
    ASSERT_VEC3_EQ(M_z.get_row1(), 1.0f, 0.0f, 0.0f);
    ASSERT_VEC3_EQ(M_z.get_row2(), 0.0f, 0.0f, 1.0f);
    return 0;
}

// ---- mat_33 mult_tilde ----

int test_mat33_mult_tilde()
{
    // identity.mult_tilde(v) = skew(v)
    // skew((1,2,3)).col0=(0,3,-2), col1=(-3,0,1), col2=(2,-1,0)
    mat_33 I;
    I.set_identity();
    vec_3 v(1.0f, 2.0f, 3.0f);
    mat_33 R = I.mult_tilde(v);
    // row0 = (0,-3,2), row1 = (3,0,-1), row2 = (-2,1,0)
    ASSERT_VEC3_EQ(R.get_row0(), 0.0f, -3.0f, 2.0f);
    ASSERT_VEC3_EQ(R.get_row1(), 3.0f, 0.0f, -1.0f);
    ASSERT_VEC3_EQ(R.get_row2(), -2.0f, 1.0f, 0.0f);
    return 0;
}

// ---- mat_44 rotation (y) ----

int test_mat44_set_rotate_y()
{
    // 90-degree rotation around Y: x→-z, z→x
    mat_44 M;
    M.set_rotate_y(3.14159265358979f / 2.0f);
    vec_4 ax(1.0f, 0.0f, 0.0f, 0.0f);
    vec_4 rx = M * ax;
    ASSERT_VEC_EQ(rx, 0.0f, 0.0f, -1.0f, 0.0f);
    vec_4 az(0.0f, 0.0f, 1.0f, 0.0f);
    vec_4 rz = M * az;
    ASSERT_VEC_EQ(rz, 1.0f, 0.0f, 0.0f, 0.0f);
    return 0;
}

// ---- mat_44 add/subtract ----

int test_mat44_add()
{
    mat_44 A, B;
    A.set_identity();
    B.set_identity();
    mat_44 C = A + B;
    // C = 2*I
    ASSERT_VEC_EQ(C.get_row0(), 2.0f, 0.0f, 0.0f, 0.0f);
    ASSERT_VEC_EQ(C.get_row1(), 0.0f, 2.0f, 0.0f, 0.0f);
    ASSERT_VEC_EQ(C.get_row2(), 0.0f, 0.0f, 2.0f, 0.0f);
    ASSERT_VEC_EQ(C.get_row3(), 0.0f, 0.0f, 0.0f, 2.0f);
    return 0;
}

int test_mat44_subtract()
{
    mat_44 A;
    A.set_identity();
    mat_44 B;
    B.col0.set(1.0f, 0.0f, 0.0f, 0.0f);
    B.col1.set(0.0f, 2.0f, 0.0f, 0.0f);
    B.col2.set(0.0f, 0.0f, 3.0f, 0.0f);
    B.col3.set(0.0f, 0.0f, 0.0f, 4.0f);
    mat_44 C = A - B;
    // I - diag(1,2,3,4) = diag(0,-1,-2,-3)
    ASSERT_VEC_EQ(C.get_row0(), 0.0f, 0.0f, 0.0f, 0.0f);
    ASSERT_VEC_EQ(C.get_row1(), 0.0f, -1.0f, 0.0f, 0.0f);
    ASSERT_VEC_EQ(C.get_row2(), 0.0f, 0.0f, -2.0f, 0.0f);
    ASSERT_VEC_EQ(C.get_row3(), 0.0f, 0.0f, 0.0f, -3.0f);
    return 0;
}

// ---- mat_44 trans_mult(mat_44) ----

int test_mat44_trans_mult_mat44()
{
    // diag(2,3,4,1).trans_mult(I) = diag(2,3,4,1)
    mat_44 M;
    M.col0.set(2.0f, 0.0f, 0.0f, 0.0f);
    M.col1.set(0.0f, 3.0f, 0.0f, 0.0f);
    M.col2.set(0.0f, 0.0f, 4.0f, 0.0f);
    M.col3.set(0.0f, 0.0f, 0.0f, 1.0f);
    mat_44 I;
    I.set_identity();
    mat_44 R = M.trans_mult(I);
    ASSERT_VEC_EQ(R.get_row0(), 2.0f, 0.0f, 0.0f, 0.0f);
    ASSERT_VEC_EQ(R.get_row1(), 0.0f, 3.0f, 0.0f, 0.0f);
    ASSERT_VEC_EQ(R.get_row2(), 0.0f, 0.0f, 4.0f, 0.0f);
    ASSERT_VEC_EQ(R.get_row3(), 0.0f, 0.0f, 0.0f, 1.0f);
    return 0;
}

// ---- mat_44 * mat_43 ----

int test_mat44_multiply_mat43()
{
    // I44 * identity_43 = identity_43
    mat_44 I44;
    I44.set_identity();
    mat_43 I43(vec_4(1.0f, 0.0f, 0.0f, 0.0f),
               vec_4(0.0f, 1.0f, 0.0f, 0.0f),
               vec_4(0.0f, 0.0f, 1.0f, 0.0f));
    mat_43 R = I44 * I43;
    vec_xyzw c0 = R.get_col0();
    vec_xyzw c1 = R.get_col1();
    vec_xyzw c2 = R.get_col2();
    ASSERT_VEC_EQ(c0, 1.0f, 0.0f, 0.0f, 0.0f);
    ASSERT_VEC_EQ(c1, 0.0f, 1.0f, 0.0f, 0.0f);
    ASSERT_VEC_EQ(c2, 0.0f, 0.0f, 1.0f, 0.0f);
    return 0;
}

// ---- mat_43 negate ----

int test_mat43_negate()
{
    mat_43 M(vec_4(1.0f, 2.0f, 3.0f, 4.0f),
             vec_4(5.0f, 6.0f, 7.0f, 8.0f),
             vec_4(9.0f, 10.0f, 11.0f, 12.0f));
    mat_43 N = -M;
    vec_xyzw c0 = N.get_col0();
    vec_xyzw c1 = N.get_col1();
    vec_xyzw c2 = N.get_col2();
    ASSERT_VEC_EQ(c0, -1.0f, -2.0f, -3.0f, -4.0f);
    ASSERT_VEC_EQ(c1, -5.0f, -6.0f, -7.0f, -8.0f);
    ASSERT_VEC_EQ(c2, -9.0f, -10.0f, -11.0f, -12.0f);
    return 0;
}

// ---- mat_43 add/subtract ----

int test_mat43_add()
{
    mat_43 A(vec_4(1.0f, 2.0f, 3.0f, 4.0f),
             vec_4(5.0f, 6.0f, 7.0f, 8.0f),
             vec_4(9.0f, 10.0f, 11.0f, 12.0f));
    mat_43 B(vec_4(-1.0f, -2.0f, -3.0f, -4.0f),
             vec_4(-5.0f, -6.0f, -7.0f, -8.0f),
             vec_4(-9.0f, -10.0f, -11.0f, -12.0f));
    mat_43 C = A + B;
    vec_xyzw c0 = C.get_col0();
    vec_xyzw c1 = C.get_col1();
    vec_xyzw c2 = C.get_col2();
    ASSERT_VEC_EQ(c0, 0.0f, 0.0f, 0.0f, 0.0f);
    ASSERT_VEC_EQ(c1, 0.0f, 0.0f, 0.0f, 0.0f);
    ASSERT_VEC_EQ(c2, 0.0f, 0.0f, 0.0f, 0.0f);
    return 0;
}

int test_mat43_subtract()
{
    mat_43 A(vec_4(5.0f, 5.0f, 5.0f, 5.0f),
             vec_4(5.0f, 5.0f, 5.0f, 5.0f),
             vec_4(5.0f, 5.0f, 5.0f, 5.0f));
    mat_43 B(vec_4(1.0f, 2.0f, 3.0f, 4.0f),
             vec_4(1.0f, 2.0f, 3.0f, 4.0f),
             vec_4(1.0f, 2.0f, 3.0f, 4.0f));
    mat_43 C = A - B;
    vec_xyzw c0 = C.get_col0();
    ASSERT_VEC_EQ(c0, 4.0f, 3.0f, 2.0f, 1.0f);
    return 0;
}

// ---- mat_43 * mat_33 ----

int test_mat43_multiply_mat33()
{
    // identity_43 * identity_33 = identity_43
    mat_43 I43(vec_4(1.0f, 0.0f, 0.0f, 0.0f),
               vec_4(0.0f, 1.0f, 0.0f, 0.0f),
               vec_4(0.0f, 0.0f, 1.0f, 0.0f));
    mat_33 I33;
    I33.set_identity();
    mat_43 R = I43 * I33;
    vec_xyzw c0 = R.get_col0();
    vec_xyzw c1 = R.get_col1();
    vec_xyzw c2 = R.get_col2();
    ASSERT_VEC_EQ(c0, 1.0f, 0.0f, 0.0f, 0.0f);
    ASSERT_VEC_EQ(c1, 0.0f, 1.0f, 0.0f, 0.0f);
    ASSERT_VEC_EQ(c2, 0.0f, 0.0f, 1.0f, 0.0f);
    return 0;
}

// ---- mat_43 mult_tilde ----

int test_mat43_mult_tilde()
{
    // identity_43.mult_tilde(v) columns = skew(v) padded to vec_4
    mat_43 I43(vec_4(1.0f, 0.0f, 0.0f, 0.0f),
               vec_4(0.0f, 1.0f, 0.0f, 0.0f),
               vec_4(0.0f, 0.0f, 1.0f, 0.0f));
    vec_3 v(1.0f, 2.0f, 3.0f);
    mat_43 R = I43.mult_tilde(v);
    // col0=(0,3,-2,0), col1=(-3,0,1,0), col2=(2,-1,0,0)
    vec_xyzw c0 = R.get_col0();
    vec_xyzw c1 = R.get_col1();
    vec_xyzw c2 = R.get_col2();
    ASSERT_VEC_EQ(c0, 0.0f, 3.0f, -2.0f, 0.0f);
    ASSERT_VEC_EQ(c1, -3.0f, 0.0f, 1.0f, 0.0f);
    ASSERT_VEC_EQ(c2, 2.0f, -1.0f, 0.0f, 0.0f);
    return 0;
}

// ---- mat_34 negate ----

int test_mat34_negate()
{
    mat_34 M(vec_3(1.0f, 2.0f, 3.0f),
             vec_3(4.0f, 5.0f, 6.0f),
             vec_3(7.0f, 8.0f, 9.0f),
             vec_3(10.0f, 11.0f, 12.0f));
    mat_34 N = -M;
    ASSERT_VEC3_EQ(N.get_row0(), -1.0f, -4.0f, -7.0f);
    ASSERT_VEC3_EQ(N.get_row1(), -2.0f, -5.0f, -8.0f);
    ASSERT_VEC3_EQ(N.get_row2(), -3.0f, -6.0f, -9.0f);
    return 0;
}

// ---- mat_34 add/subtract ----

int test_mat34_add()
{
    mat_34 A(vec_3(1.0f, 0.0f, 0.0f),
             vec_3(0.0f, 1.0f, 0.0f),
             vec_3(0.0f, 0.0f, 1.0f),
             vec_3(0.0f, 0.0f, 0.0f));
    mat_34 B(vec_3(0.0f, 0.0f, 0.0f),
             vec_3(0.0f, 0.0f, 0.0f),
             vec_3(0.0f, 0.0f, 0.0f),
             vec_3(4.0f, 5.0f, 6.0f));
    mat_34 C = A + B;
    ASSERT_VEC3_EQ(C.get_row0(), 1.0f, 0.0f, 0.0f);
    ASSERT_VEC3_EQ(C.get_row1(), 0.0f, 1.0f, 0.0f);
    ASSERT_VEC3_EQ(C.get_row2(), 0.0f, 0.0f, 1.0f);
    return 0;
}

int test_mat34_subtract()
{
    mat_34 A(vec_3(2.0f, 0.0f, 0.0f),
             vec_3(0.0f, 2.0f, 0.0f),
             vec_3(0.0f, 0.0f, 2.0f),
             vec_3(0.0f, 0.0f, 0.0f));
    mat_34 B(vec_3(1.0f, 0.0f, 0.0f),
             vec_3(0.0f, 1.0f, 0.0f),
             vec_3(0.0f, 0.0f, 1.0f),
             vec_3(0.0f, 0.0f, 0.0f));
    mat_34 C = A - B;
    ASSERT_VEC3_EQ(C.get_row0(), 1.0f, 0.0f, 0.0f);
    ASSERT_VEC3_EQ(C.get_row1(), 0.0f, 1.0f, 0.0f);
    ASSERT_VEC3_EQ(C.get_row2(), 0.0f, 0.0f, 1.0f);
    return 0;
}

// ---- mat_34 * mat_43 ----

int test_mat34_multiply_mat43()
{
    // I34 (with col3=0) * I43 = I33
    mat_34 I34(vec_3(1.0f, 0.0f, 0.0f),
               vec_3(0.0f, 1.0f, 0.0f),
               vec_3(0.0f, 0.0f, 1.0f),
               vec_3(0.0f, 0.0f, 0.0f));
    mat_43 I43(vec_4(1.0f, 0.0f, 0.0f, 0.0f),
               vec_4(0.0f, 1.0f, 0.0f, 0.0f),
               vec_4(0.0f, 0.0f, 1.0f, 0.0f));
    mat_33 R = I34 * I43;
    ASSERT_VEC3_EQ(R.get_row0(), 1.0f, 0.0f, 0.0f);
    ASSERT_VEC3_EQ(R.get_row1(), 0.0f, 1.0f, 0.0f);
    ASSERT_VEC3_EQ(R.get_row2(), 0.0f, 0.0f, 1.0f);
    return 0;
}

// ---- mat_34 * mat_44 ----

int test_mat34_multiply_mat44()
{
    // I34 * I44 = I34
    mat_34 I34(vec_3(1.0f, 0.0f, 0.0f),
               vec_3(0.0f, 1.0f, 0.0f),
               vec_3(0.0f, 0.0f, 1.0f),
               vec_3(0.0f, 0.0f, 0.0f));
    mat_44 I44;
    I44.set_identity();
    mat_34 R = I34 * I44;
    ASSERT_VEC3_EQ(R.get_row0(), 1.0f, 0.0f, 0.0f);
    ASSERT_VEC3_EQ(R.get_row1(), 0.0f, 1.0f, 0.0f);
    ASSERT_VEC3_EQ(R.get_row2(), 0.0f, 0.0f, 1.0f);
    return 0;
}

// ---- mat_33::inverse ----

int test_mat33_inverse()
{
    // Diagonal matrix diag(2,3,4): inverse = diag(0.5, 1/3, 0.25)
    mat_33 M;
    M.col0.set(2.0f, 0.0f, 0.0f);
    M.col1.set(0.0f, 3.0f, 0.0f);
    M.col2.set(0.0f, 0.0f, 4.0f);
    mat_33 inv = M.inverse();
    ASSERT_VEC3_EQ(inv.get_row0(), 0.5f, 0.0f, 0.0f);
    ASSERT_VEC3_EQ(inv.get_row1(), 0.0f, 1.0f/3.0f, 0.0f);
    ASSERT_VEC3_EQ(inv.get_row2(), 0.0f, 0.0f, 0.25f);
    return 0;
}

// ---- vec_xyz::operator~() tilde matrix ----

int test_vec_xyz_tilde()
{
    // ~(1,2,3) = skew-symmetric matrix
    // col0=(0,3,-2), col1=(-3,0,1), col2=(2,-1,0)
    vec_3 v(1.0f, 2.0f, 3.0f);
    mat_33 T = ~v;
    ASSERT_VEC3_EQ(T.get_row0(), 0.0f, -3.0f, 2.0f);
    ASSERT_VEC3_EQ(T.get_row1(), 3.0f, 0.0f, -1.0f);
    ASSERT_VEC3_EQ(T.get_row2(), -2.0f, 1.0f, 0.0f);
    return 0;
}

// Helper: 90-degree Z rotation + translation(4,5,6)
static void make_test_transform(transform_t& T)
{
    // Write Rz(90 deg) + translation(4,5,6) using individual float stores
    // to avoid R5900 GCC TI-mode register-pair bug (stale upper register with sd)
    float *p = reinterpret_cast<float*>(&T);
    // col0 = (0, 1, 0, 0)
    p[0] = 0.0f;  p[1] = 1.0f;  p[2] = 0.0f;  p[3] = 0.0f;
    // col1 = (-1, 0, 0, 0)
    p[4] = -1.0f; p[5] = 0.0f;  p[6] = 0.0f;  p[7] = 0.0f;
    // col2 = (0, 0, 1, 0)
    p[8] = 0.0f;  p[9] = 0.0f;  p[10] = 1.0f; p[11] = 0.0f;
    // col3 = (4, 5, 6, 1)
    p[12] = 4.0f; p[13] = 5.0f; p[14] = 6.0f; p[15] = 1.0f;
}

// ---- transform_t::operator*(vec_4) ----

int test_transform_mult_vec4()
{
    transform_t T;
    make_test_transform(T);
    // T * (1,2,3,1): R*(1,2,3) + t = (0+(-2)+0+4, 1+0+0+5, 0+0+3+6, 1) = (2,6,9,1)
    vec_4 r = T * vec_4(1.0f, 2.0f, 3.0f, 1.0f);
    ASSERT_VEC_EQ(r, 2.0f, 6.0f, 9.0f, 1.0f);
    // T * (1,0,0,0): direction, translation not applied
    vec_4 r2 = T * vec_4(1.0f, 0.0f, 0.0f, 0.0f);
    ASSERT_VEC_EQ(r2, 0.0f, 1.0f, 0.0f, 0.0f);
    return 0;
}

// ---- transform_t::operator*(vector_t) ----

int test_transform_mult_vector()
{
    transform_t T;
    make_test_transform(T);
    // T * (1,0,0) = col0 = (0,1,0)
    vector_t r = T * vector_t(1.0f, 0.0f, 0.0f);
    ASSERT_VEC3_EQ(r, 0.0f, 1.0f, 0.0f);
    // T * (0,1,0) = col1 = (-1,0,0)
    vector_t r2 = T * vector_t(0.0f, 1.0f, 0.0f);
    ASSERT_VEC3_EQ(r2, -1.0f, 0.0f, 0.0f);
    return 0;
}

// ---- transform_t::operator*(point_t) ----

int test_transform_mult_point()
{
    transform_t T;
    make_test_transform(T);
    // T * (1,2,3) = R*(1,2,3) + t = (2,6,9)
    point_t r = T * point_t(1.0f, 2.0f, 3.0f);
    ASSERT_VEC3_EQ(r, 2.0f, 6.0f, 9.0f);
    return 0;
}

// ---- transform_t::inverse() ----

int test_transform_inverse()
{
    transform_t T;
    make_test_transform(T);
    transform_t inv = T.inverse();
    // R_z(90)^{-1} = R_z(-90): col0=(0,-1,0), col1=(1,0,0), col2=(0,0,1)
    // -R^T * t = -(0*4+1*5+0*6, -1*4+0*5+0*6, 0*4+0*5+1*6) = (-5, 4, -6)
    // get_rowN = (colN.x, colN.y, colN.z) packed, last = col3.N
    ASSERT_VEC_EQ(inv.get_row0(), 0.0f, 1.0f, 0.0f, -5.0f);
    ASSERT_VEC_EQ(inv.get_row1(), -1.0f, 0.0f, 0.0f, 4.0f);
    ASSERT_VEC_EQ(inv.get_row2(), 0.0f, 0.0f, 1.0f, -6.0f);
    return 0;
}

// ---- transform_t::orthonormal_inverse() ----

int test_transform_orthonormal_inverse()
{
    transform_t T;
    make_test_transform(T);
    transform_t inv = T.orthonormal_inverse();
    ASSERT_VEC_EQ(inv.get_row0(), 0.0f, 1.0f, 0.0f, -5.0f);
    ASSERT_VEC_EQ(inv.get_row1(), -1.0f, 0.0f, 0.0f, 4.0f);
    ASSERT_VEC_EQ(inv.get_row2(), 0.0f, 0.0f, 1.0f, -6.0f);
    return 0;
}

// ---- transform_t::orthonormal_inverse_in_place() ----

int test_transform_orthonormal_inverse_in_place()
{
    transform_t T;
    make_test_transform(T);
    T.orthonormal_inverse_in_place();
    ASSERT_VEC_EQ(T.get_row0(), 0.0f, 1.0f, 0.0f, -5.0f);
    ASSERT_VEC_EQ(T.get_row1(), -1.0f, 0.0f, 0.0f, 4.0f);
    ASSERT_VEC_EQ(T.get_row2(), 0.0f, 0.0f, 1.0f, -6.0f);
    return 0;
}

#ifndef USE_CPU_COMPAT
// ============================================================================
// VU0 asm fast-path tests for cpu_vec_4 and cpu_mat_44 operators.
// These exercise the new VU0 macro-mode asm blocks added to cpu_vector.h /
// cpu_matrix.h that are gated on !NO_VU0_VECTORS. In the USE_CPU_COMPAT build
// the same operators take their scalar fallback path and are covered by the
// legacy CPU tests below.
// ============================================================================

int test_cpu_vec4_vu0_addition() {
    cpu_vec_4 a(1.0f, 2.0f, 3.0f, 4.0f), b(5.0f, 6.0f, 7.0f, 8.0f);
    cpu_vec_4 r = a + b;
    ASSERT_CPU_VEC4_EQ(r, 6.0f, 8.0f, 10.0f, 12.0f);
    return 0;
}

int test_cpu_vec4_vu0_addition_signed() {
    cpu_vec_4 a(-1.5f, 2.5f, -3.25f, 4.75f), b(1.5f, -2.5f, 3.25f, -4.75f);
    cpu_vec_4 r = a + b;
    ASSERT_CPU_VEC4_EQ(r, 0.0f, 0.0f, 0.0f, 0.0f);
    return 0;
}

int test_cpu_vec4_vu0_subtraction() {
    cpu_vec_4 a(10.0f, 20.0f, 30.0f, 40.0f), b(1.0f, 2.0f, 3.0f, 4.0f);
    cpu_vec_4 r = a - b;
    ASSERT_CPU_VEC4_EQ(r, 9.0f, 18.0f, 27.0f, 36.0f);
    return 0;
}

int test_cpu_vec4_vu0_negation() {
    cpu_vec_4 v(1.0f, -2.0f, 3.0f, -4.0f);
    cpu_vec_4 r = -v;
    ASSERT_CPU_VEC4_EQ(r, -1.0f, 2.0f, -3.0f, 4.0f);
    return 0;
}

int test_cpu_vec4_vu0_negation_zero() {
    cpu_vec_4 v(0.0f, 0.0f, 0.0f, 0.0f);
    cpu_vec_4 r = -v;
    ASSERT_CPU_VEC4_EQ(r, 0.0f, 0.0f, 0.0f, 0.0f);
    return 0;
}

int test_cpu_vec4_vu0_component_mul() {
    cpu_vec_4 a(2.0f, 3.0f, 4.0f, 5.0f), b(3.0f, 4.0f, 5.0f, 6.0f);
    cpu_vec_4 r = a * b;
    ASSERT_CPU_VEC4_EQ(r, 6.0f, 12.0f, 20.0f, 30.0f);
    return 0;
}

int test_cpu_vec4_vu0_component_mul_signed() {
    cpu_vec_4 a(-1.0f, 2.0f, -3.0f, 4.0f), b(2.0f, -3.0f, 4.0f, -5.0f);
    cpu_vec_4 r = a * b;
    ASSERT_CPU_VEC4_EQ(r, -2.0f, -6.0f, -12.0f, -20.0f);
    return 0;
}

int test_cpu_mat44_vu0_mult_vector_identity() {
    cpu_mat_44 m; m.set_identity();
    cpu_vec_4 v(1.5f, -2.5f, 3.5f, -4.5f);
    cpu_vec_4 r = m * v;
    ASSERT_CPU_VEC4_EQ(r, 1.5f, -2.5f, 3.5f, -4.5f);
    return 0;
}

int test_cpu_mat44_vu0_mult_vector_diag() {
    cpu_mat_44 m(cpu_vec_4(2.0f, 0.0f, 0.0f, 0.0f),
                 cpu_vec_4(0.0f, 3.0f, 0.0f, 0.0f),
                 cpu_vec_4(0.0f, 0.0f, 4.0f, 0.0f),
                 cpu_vec_4(0.0f, 0.0f, 0.0f, 5.0f));
    cpu_vec_4 v(1.0f, 1.0f, 1.0f, 1.0f);
    cpu_vec_4 r = m * v;
    ASSERT_CPU_VEC4_EQ(r, 2.0f, 3.0f, 4.0f, 5.0f);
    return 0;
}

int test_cpu_mat44_vu0_mult_vector_affine() {
    // Column-major: cols are basis vectors, last col is translation.
    cpu_mat_44 m(cpu_vec_4(1.0f, 0.0f, 0.0f, 0.0f),
                 cpu_vec_4(0.0f, 2.0f, 0.0f, 0.0f),
                 cpu_vec_4(0.0f, 0.0f, 3.0f, 0.0f),
                 cpu_vec_4(4.0f, 5.0f, 6.0f, 1.0f));
    cpu_vec_4 v(1.0f, 1.0f, 1.0f, 1.0f);
    cpu_vec_4 r = m * v;
    ASSERT_CPU_VEC4_EQ(r, 5.0f, 7.0f, 9.0f, 1.0f);
    return 0;
}

int test_cpu_mat44_vu0_mult_vector_general() {
    cpu_mat_44 m(cpu_vec_4(1.0f, 2.0f, 3.0f, 4.0f),
                 cpu_vec_4(5.0f, 6.0f, 7.0f, 8.0f),
                 cpu_vec_4(9.0f, 10.0f, 11.0f, 12.0f),
                 cpu_vec_4(13.0f, 14.0f, 15.0f, 16.0f));
    cpu_vec_4 v(1.0f, 1.0f, 1.0f, 1.0f);
    cpu_vec_4 r = m * v;
    // Sum of columns
    ASSERT_CPU_VEC4_EQ(r, 28.0f, 32.0f, 36.0f, 40.0f);
    return 0;
}

int test_cpu_mat44_vu0_mult_mat_identity() {
    cpu_mat_44 a(cpu_vec_4(1.0f, 2.0f, 3.0f, 4.0f),
                 cpu_vec_4(5.0f, 6.0f, 7.0f, 8.0f),
                 cpu_vec_4(9.0f, 10.0f, 11.0f, 12.0f),
                 cpu_vec_4(13.0f, 14.0f, 15.0f, 16.0f));
    cpu_mat_44 i; i.set_identity();
    cpu_mat_44 r = a * i;
    ASSERT_CPU_VEC4_EQ(r.get_col0(), 1.0f, 2.0f, 3.0f, 4.0f);
    ASSERT_CPU_VEC4_EQ(r.get_col1(), 5.0f, 6.0f, 7.0f, 8.0f);
    ASSERT_CPU_VEC4_EQ(r.get_col2(), 9.0f, 10.0f, 11.0f, 12.0f);
    ASSERT_CPU_VEC4_EQ(r.get_col3(), 13.0f, 14.0f, 15.0f, 16.0f);
    return 0;
}

int test_cpu_mat44_vu0_mult_mat_scale_translate() {
    cpu_mat_44 s; s.set_scale(cpu_vec_3(2.0f, 3.0f, 4.0f));
    cpu_mat_44 t; t.set_translate(cpu_vec_3(1.0f, 2.0f, 3.0f));
    cpu_mat_44 r = s * t;
    // Scale then translate: col3 becomes (2,6,12,1); diag stays (2,3,4,1).
    ASSERT_CPU_VEC4_EQ(r.get_col0(), 2.0f, 0.0f, 0.0f, 0.0f);
    ASSERT_CPU_VEC4_EQ(r.get_col1(), 0.0f, 3.0f, 0.0f, 0.0f);
    ASSERT_CPU_VEC4_EQ(r.get_col2(), 0.0f, 0.0f, 4.0f, 0.0f);
    ASSERT_CPU_VEC4_EQ(r.get_col3(), 2.0f, 6.0f, 12.0f, 1.0f);
    return 0;
}

int test_cpu_mat44_vu0_mult_mat_general() {
    // Verify a non-diagonal multiply against a hand-computed result.
    // A column-major: cols [(1,0,0,0), (0,1,0,0), (1,1,1,0), (0,0,0,1)]
    // B column-major: cols [(2,0,0,0), (0,2,0,0), (0,0,2,0), (1,1,1,1)]
    // R = A * B (column-major): col_k = A * B.col_k
    cpu_mat_44 A(cpu_vec_4(1.0f, 0.0f, 0.0f, 0.0f),
                 cpu_vec_4(0.0f, 1.0f, 0.0f, 0.0f),
                 cpu_vec_4(1.0f, 1.0f, 1.0f, 0.0f),
                 cpu_vec_4(0.0f, 0.0f, 0.0f, 1.0f));
    cpu_mat_44 B(cpu_vec_4(2.0f, 0.0f, 0.0f, 0.0f),
                 cpu_vec_4(0.0f, 2.0f, 0.0f, 0.0f),
                 cpu_vec_4(0.0f, 0.0f, 2.0f, 0.0f),
                 cpu_vec_4(1.0f, 1.0f, 1.0f, 1.0f));
    cpu_mat_44 R = A * B;
    ASSERT_CPU_VEC4_EQ(R.get_col0(), 2.0f, 0.0f, 0.0f, 0.0f);
    ASSERT_CPU_VEC4_EQ(R.get_col1(), 0.0f, 2.0f, 0.0f, 0.0f);
    ASSERT_CPU_VEC4_EQ(R.get_col2(), 2.0f, 2.0f, 2.0f, 0.0f);
    ASSERT_CPU_VEC4_EQ(R.get_col3(), 2.0f, 2.0f, 1.0f, 1.0f);
    return 0;
}
#endif // !USE_CPU_COMPAT

#ifdef USE_CPU_COMPAT
// ============================================================================
// CPU legacy type tests (cpu_vec_3, cpu_vec_4, cpu_mat_44)
// Only compiled in the USE_CPU_COMPAT build.
// ============================================================================
int test_cpu_vec3_construction() {
    cpu_vec_3 v(1.0f, 2.0f, 3.0f);
    ASSERT_CPU_VEC3_EQ(v, 1.0f, 2.0f, 3.0f); return 0; }

int test_cpu_vec3_set() {
    cpu_vec_3 v(0.0f, 0.0f, 0.0f);
    v.set(4.0f, 5.0f, 6.0f);
    ASSERT_CPU_VEC3_EQ(v, 4.0f, 5.0f, 6.0f); return 0; }

int test_cpu_vec3_addition() {
    cpu_vec_3 a(1.0f, 2.0f, 3.0f), b(4.0f, 5.0f, 6.0f);
    cpu_vec_3 r = a + b;
    ASSERT_CPU_VEC3_EQ(r, 5.0f, 7.0f, 9.0f); return 0; }

int test_cpu_vec3_subtraction() {
    cpu_vec_3 a(5.0f, 7.0f, 9.0f), b(1.0f, 2.0f, 3.0f);
    cpu_vec_3 r = a - b;
    ASSERT_CPU_VEC3_EQ(r, 4.0f, 5.0f, 6.0f); return 0; }

int test_cpu_vec3_negation() {
    cpu_vec_3 v(1.0f, -2.0f, 3.0f);
    cpu_vec_3 r = -v;
    ASSERT_CPU_VEC3_EQ(r, -1.0f, 2.0f, -3.0f); return 0; }

int test_cpu_vec3_scalar_mul() {
    cpu_vec_3 v(1.0f, 2.0f, 3.0f);
    cpu_vec_3 r = v * 3.0f;
    ASSERT_CPU_VEC3_EQ(r, 3.0f, 6.0f, 9.0f); return 0; }

int test_cpu_vec3_dot() {
    cpu_vec_3 a(1.0f, 2.0f, 3.0f), b(4.0f, 5.0f, 6.0f);
    ASSERT_FLOAT_EQ(a.dot(b), 32.0f); return 0; }  // 1*4+2*5+3*6=32

int test_cpu_vec3_cross() {
    cpu_vec_3 a(1.0f, 0.0f, 0.0f), b(0.0f, 1.0f, 0.0f);
    cpu_vec_3 r = a.cross(b);
    ASSERT_CPU_VEC3_EQ(r, 0.0f, 0.0f, 1.0f); return 0; }

int test_cpu_vec3_cross_general() {
    cpu_vec_3 a(1.0f, 2.0f, 3.0f), b(4.0f, 5.0f, 6.0f);
    cpu_vec_3 r = a.cross(b);
    ASSERT_CPU_VEC3_EQ(r, -3.0f, 6.0f, -3.0f); return 0; }

int test_cpu_vec3_normalized() {
    cpu_vec_3 v(3.0f, 0.0f, 0.0f);
    cpu_vec_3 r = v.normalized();
    ASSERT_CPU_VEC3_EQ(r, 1.0f, 0.0f, 0.0f); return 0; }

int test_cpu_vec3_normalize() {
    cpu_vec_3 v(0.0f, 0.0f, 5.0f);
    v.normalize();
    ASSERT_CPU_VEC3_EQ(v, 0.0f, 0.0f, 1.0f); return 0; }

int test_cpu_vec3_index_operator() {
    cpu_vec_3 v(7.0f, 8.0f, 9.0f);
    ASSERT_FLOAT_EQ(v(0), 7.0f);
    ASSERT_FLOAT_EQ(v(1), 8.0f);
    ASSERT_FLOAT_EQ(v(2), 9.0f); return 0; }

int test_cpu_vec4_construction() {
    cpu_vec_4 v(1.0f, 2.0f, 3.0f, 4.0f);
    ASSERT_CPU_VEC4_EQ(v, 1.0f, 2.0f, 3.0f, 4.0f); return 0; }

int test_cpu_vec4_set() {
    cpu_vec_4 v(0.0f, 0.0f, 0.0f, 0.0f);
    v.set(1.0f, 2.0f, 3.0f, 4.0f);
    ASSERT_CPU_VEC4_EQ(v, 1.0f, 2.0f, 3.0f, 4.0f); return 0; }

int test_cpu_vec4_addition() {
    cpu_vec_4 a(1.0f, 2.0f, 3.0f, 4.0f), b(5.0f, 6.0f, 7.0f, 8.0f);
    cpu_vec_4 r = a + b;
    ASSERT_CPU_VEC4_EQ(r, 6.0f, 8.0f, 10.0f, 12.0f); return 0; }

int test_cpu_vec4_subtraction() {
    cpu_vec_4 a(5.0f, 6.0f, 7.0f, 8.0f), b(1.0f, 2.0f, 3.0f, 4.0f);
    cpu_vec_4 r = a - b;
    ASSERT_CPU_VEC4_EQ(r, 4.0f, 4.0f, 4.0f, 4.0f); return 0; }

int test_cpu_vec4_negation() {
    cpu_vec_4 v(1.0f, -2.0f, 3.0f, -4.0f);
    cpu_vec_4 r = -v;
    ASSERT_CPU_VEC4_EQ(r, -1.0f, 2.0f, -3.0f, 4.0f); return 0; }

int test_cpu_vec4_component_mul() {
    cpu_vec_4 a(2.0f, 3.0f, 4.0f, 5.0f), b(3.0f, 4.0f, 5.0f, 6.0f);
    cpu_vec_4 r = a * b;
    ASSERT_CPU_VEC4_EQ(r, 6.0f, 12.0f, 20.0f, 30.0f); return 0; }

int test_cpu_vec4_scalar_mul() {
    cpu_vec_4 v(1.0f, 2.0f, 3.0f, 4.0f);
    cpu_vec_4 r = v * 2.0f;
    ASSERT_CPU_VEC4_EQ(r, 2.0f, 4.0f, 6.0f, 8.0f); return 0; }

int test_cpu_vec4_dot() {
    cpu_vec_4 a(1.0f, 2.0f, 3.0f, 4.0f), b(5.0f, 6.0f, 7.0f, 8.0f);
    ASSERT_FLOAT_EQ(a.dot(b), 70.0f); return 0; }  // 1*5+2*6+3*7+4*8=70

int test_cpu_vec4_length() {
    cpu_vec_4 v(2.0f, 0.0f, 0.0f, 0.0f);
    ASSERT_FLOAT_EQ(v.length(), 2.0f);
    cpu_vec_4 v2(1.0f, 1.0f, 1.0f, 1.0f);
    ASSERT_FLOAT_EQ(v2.length(), 2.0f); return 0; }

int test_cpu_vec4_normalized() {
    cpu_vec_4 v(4.0f, 0.0f, 0.0f, 0.0f);
    cpu_vec_4 r = v.normalized();
    ASSERT_CPU_VEC4_EQ(r, 1.0f, 0.0f, 0.0f, 0.0f); return 0; }

int test_cpu_vec4_normalize() {
    cpu_vec_4 v(0.0f, 0.0f, 3.0f, 0.0f);
    v.normalize();
    ASSERT_CPU_VEC4_EQ(v, 0.0f, 0.0f, 1.0f, 0.0f); return 0; }

int test_cpu_mat44_set_identity() {
    cpu_mat_44 m; m.set_identity();
    ASSERT_CPU_VEC4_EQ(m.get_col0(), 1.0f, 0.0f, 0.0f, 0.0f);
    ASSERT_CPU_VEC4_EQ(m.get_col1(), 0.0f, 1.0f, 0.0f, 0.0f);
    ASSERT_CPU_VEC4_EQ(m.get_col2(), 0.0f, 0.0f, 1.0f, 0.0f);
    ASSERT_CPU_VEC4_EQ(m.get_col3(), 0.0f, 0.0f, 0.0f, 1.0f); return 0; }

int test_cpu_mat44_col_access() {
    cpu_mat_44 m;
    m.set_col0(cpu_vec_xyzw(1.0f, 2.0f, 3.0f, 4.0f));
    m.set_col1(cpu_vec_xyzw(5.0f, 6.0f, 7.0f, 8.0f));
    m.set_col2(cpu_vec_xyzw(9.0f, 10.0f, 11.0f, 12.0f));
    m.set_col3(cpu_vec_xyzw(13.0f, 14.0f, 15.0f, 16.0f));
    ASSERT_CPU_VEC4_EQ(m.get_col0(), 1.0f, 2.0f, 3.0f, 4.0f);
    ASSERT_CPU_VEC4_EQ(m.get_col1(), 5.0f, 6.0f, 7.0f, 8.0f);
    ASSERT_CPU_VEC4_EQ(m.get_col2(), 9.0f, 10.0f, 11.0f, 12.0f);
    ASSERT_CPU_VEC4_EQ(m.get_col3(), 13.0f, 14.0f, 15.0f, 16.0f); return 0; }

int test_cpu_mat44_mult_vector_identity() {
    cpu_mat_44 m; m.set_identity();
    cpu_vec_4 v(1.0f, 2.0f, 3.0f, 4.0f);
    cpu_vec_4 r = m * v;
    ASSERT_CPU_VEC4_EQ(r, 1.0f, 2.0f, 3.0f, 4.0f); return 0; }

int test_cpu_mat44_mult_vector() {
    cpu_mat_44 m(cpu_vec_4(1.0f, 0.0f, 0.0f, 0.0f),
                 cpu_vec_4(0.0f, 2.0f, 0.0f, 0.0f),
                 cpu_vec_4(0.0f, 0.0f, 3.0f, 0.0f),
                 cpu_vec_4(4.0f, 5.0f, 6.0f, 1.0f));
    cpu_vec_4 v(1.0f, 1.0f, 1.0f, 1.0f);
    cpu_vec_4 r = m * v;
    ASSERT_CPU_VEC4_EQ(r, 5.0f, 7.0f, 9.0f, 1.0f); return 0; }

int test_cpu_mat44_set_scale() {
    cpu_mat_44 m; m.set_scale(cpu_vec_3(2.0f, 3.0f, 4.0f));
    cpu_vec_4 r = m * cpu_vec_4(1.0f, 1.0f, 1.0f, 1.0f);
    ASSERT_CPU_VEC4_EQ(r, 2.0f, 3.0f, 4.0f, 1.0f); return 0; }

int test_cpu_mat44_set_translate() {
    cpu_mat_44 m; m.set_translate(cpu_vec_3(5.0f, 6.0f, 7.0f));
    cpu_vec_4 r = m * cpu_vec_4(1.0f, 2.0f, 3.0f, 1.0f);
    ASSERT_CPU_VEC4_EQ(r, 6.0f, 8.0f, 10.0f, 1.0f); return 0; }

int test_cpu_mat44_set_rotate_z() {
    cpu_mat_44 m;
    m.set_rotate(3.14159265358979f / 2.0f, cpu_vec_3(0.0f, 0.0f, 1.0f));
    cpu_vec_4 r = m * cpu_vec_4(1.0f, 0.0f, 0.0f, 1.0f);
    ASSERT_FLOAT_EQ(r.x, 0.0f); ASSERT_FLOAT_EQ(r.y, 1.0f); ASSERT_FLOAT_EQ(r.z, 0.0f); return 0; }

int test_cpu_mat44_set_rotate_x() {
    cpu_mat_44 m;
    m.set_rotate(3.14159265358979f / 2.0f, cpu_vec_3(1.0f, 0.0f, 0.0f));
    cpu_vec_4 r = m * cpu_vec_4(0.0f, 1.0f, 0.0f, 1.0f);
    ASSERT_FLOAT_EQ(r.x, 0.0f); ASSERT_FLOAT_EQ(r.y, 0.0f); ASSERT_FLOAT_EQ(r.z, 1.0f); return 0; }

int test_cpu_mat44_transpose() {
    cpu_mat_44 m(cpu_vec_4(1.0f, 2.0f, 3.0f, 4.0f),
                 cpu_vec_4(5.0f, 6.0f, 7.0f, 8.0f),
                 cpu_vec_4(9.0f, 10.0f, 11.0f, 12.0f),
                 cpu_vec_4(13.0f, 14.0f, 15.0f, 16.0f));
    cpu_mat_44 t = m.transpose();
    ASSERT_CPU_VEC4_EQ(t.get_col0(), 1.0f,  5.0f,  9.0f,  13.0f);
    ASSERT_CPU_VEC4_EQ(t.get_col1(), 2.0f,  6.0f,  10.0f, 14.0f);
    ASSERT_CPU_VEC4_EQ(t.get_col2(), 3.0f,  7.0f,  11.0f, 15.0f);
    ASSERT_CPU_VEC4_EQ(t.get_col3(), 4.0f,  8.0f,  12.0f, 16.0f); return 0; }

int test_cpu_mat44_multiply_mat() {
    cpu_mat_44 a; a.set_scale(cpu_vec_3(2.0f, 3.0f, 4.0f));
    cpu_mat_44 b; b.set_translate(cpu_vec_3(1.0f, 2.0f, 3.0f));
    cpu_mat_44 r = a * b;
    ASSERT_CPU_VEC4_EQ(r.get_col3(), 2.0f, 6.0f, 12.0f, 1.0f);
    ASSERT_CPU_VEC4_EQ(r.get_col0(), 2.0f, 0.0f, 0.0f, 0.0f); return 0; }
#endif // USE_CPU_COMPAT

int main()
{
        printf("=== VU0 Function Unit Tests ===\n\n");

        // Detect emulator/hardware VU multiply behavior and report it up front.
        // This helps identify float corner-case differences (1*X vs X*1) noted
        // in https://fobes.dev/ps2/detecting-emu-vu-floats
        g_vu_mul_emulator_bug = isVUMulErrorPresent();
        printf("VU mul 1*X behavior detection: %s\n\n",
            g_vu_mul_emulator_bug ? "hardware-like (1*X != X)" : "emulator-like (1*X == X)");

        int failed = 0;
    
    // Vector 3D tests
    RUN_TEST(test_vec3_addition);
    RUN_TEST(test_vec3_subtraction);
    RUN_TEST(test_vec3_multiplication);
    RUN_TEST(test_vec3_scalar_multiplication);
    RUN_TEST(test_vec3_division);
    RUN_TEST(test_vec3_negate);
    RUN_TEST(test_vec3_abs);
    RUN_TEST(test_vec3_max);
    RUN_TEST(test_vec3_min);
    RUN_TEST(test_vec3_normalized);
    RUN_TEST(test_vec3_set_zero);
    RUN_TEST(test_vec3_set);
    RUN_TEST(test_vec3_set_vec);
    RUN_TEST(test_vec3_operator_plus_equals);
    RUN_TEST(test_vec3_operator_minus_equals);
    RUN_TEST(test_vec3_operator_multiply_equals);
    RUN_TEST(test_vec3_dot_product);
    RUN_TEST(test_vec3_length);
    RUN_TEST(test_vec3_length_sqr);
    RUN_TEST(test_vec3_cross_product);
    
    // Vector 4D tests
    RUN_TEST(test_vec4_addition);
    RUN_TEST(test_vec4_subtraction);
    RUN_TEST(test_vec4_scalar_multiplication);
    
    RUN_TEST(test_mat33_set_zero);
    RUN_TEST(test_mat33_set_row);
    RUN_TEST(test_mat33_get_row);
    RUN_TEST(test_mat33_get_row1);
    RUN_TEST(test_mat33_get_row2);
    RUN_TEST(test_mat33_multiply_vector);
    RUN_TEST(test_mat33_set_identity);
    RUN_TEST(test_mat33_set_scale);
    RUN_TEST(test_mat33_transpose);
    
    // Matrix 4x4 tests
    RUN_TEST(test_mat44_set_zero);
    RUN_TEST(test_mat44_set_row);
    RUN_TEST(test_mat44_get_row1);
    RUN_TEST(test_mat44_get_row2);
    RUN_TEST(test_mat44_get_row3);
    RUN_TEST(test_mat44_multiply_vector);
    RUN_TEST(test_mat44_set_identity);
    RUN_TEST(test_mat44_set_scale);
    RUN_TEST(test_mat44_set_translate);
    RUN_TEST(test_mat44_transpose);
    
    // Vector 4D additional tests
    RUN_TEST(test_vec4_normalized3);
    RUN_TEST(test_vec4_normalize3);
    
    // Vector 3D additional tests
    RUN_TEST(test_vec3_normalized_zero);
    RUN_TEST(test_vec3_truncate_length_shorter);
    RUN_TEST(test_vec3_truncate_length_longer);
    RUN_TEST(test_vec3_is_zero);
    RUN_TEST(test_vec3_sign);
    RUN_TEST(test_vec3_one_over);
    RUN_TEST(test_vec3_interpolate);
    RUN_TEST(test_vec3_distance_from);
    RUN_TEST(test_vec3_set_length);
    RUN_TEST(test_vec3_parallel_component);
    RUN_TEST(test_vec3_perpendicular_component);
    RUN_TEST(test_vec3_normalize);

#ifndef USE_CPU_COMPAT
    // Accumulator tests
    RUN_TEST(test_accumulator_operations);
    RUN_TEST(test_vec3_to_a_from_a);
    RUN_TEST(test_vec3_aadd);
    RUN_TEST(test_vec3_asub);
    RUN_TEST(test_vec3_madd);
    RUN_TEST(test_vec3_msub);
    RUN_TEST(test_vec3_madda);
    RUN_TEST(test_vec3_msuba);
    RUN_TEST(test_vec3_aadda);
    RUN_TEST(test_vec3_asuba);
    
    // VU0 copy function tests
    RUN_TEST(test_vu0_copy_qwords);
    RUN_TEST(test_vu0_copy_even_qwords);
#endif // !USE_CPU_COMPAT

    // mat_43 tests
    RUN_TEST(test_mat43_set_zero);
    RUN_TEST(test_mat43_get_rows);
    RUN_TEST(test_mat43_multiply_vector);
    RUN_TEST(test_mat43_transpose);
    RUN_TEST(test_mat43_trans_mult_vec4);
    RUN_TEST(test_mat43_trans_mult_vector_t);
    RUN_TEST(test_mat43_trans_mult_point_t);

    // mat_34 tests
    RUN_TEST(test_mat34_set_zero);
    RUN_TEST(test_mat34_get_rows);
    RUN_TEST(test_mat34_multiply_vector);
    RUN_TEST(test_mat34_transpose);
    RUN_TEST(test_mat34_trans_mult_vec3);

    // Matrix-matrix multiply tests
    RUN_TEST(test_mat33_multiply_mat33);
    RUN_TEST(test_mat33_multiply_mat33_nontrivial);
    RUN_TEST(test_mat44_multiply_mat44);

    // Row-vector * matrix tests
    RUN_TEST(test_vec3_multiply_mat33);
    RUN_TEST(test_vec3_multiply_mat34);
    RUN_TEST(test_vec4_multiply_mat43);
    RUN_TEST(test_vec4_multiply_mat44);

    // mat_44 trans_mult tests
    RUN_TEST(test_mat44_trans_mult_vec4);
    RUN_TEST(test_mat44_trans_mult_vector_t);
    RUN_TEST(test_mat44_trans_mult_point_t);

    // Matrix negate tests
    RUN_TEST(test_mat33_negate);
    RUN_TEST(test_mat44_negate);
    RUN_TEST(test_mat43_negate);
    RUN_TEST(test_mat34_negate);

    // mat_44 rotation tests
    RUN_TEST(test_mat44_set_rotate_x);
    RUN_TEST(test_mat44_set_rotate_z);
    RUN_TEST(test_mat44_set_rotate_y);

    // mat_33 rotation tests
    RUN_TEST(test_mat33_set_rotate_x);
    RUN_TEST(test_mat33_set_rotate_y);
    RUN_TEST(test_mat33_set_rotate_z);

    // mat_33 trans_mult and arithmetic
    RUN_TEST(test_mat33_trans_mult_vec3);
    RUN_TEST(test_mat33_trans_mult_mat33);
    RUN_TEST(test_mat33_add);
    RUN_TEST(test_mat33_subtract);

    // mat_33 quaternion constructor and mult_tilde
    RUN_TEST(test_mat33_from_quat);
    RUN_TEST(test_mat33_mult_tilde);

    // mat_44 arithmetic and products
    RUN_TEST(test_mat44_add);
    RUN_TEST(test_mat44_subtract);
    RUN_TEST(test_mat44_trans_mult_mat44);
    RUN_TEST(test_mat44_multiply_mat43);

    // mat_43 arithmetic and products
    RUN_TEST(test_mat43_add);
    RUN_TEST(test_mat43_subtract);
    RUN_TEST(test_mat43_multiply_mat33);
    RUN_TEST(test_mat43_mult_tilde);

    // mat_34 arithmetic and products
    RUN_TEST(test_mat34_add);
    RUN_TEST(test_mat34_subtract);
    RUN_TEST(test_mat34_multiply_mat43);
    RUN_TEST(test_mat34_multiply_mat44);

    // mat_33::inverse, vec_xyz tilde, transform_t operations
    RUN_TEST(test_mat33_inverse);
    RUN_TEST(test_vec_xyz_tilde);
    RUN_TEST(test_transform_mult_vec4);
    RUN_TEST(test_transform_mult_vector);
    RUN_TEST(test_transform_mult_point);
    RUN_TEST(test_transform_inverse);
    RUN_TEST(test_transform_orthonormal_inverse);
    RUN_TEST(test_transform_orthonormal_inverse_in_place);

#ifndef USE_CPU_COMPAT
    printf("\n--- cpu_vec_4 / cpu_mat_44 VU0 asm fast paths ---\n\n");
    RUN_TEST(test_cpu_vec4_vu0_addition);
    RUN_TEST(test_cpu_vec4_vu0_addition_signed);
    RUN_TEST(test_cpu_vec4_vu0_subtraction);
    RUN_TEST(test_cpu_vec4_vu0_negation);
    RUN_TEST(test_cpu_vec4_vu0_negation_zero);
    RUN_TEST(test_cpu_vec4_vu0_component_mul);
    RUN_TEST(test_cpu_vec4_vu0_component_mul_signed);
    RUN_TEST(test_cpu_mat44_vu0_mult_vector_identity);
    RUN_TEST(test_cpu_mat44_vu0_mult_vector_diag);
    RUN_TEST(test_cpu_mat44_vu0_mult_vector_affine);
    RUN_TEST(test_cpu_mat44_vu0_mult_vector_general);
    RUN_TEST(test_cpu_mat44_vu0_mult_mat_identity);
    RUN_TEST(test_cpu_mat44_vu0_mult_mat_scale_translate);
    RUN_TEST(test_cpu_mat44_vu0_mult_mat_general);
#endif // !USE_CPU_COMPAT

#ifdef USE_CPU_COMPAT
    printf("\n--- CPU legacy types (cpu_vec_3, cpu_vec_4, cpu_mat_44) ---\n\n");
    RUN_TEST(test_cpu_vec3_construction);
    RUN_TEST(test_cpu_vec3_set);
    RUN_TEST(test_cpu_vec3_addition);
    RUN_TEST(test_cpu_vec3_subtraction);
    RUN_TEST(test_cpu_vec3_negation);
    RUN_TEST(test_cpu_vec3_scalar_mul);
    RUN_TEST(test_cpu_vec3_dot);
    RUN_TEST(test_cpu_vec3_cross);
    RUN_TEST(test_cpu_vec3_cross_general);
    RUN_TEST(test_cpu_vec3_normalized);
    RUN_TEST(test_cpu_vec3_normalize);
    RUN_TEST(test_cpu_vec3_index_operator);
    RUN_TEST(test_cpu_vec4_construction);
    RUN_TEST(test_cpu_vec4_set);
    RUN_TEST(test_cpu_vec4_addition);
    RUN_TEST(test_cpu_vec4_subtraction);
    RUN_TEST(test_cpu_vec4_negation);
    RUN_TEST(test_cpu_vec4_component_mul);
    RUN_TEST(test_cpu_vec4_scalar_mul);
    RUN_TEST(test_cpu_vec4_dot);
    RUN_TEST(test_cpu_vec4_length);
    RUN_TEST(test_cpu_vec4_normalized);
    RUN_TEST(test_cpu_vec4_normalize);
    RUN_TEST(test_cpu_mat44_set_identity);
    RUN_TEST(test_cpu_mat44_col_access);
    RUN_TEST(test_cpu_mat44_mult_vector_identity);
    RUN_TEST(test_cpu_mat44_mult_vector);
    RUN_TEST(test_cpu_mat44_set_scale);
    RUN_TEST(test_cpu_mat44_set_translate);
    RUN_TEST(test_cpu_mat44_set_rotate_z);
    RUN_TEST(test_cpu_mat44_set_rotate_x);
    RUN_TEST(test_cpu_mat44_transpose);
    RUN_TEST(test_cpu_mat44_multiply_mat);
#endif // USE_CPU_COMPAT

    printf("=== All Tests Completed ===\n");
    printf("Total tests run: %d\n", test_count);
    
    if (failed_count > 0) {
        printf("\n=== FAILED TESTS ===\n");
        printf("Total failed: %d\n", failed_count);
        for (int i = 0; i < failed_count; i++) {
            printf("  - %s\n", failed_tests[i]);
        }
        printf("\n");
    } else {
        printf("All tests passed!\n");
    }

    SleepThread();
    
    return (failed_count > 0) ? 1 : 0;
}

