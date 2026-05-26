# VU0 Unit Tests

This directory contains unit tests for the VU0 functions that were converted from custom GCC extensions to standard GCC inline assembly.

## Building

Configure and build from the repository root with tests enabled:

```bash
cmake -S . -B build -DBUILD_TESTS=ON
cmake --build build -j
```

## Running

To run the tests on a PS2 via ps2client:

```bash
cd build/tests
ps2client execee host:test_vu0.elf
```

Or manually:

```bash
/Applications/PCSX2.app/Contents/MacOS/PCSX2 -elf $(pwd)/test_vu0.elf
```

Note on emulation differences:

- Some PS2 VU floating-point corner cases are not emulated identically by
	popular emulators (for example, specific VMUL cases where `1 * X` may not
	equal `X` on real hardware). If tests behave differently under PCSX2 vs
	real hardware, see: https://fobes.dev/ps2/detecting-emu-vu-floats and
	consider running the affected tests on real hardware or making assertions
	tolerant to small, platform-dependent float differences.

## Test Coverage

The tests cover the following VU0 functions:

### Vector Operations (vec_xyz)
- Addition, subtraction, multiplication, division
- Scalar multiplication
- Negation
- Absolute value
- Max/min operations
- Normalization
- Set operations (set_zero, set, set_vec)
- Compound assignment operators (+=, -=, *=)
- Dot product
- Length and length squared
- Cross product

### Vector Operations (vec_xyzw)
- Addition, subtraction
- Scalar multiplication

### Matrix Operations (mat_33)
- set_zero
- set_row/get_row
- Matrix-vector multiplication

### Matrix Operations (mat_44)
- set_zero
- set_row/get_row
- Matrix-vector multiplication

### Accumulator Operations
- Basic accumulator function tests

## Test Framework

The test framework uses simple macros:
- `ASSERT_EQ(a, b)` - Asserts two floats are equal within EPSILON
- `ASSERT_VEC3_EQ(v, x, y, z)` - Asserts a vec_xyz matches expected values
- `ASSERT_VEC_EQ(v, x, y, z, w)` - Asserts a vec_xyzw matches expected values
- `RUN_TEST(func)` - Runs a test function and reports results

## Notes

- EPSILON is set to 1e-5f for floating-point comparisons
- Tests use `vec_x(v)`, `vec_y(v)`, etc. to extract vector components
- Some functions in vector.h still use "j" constraints and may need conversion if you encounter issues

