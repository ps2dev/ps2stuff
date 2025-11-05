/*	  Copyright (C) 2000,2001,2002  Sony Computer Entertainment America

       	  This file is subject to the terms and conditions of the GNU Lesser
	  General Public License Version 2.1. See the file "COPYING" in the
	  main directory of this archive for more details.                             */

#ifndef vector_common_h
#define vector_common_h

#include <stdint.h>
#include <stdio.h>

/********************************************
 * This file contains the class 'vec_template' which provides all vector
 * methods that can be generalized, as well as the '*_base' classes that
 * are used to instanciate 'vec_template'.
 *
 * A few notes on implementation:
 *
 * - Everything here relies on Dylan's vu0 support patches to gcc, as well as
 *   Tyler's patches.  See the 'patches' project in cvs.
 *
 * - The classes are set up like this:
 *	*_base - provide the template and specific vector classes with info
 *		 on what fields they contain for use by the template methods.
 *				|
 *			subclassed by
 *				|
 *	vec_template - provides generalized methods for all vectors
 *				|
 *		instances subclassed by
 *				|
 *	vec_* -	provide vector-specific methods and wrappers for template
 *		methods, when necessary.
 *
 * - Another way to implement this would be with traits and a template that
 *   is used directly.  This would make wrappers in the vec_* classes
 *   unnecessary, and make interaction with floats, ints, etc. easier.  I
 *   chose the structure described above for several reasons:  the extra
 *   level of indirection in the traits concept can be confusing, error
 *   messages from gcc involving two-parameter templates with default values
 *   are a mess, and forcing explicit definitions for each vector class, while
 *   tedious, makes it painfully obvious what is available.  But that's just
 *   my opinion...
 */

#include <stdio.h>
// #include <ps2s/debug.h>

/********************************************
 * Variables that represent VU0 registers.
 *
 * You can create a global variable that represents a VU0 special register by
 * listing the global as an input or output of any inline assembly that reads or
 * writes the register, respectively. By maintaining the correct order between
 * the inputs and outputs of the global, the compiler will order the writes
 * and reads of the VU0 register appropriately.
 *
 * This trick is mainly used here to allow use of the ACC by vector class methods.
 * Note that GCC doesn't know what's inside your inline asm statements, so you
 * don't actually have to write or read the global at all. Note also, you wouldn't
 * typically use this on a general VU0 register, since you wouldn't know which
 * assembly statements might use that register.  But if you keep some VU0 general
 * registers from the assembler (using compiler flag -mvu0-use-vf*-vf*) you could
 * use the same trick on your reserved registers.
 *
 * Binding the globals to GPR $0 seems to prevent any load and store side effects.
 */

register int vu0_ACC asm("$0");
// Q register is accessed via cfc2/ctc2 instructions, not as a global register
// We'll handle it differently in inline asm

/********************************************
 * constants
 */

namespace fields {
static const unsigned int none = 0;
static const unsigned int x    = 1 << 0;
static const unsigned int y    = 1 << 1;
static const unsigned int z    = 1 << 2;
static const unsigned int w    = 1 << 3;
}

/********************************************
 * base classes
 *
 * These store the attributes (traits) of the different
 * vector types and are used by vec_template (below).
 *
 * Note:  The vec128 member should really be declared in
 * the template since it is the same for all vectors, but
 * gcc will commit everything to memory if the template
 * inherits from an (eventually) empty class...
 */

typedef unsigned int vec128_t __attribute__((mode(TI), aligned(16)));

class x_base {
public:
    static const unsigned int broadcast_field = fields::x;
    static const unsigned int valid_fields    = fields::x;
    vec128_t vec128;
};

class y_base {
public:
    static const unsigned int broadcast_field = fields::y;
    static const unsigned int valid_fields    = fields::y;
    vec128_t vec128;
};

class z_base {
public:
    static const unsigned int broadcast_field = fields::z;
    static const unsigned int valid_fields    = fields::z;
    vec128_t vec128;
};

class w_base {
public:
    static const unsigned int broadcast_field = fields::w;
    static const unsigned int valid_fields    = fields::w;
    vec128_t vec128;
};

class xy_base {
public:
    static const unsigned int broadcast_field = fields::none;
    static const unsigned int valid_fields    = fields::x | fields::y;
    vec128_t vec128;
};

class xyz_base {
public:
    static const unsigned int broadcast_field = fields::none;
    static const unsigned int valid_fields    = fields::x | fields::y | fields::z;
    vec128_t vec128;
};

class xyzw_base {
public:
    static const unsigned int broadcast_field = fields::none;
    static const unsigned int valid_fields    = fields::x | fields::y | fields::z | fields::w;
    vec128_t vec128;
};

class point_base {
public:
    static const unsigned int broadcast_field = fields::none;
    static const unsigned int valid_fields    = fields::x | fields::y | fields::z;
    vec128_t vec128;
};

class vector_base {
public:
    static const unsigned int broadcast_field = fields::none;
    static const unsigned int valid_fields    = fields::x | fields::y | fields::z;
    vec128_t vec128;
};

/********************************************
 * vec_template
 *
 * This template is used to provide methods that can be generalized
 * for all vector classes.  Class-specific methods are found in the
 * vec_* classes, which are derived from the template.
 */

class mat_44;

template <class base_vec>
class vec_template : public base_vec {
    friend class mat_44;

public:
    // constructors

    inline vec_template() {}
    inline vec_template(const vec128_t initVal) { this->vec128 = initVal; }

    // accessors

    inline vec128_t get128() const { return this->vec128; }

    // mutators

    // this is called by all the operator = ()'s
    template <class rhs_type>
    inline void
    set(const rhs_type rhs)
    {
        vec128_t rhs_val = rhs.vec128;
        unsigned int mask = this->valid_fields & rhs.valid_fields;
        asm __volatile__(
            "qmtc2      %[rhs_val], $vf1        \n"
            "qmtc2      %[this_val], $vf2       \n"
            "vsub       $vf2, $vf0, $vf0        \n"
            "vmove      $vf2, $vf1              \n"
            "qmfc2      %[this_val], $vf2       \n"
            : [this_val] "+r"(this->vec128)
            : [rhs_val] "r"(rhs_val), [mask] "r"(mask)
            : "memory");
    }
    // make sure that for vectors of the same type it's a simple assignment
    inline void
    set(const vec_template<base_vec> rhs)
    {
        this->vec128 = rhs;
    }

    inline void
    set(float rhs)
    {
        asm __volatile__(
            "pextlw     %[this_val], %[rhs], %[rhs] \n"
            "pextlw     %[this_val], %[this_val], %[this_val] \n"
            : [this_val] "+r"(this->vec128)
            : [rhs] "r"(*reinterpret_cast<uint32_t*>(&rhs)));
    }

    inline void
    set_zero()
    {
        asm __volatile__(
            "qmtc2      %[this_val], $vf1        \n"
            "vsub       $vf1, $vf0, $vf0         \n"
            "qmfc2      %[this_val], $vf1        \n"
            : [this_val] "+r"(this->vec128)
            :
            : "memory");
    }

    static const unsigned int fields = (base_vec::broadcast_field << 4) | base_vec::valid_fields;

private:
    // for readability -- this class is the left-hand operand (*this)
    // in the following methods
    typedef vec_template<base_vec> lhs_type;

public:
    // math

    template <class rhs_type>
    inline lhs_type
    operator+(rhs_type rhs) const
    {
        const lhs_type& lhs = *this;
        vec128_t result;
        vec128_t lhs_val = lhs.vec128;
        vec128_t rhs_val = rhs.vec128;
        asm __volatile__(
            "qmtc2      %[lhs_val], $vf1        \n"
            "qmtc2      %[rhs_val], $vf2        \n"
            "vadd       $vf3, $vf1, $vf2        \n"
            "qmfc2      %[result], $vf3         \n"
            : [result] "=r"(result)
            : [lhs_val] "r"(lhs_val), [rhs_val] "r"(rhs_val)
            : "memory");
        return lhs_type(result);
    }
    inline lhs_type
    operator+(float rhs) const
    {
        vec128_t result;
        vec128_t lhs_val = this->vec128;
        uint32_t rhs_bits = *reinterpret_cast<uint32_t*>(&rhs);
        asm __volatile__(
            "qmtc2      %[lhs_val], $vf1        \n"
            "ctc2       %[rhs_bits], $vi21      \n"
            "vnop                               \n"
            "vaddi      $vf2, $vf1, $I          \n"
            "qmfc2      %[result], $vf2         \n"
            : [result] "=r"(result)
            : [lhs_val] "r"(lhs_val), [rhs_bits] "r"(rhs_bits)
            : "memory");
        return lhs_type(result);
    }
    template <class rhs_type>
    inline void
    operator+=(rhs_type rhs)
    {
        *this = *this + rhs;
    }

    template <class rhs_type>
    inline lhs_type
    operator-(rhs_type rhs) const
    {
        const lhs_type& lhs = *this;
        vec128_t result;
        vec128_t lhs_val = lhs.vec128;
        vec128_t rhs_val = rhs.vec128;
        asm __volatile__(
            "qmtc2      %[lhs_val], $vf1        \n"
            "qmtc2      %[rhs_val], $vf2        \n"
            "vsub       $vf3, $vf1, $vf2        \n"
            "qmfc2      %[result], $vf3         \n"
            : [result] "=r"(result)
            : [lhs_val] "r"(lhs_val), [rhs_val] "r"(rhs_val)
            : "memory");
        return lhs_type(result);
    }
    template <class rhs_type>
    inline void
    operator-=(rhs_type rhs)
    {
        *this = *this - rhs;
    }

    template <class rhs_type>
    inline lhs_type
    operator*(rhs_type rhs) const
    {
        const lhs_type& lhs = *this;
        vec128_t result;
        vec128_t lhs_val = lhs.vec128;
        vec128_t rhs_val = rhs.vec128;
        asm __volatile__(
            "qmtc2      %[lhs_val], $vf1        \n"
            "qmtc2      %[rhs_val], $vf2        \n"
            "vmul       $vf3, $vf1, $vf2        \n"
            "qmfc2      %[result], $vf3         \n"
            : [result] "=r"(result)
            : [lhs_val] "r"(lhs_val), [rhs_val] "r"(rhs_val)
            : "memory");
        return lhs_type(result);
    }
    inline lhs_type
    operator*(float rhs) const
    {
        vec128_t result;
        vec128_t this_val = this->vec128;
        uint32_t rhs_bits = *reinterpret_cast<uint32_t*>(&rhs);
        asm __volatile__(
            "qmtc2      %[this_val], $vf1        \n"
            "ctc2       %[rhs_bits], $vi21       \n"
            "vnop                               \n"
            "vmuli      $vf2, $vf1, $I           \n"
            "qmfc2      %[result], $vf2         \n"
            : [result] "=r"(result)
            : [this_val] "r"(this_val), [rhs_bits] "r"(rhs_bits)
            : "memory");
        return lhs_type(result);
    }
    template <class rhs_type>
    inline void
    operator*=(rhs_type rhs)
    {
        *this = *this * rhs;
    }

    template <class rhs_type>
    inline lhs_type
    operator/(rhs_type rhs) const
    {
        const lhs_type& lhs = *this;
        vec128_t result;
        vec128_t lhs_val = lhs.vec128;
        vec128_t rhs_val = rhs.vec128;
        asm __volatile__(
            "qmtc2      %[lhs_val], $vf1        \n"
            "qmtc2      %[rhs_val], $vf2        \n"
            "vdiv       $Q, $vf0w, $vf2x        \n"
            "vwaitq                             \n"
            "vmulq      $vf3, $vf1, $Q          \n"
            "qmfc2      %[result], $vf3         \n"
            : [result] "=r"(result)
            : [lhs_val] "r"(lhs_val), [rhs_val] "r"(rhs_val)
            : "memory");
        return lhs_type(result);
    }
    inline lhs_type
    operator/(float rhs) const
    {
        return *this * (1.0f / rhs);
    }
    template <class rhs_type>
    inline void
    operator/=(rhs_type rhs)
    {
        *this = *this / rhs;
    }
    inline void
    operator/=(float rhs)
    {
        *this = *this / rhs;
    }

    inline lhs_type
    operator-() const
    {
        vec128_t result;
        vec128_t this_val = this->vec128;
        asm __volatile__(
            "qmtc2      %[this_val], $vf1        \n"
            "vsuba      $ACC, $vf0, $vf0        \n"
            "vmsubw     $vf2, $vf1, $vf0w       \n"
            "qmfc2      %[result], $vf2         \n"
            : [result] "=r"(result), "=r"(vu0_ACC)
            : [this_val] "r"(this_val)
            : "memory");
        return lhs_type(result);
    }

    // negate for vectors with no w field.

    inline lhs_type
    no_w_negate() const
    {
        vec128_t result;
        vec128_t this_val = this->vec128;
        asm __volatile__(
            "qmtc2      %[this_val], $vf1        \n"
            "vsub       $vf2, $vf0, $vf1        \n"
            "qmfc2      %[result], $vf2         \n"
            : [result] "=r"(result)
            : [this_val] "r"(this_val)
            : "memory");
        return lhs_type(result);
    }

    // add and multiply-add functions.
    // You can write the accumulator in one method and add or
    // madd to it in another - data dependency is respected
    // due to the vu0_ACC global (see declaration above)�
    // However, you are responsible for matching the type
    // written to and read from the accumulator - use with
    // care.

    // to_a (to accumulator): ACC = this

    inline void
    to_a() const
    {
        vec128_t this_val = this->vec128;
        asm __volatile__(
            "qmtc2      %[this_val], $vf1        \n"
            // Set ACC to this vector: ACC = vf1
            // Use vmulaw: ACC = ACC + vf1 * vf0w = ACC + vf1 * 1.0
            // To set ACC = vf1 (not add), we need ACC to be 0 first
            // Use vmula to set ACC = vf1 * (1,1,1,1)
            // Create (1,1,1,1) by using vf0w which is 1.0
            // Actually, vmula sets ACC = src1 * src2 (not ACC + src1 * src2)
            // So vmula $ACC, $vf1, $vf0 does: ACC = vf1 * vf0
            // But vf0 is (0,0,0,1), so we'd get (0,0,0,vf1.w)
            // Need to use vmove to broadcast vf0w or use a different approach
            // Let's use vmulaw with zero ACC: if ACC is 0, then ACC = 0 + vf1 * 1.0 = vf1
            // But ACC might not be 0. Let's use vmula with a vector of ones
            // Actually, simplest: use vmove to copy vf1 to a temp, then use vmula
            "vmove      $vf2, $vf1               \n"  // vf2 = vf1
            "vmulaw     $ACC, $vf2, $vf0         \n"  // ACC = ACC + vf2 * vf0w = ACC + vf1 * 1.0
            // If ACC is not zero, this won't work. Let's zero ACC first using vmula with zero
            "vsub       $vf3, $vf0, $vf0         \n"  // vf3 = 0
            "vmula      $ACC, $vf3, $vf3         \n"  // ACC = vf3 * vf3 = 0 * 0 = 0
            "vmulaw     $ACC, $vf1, $vf0         \n"  // ACC = ACC + vf1 * vf0w = 0 + vf1 * 1.0 = vf1
            "cfc2       %[acc], $vi16            \n"
            : [acc] "=r"(vu0_ACC)
            : [this_val] "r"(this_val)
            : "memory");
    }

    // from_a (from accumulator): this = ACC

    inline void
    from_a()
    {
        asm __volatile__(
            "ctc2       %[acc], $vi16            \n"
            // Read ACC: use vmaddw with zero to get ACC + 0 = ACC
            "vsub       $vf1, $vf0, $vf0         \n"
            "vmaddw     $vf1, $vf1, $vf0         \n"  // vf1 = ACC + vf1*vf0 = ACC + 0 = ACC
            "qmfc2      %[this_val], $vf1        \n"
            : [this_val] "+r"(this->vec128)
            : [acc] "r"(vu0_ACC)
            : "memory");
    }

    // mula (multiply, to accumulator): ACC = this * rhs

    template <class rhs_type>
    inline void
    mula(rhs_type rhs) const
    {
        const lhs_type& lhs = *this;
        vec128_t lhs_val = lhs.vec128;
        vec128_t rhs_val = rhs.vec128;
        asm __volatile__(
            "qmtc2      %[lhs_val], $vf1        \n"
            "qmtc2      %[rhs_val], $vf2        \n"
            "vmula      $ACC, $vf1, $vf2        \n"
            "cfc2       %[acc], $vi16           \n"
            : [acc] "=r"(vu0_ACC)
            : [lhs_val] "r"(lhs_val), [rhs_val] "r"(rhs_val)
            : "memory");
    }

    inline void
    mula(float rhs) const
    {
        vec128_t lhs_val = this->vec128;
        uint32_t rhs_bits = *reinterpret_cast<uint32_t*>(&rhs);
        asm __volatile__(
            "qmtc2      %[lhs_val], $vf1        \n"
            "ctc2       %[rhs_bits], $vi21     \n"
            "vnop                               \n"
            "vmulai     $ACC, $vf1, $I          \n"
            "cfc2       %[acc], $vi16           \n"
            : [acc] "=r"(vu0_ACC)
            : [lhs_val] "r"(lhs_val), [rhs_bits] "r"(rhs_bits)
            : "memory");
    }

    // aadd (accumulator add): result = ACC + this
    // asub (accumulator subtract): result = ACC - this

    inline lhs_type
    aadd() const
    {
        vec128_t result;
        vec128_t this_val = this->vec128;
        asm __volatile__(
            "ctc2       %[acc], $vi16            \n"
            "qmtc2      %[this_val], $vf1        \n"
            "vmaddw     $vf2, $vf1, $vf0         \n"
            "qmfc2      %[result], $vf2         \n"
            : [result] "=r"(result)
            : [this_val] "r"(this_val), [acc] "r"(vu0_ACC)
            : "memory");
        return lhs_type(result);
    }

    inline lhs_type
    asub() const
    {
        vec128_t result;
        vec128_t this_val = this->vec128;
        asm __volatile__(
            "ctc2       %[acc], $vi16            \n"
            "qmtc2      %[this_val], $vf1        \n"
            "vmsubw     $vf2, $vf1, $vf0         \n"
            "qmfc2      %[result], $vf2         \n"
            : [result] "=r"(result)
            : [this_val] "r"(this_val), [acc] "r"(vu0_ACC)
            : "memory");
        return lhs_type(result);
    }

    // adda (add, to accumulator): ACC = this + rhs
    // suba (subtract, to accumulator): ACC = this - rhs

    template <class rhs_type>
    inline void
    adda(rhs_type rhs) const
    {
        const lhs_type& lhs = *this;
        vec128_t lhs_val = lhs.vec128;
        vec128_t rhs_val = rhs.vec128;
        asm __volatile__(
            "qmtc2      %[lhs_val], $vf1        \n"
            "qmtc2      %[rhs_val], $vf2        \n"
            "vadda      $ACC, $vf1, $vf2        \n"
            "cfc2       %[acc], $vi16           \n"
            : [acc] "=r"(vu0_ACC)
            : [lhs_val] "r"(lhs_val), [rhs_val] "r"(rhs_val)
            : "memory");
    }

    inline void
    adda(float rhs) const
    {
        vec128_t lhs_val = this->vec128;
        uint32_t rhs_bits = *reinterpret_cast<uint32_t*>(&rhs);
        asm __volatile__(
            "qmtc2      %[lhs_val], $vf1        \n"
            "ctc2       %[rhs_bits], $vi21     \n"
            "vnop                               \n"
            "vaddai     $ACC, $vf1, $I          \n"
            "cfc2       %[acc], $vi16           \n"
            : [acc] "=r"(vu0_ACC)
            : [lhs_val] "r"(lhs_val), [rhs_bits] "r"(rhs_bits)
            : "memory");
    }

    template <class rhs_type>
    inline void
    suba(rhs_type rhs) const
    {
        const lhs_type& lhs = *this;
        vec128_t lhs_val = lhs.vec128;
        vec128_t rhs_val = rhs.vec128;
        asm __volatile__(
            "qmtc2      %[lhs_val], $vf1        \n"
            "qmtc2      %[rhs_val], $vf2        \n"
            "vsuba      $ACC, $vf1, $vf2        \n"
            "cfc2       %[acc], $vi16           \n"
            : [acc] "=r"(vu0_ACC)
            : [lhs_val] "r"(lhs_val), [rhs_val] "r"(rhs_val)
            : "memory");
    }

    inline void
    suba(float rhs) const
    {
        vec128_t lhs_val = this->vec128;
        uint32_t rhs_bits = *reinterpret_cast<uint32_t*>(&rhs);
        asm __volatile__(
            "qmtc2      %[lhs_val], $vf1        \n"
            "ctc2       %[rhs_bits], $vi21     \n"
            "vnop                               \n"
            "vsubai     $ACC, $vf1, $I          \n"
            "cfc2       %[acc], $vi16           \n"
            : [acc] "=r"(vu0_ACC)
            : [lhs_val] "r"(lhs_val), [rhs_bits] "r"(rhs_bits)
            : "memory");
    }

    // aadda (accumulator add, to accumulator): ACC = ACC + this
    // asuba (accumulator subtract, to accumulator): ACC = ACC - this

    inline lhs_type
    aadda() const
    {
        // Add this vector to ACC: ACC = ACC + this
        // Then return ACC
        vec128_t this_val = this->vec128;
        vec128_t result;
        asm __volatile__(
            "ctc2       %[acc], $vi16            \n"
            "qmtc2      %[this_val], $vf1        \n"
            // First, read ACC into vf2: vf2 = ACC
            "vsub       $vf2, $vf0, $vf0         \n"  // vf2 = 0
            "vmaddw     $vf2, $vf2, $vf0         \n"  // vf2 = ACC + vf2 * vf0w = ACC + 0 = ACC
            // Now add this vector: ACC = vf2 + vf1 = ACC + this
            "vadda      $ACC, $vf2, $vf1         \n"  // ACC = vf2 + vf1 = (old ACC) + this
            "cfc2       %[acc], $vi16            \n"
            // Read result from ACC
            "vsub       $vf3, $vf0, $vf0         \n"  // vf3 = 0
            "vmaddw     $vf3, $vf3, $vf0         \n"  // vf3 = ACC + 0 = ACC
            "qmfc2      %[result], $vf3          \n"
            : [result] "=r"(result), [acc] "+r"(vu0_ACC)
            : [this_val] "r"(this_val)
            : "memory");
        return lhs_type(result);
    }

    inline lhs_type
    asuba() const
    {
        vec128_t result;
        vec128_t this_val = this->vec128;
        asm __volatile__(
            "ctc2       %[acc], $vi16            \n"
            "qmtc2      %[this_val], $vf1        \n"
            // First, read ACC into vf2: vf2 = ACC
            "vsub       $vf2, $vf0, $vf0         \n"  // vf2 = 0
            "vmaddw     $vf2, $vf2, $vf0         \n"  // vf2 = ACC + 0 = ACC
            // Now subtract this vector: ACC = vf2 - vf1 = ACC - this
            "vsuba      $ACC, $vf2, $vf1         \n"  // ACC = vf2 - vf1 = (old ACC) - this
            "cfc2       %[acc], $vi16            \n"
            // Read result from ACC
            "vsub       $vf3, $vf0, $vf0         \n"  // vf3 = 0
            "vmaddw     $vf3, $vf3, $vf0         \n"  // vf3 = ACC + 0 = ACC
            "qmfc2      %[result], $vf3          \n"
            : [result] "=r"(result), [acc] "+r"(vu0_ACC)
            : [this_val] "r"(this_val)
            : "memory");
        return lhs_type(result);
    }

    // madd (multiply, add with accumulator): result = ACC + this * rhs
    // msub (multiply, subtract from accumulator): result = ACC - this * rhs

    template <class rhs_type>
    inline lhs_type
    madd(rhs_type rhs) const
    {
        vec128_t result;
        vec128_t this_val = this->vec128;
        vec128_t rhs_val = rhs.vec128;
        asm __volatile__(
            "ctc2       %[acc], $vi16            \n"
            "qmtc2      %[this_val], $vf1        \n"
            "qmtc2      %[rhs_val], $vf2         \n"
            "vmadd      $vf3, $vf1, $vf2         \n"
            "qmfc2      %[result], $vf3         \n"
            : [result] "=r"(result)
            : [this_val] "r"(this_val), [rhs_val] "r"(rhs_val), [acc] "r"(vu0_ACC)
            : "memory");
        return lhs_type(result);
    }

    inline lhs_type
    madd(float rhs) const
    {
        vec128_t result;
        vec128_t this_val = this->vec128;
        uint32_t rhs_bits = *reinterpret_cast<uint32_t*>(&rhs);
        asm __volatile__(
            "ctc2       %[acc], $vi16            \n"
            "qmtc2      %[this_val], $vf1        \n"
            "ctc2       %[rhs_bits], $vi21       \n"
            "vnop                               \n"
            "vmaddi     $vf2, $vf1, $I           \n"
            "qmfc2      %[result], $vf2         \n"
            : [result] "=r"(result)
            : [this_val] "r"(this_val), [rhs_bits] "r"(rhs_bits), [acc] "r"(vu0_ACC)
            : "memory");
        return lhs_type(result);
    }

    template <class rhs_type>
    inline lhs_type
    msub(rhs_type rhs) const
    {
        vec128_t result;
        vec128_t this_val = this->vec128;
        vec128_t rhs_val = rhs.vec128;
        asm __volatile__(
            "ctc2       %[acc], $vi16            \n"
            "qmtc2      %[this_val], $vf1        \n"
            "qmtc2      %[rhs_val], $vf2         \n"
            "vmsub      $vf3, $vf1, $vf2         \n"
            "qmfc2      %[result], $vf3         \n"
            : [result] "=r"(result)
            : [this_val] "r"(this_val), [rhs_val] "r"(rhs_val), [acc] "r"(vu0_ACC)
            : "memory");
        return lhs_type(result);
    }

    inline lhs_type
    msub(float rhs) const
    {
        vec128_t result;
        vec128_t this_val = this->vec128;
        uint32_t rhs_bits = *reinterpret_cast<uint32_t*>(&rhs);
        asm __volatile__(
            "ctc2       %[acc], $vi16            \n"
            "qmtc2      %[this_val], $vf1        \n"
            "ctc2       %[rhs_bits], $vi21       \n"
            "vnop                               \n"
            "vmsubi     $vf2, $vf1, $I           \n"
            "qmfc2      %[result], $vf2         \n"
            : [result] "=r"(result)
            : [this_val] "r"(this_val), [rhs_bits] "r"(rhs_bits), [acc] "r"(vu0_ACC)
            : "memory");
        return lhs_type(result);
    }

    // madda (multiply, add with accumulator, to accumulator): ACC = ACC + this * rhs
    // msuba (multiply, subtract from accumulator, to accumulator): ACC = ACC - this * rhs

    template <class rhs_type>
    inline void
    madda(rhs_type rhs) const
    {
        const lhs_type& lhs = *this;
        vec128_t lhs_val = lhs.vec128;
        vec128_t rhs_val = rhs.vec128;
        asm __volatile__(
            "ctc2       %[acc], $vi16            \n"
            "qmtc2      %[lhs_val], $vf1         \n"
            "qmtc2      %[rhs_val], $vf2         \n"
            "vmadda     $ACC, $vf1, $vf2         \n"
            "cfc2       %[acc], $vi16            \n"
            : [acc] "+r"(vu0_ACC)
            : [lhs_val] "r"(lhs_val), [rhs_val] "r"(rhs_val)
            : "memory");
    }

    inline void
    madda(float rhs) const
    {
        vec128_t lhs_val = this->vec128;
        uint32_t rhs_bits = *reinterpret_cast<uint32_t*>(&rhs);
        asm __volatile__(
            "ctc2       %[acc], $vi16            \n"
            "qmtc2      %[lhs_val], $vf1         \n"
            "ctc2       %[rhs_bits], $vi21       \n"
            "vnop                               \n"
            "vmaddai    $ACC, $vf1, $I           \n"
            "cfc2       %[acc], $vi16            \n"
            : [acc] "+r"(vu0_ACC)
            : [lhs_val] "r"(lhs_val), [rhs_bits] "r"(rhs_bits)
            : "memory");
    }

    template <class rhs_type>
    inline void
    msuba(rhs_type rhs) const
    {
        const lhs_type& lhs = *this;
        vec128_t lhs_val = lhs.vec128;
        vec128_t rhs_val = rhs.vec128;
        asm __volatile__(
            "ctc2       %[acc], $vi16            \n"
            "qmtc2      %[lhs_val], $vf1         \n"
            "qmtc2      %[rhs_val], $vf2         \n"
            "vmsuba     $ACC, $vf1, $vf2         \n"
            "cfc2       %[acc], $vi16            \n"
            : [acc] "+r"(vu0_ACC)
            : [lhs_val] "r"(lhs_val), [rhs_val] "r"(rhs_val)
            : "memory");
    }

    inline void
    msuba(float rhs) const
    {
        vec128_t lhs_val = this->vec128;
        uint32_t rhs_bits = *reinterpret_cast<uint32_t*>(&rhs);
        asm __volatile__(
            "ctc2       %[acc], $vi16            \n"
            "qmtc2      %[lhs_val], $vf1         \n"
            "ctc2       %[rhs_bits], $vi21       \n"
            "vnop                               \n"
            "vmsubai    $ACC, $vf1, $I           \n"
            "cfc2       %[acc], $vi16            \n"
            : [acc] "+r"(vu0_ACC)
            : [lhs_val] "r"(lhs_val), [rhs_bits] "r"(rhs_bits)
            : "memory");
    }

    inline lhs_type
    abs() const
    {
        vec128_t result;
        vec128_t this_val = this->vec128;
        asm __volatile__(
            "qmtc2      %[this_val], $vf1        \n"
            "vabs       $vf2, $vf1               \n"
            "qmfc2      %[result], $vf2         \n"
            : [result] "=r"(result)
            : [this_val] "r"(this_val)
            : "memory");
        return lhs_type(result);
    }

    inline lhs_type
    sign() const
    {
        vec128_t result;
        vec128_t this_val = this->vec128;
        uint32_t macflag;
        
        asm __volatile__(
            "qmtc2      %[this_val], $vf1        \n"
            // Get absolute value
            "vabs       $vf2, $vf1               \n"
            // Divide original by absolute value to get sign
            // For non-zero: sign = original / abs = ±1
            // For zero: division by zero will set MACflag
            "vdiv       $Q, $vf0w, $vf2x         \n"
            "vwaitq                              \n"
            "vmulq.x    $vf3, $vf1, $Q          \n"
            "vdiv       $Q, $vf0w, $vf2y         \n"
            "vwaitq                              \n"
            "vmulq.y    $vf3, $vf1, $Q          \n"
            "vdiv       $Q, $vf0w, $vf2z         \n"
            "vwaitq                              \n"
            "vmulq.z    $vf3, $vf1, $Q          \n"
            "vdiv       $Q, $vf0w, $vf2w         \n"
            "vwaitq                              \n"
            "vmulq.w    $vf3, $vf1, $Q          \n"
            // Check MACflag for each component to detect zero
            // If division by zero occurred, set that component to 0
            "cfc2       %[macflag], $vi17        \n"
            "andi       %[macflag], %[macflag], 8 \n"  // Check DZ (divide by zero) flag
            "bgtz       %[macflag], 1f           \n"
            "nop                                 \n"
            "qmfc2      %[result], $vf3         \n"
            "j          2f                       \n"
            "nop                                 \n"
            "1:                                  \n"
            // If zero detected, set result to zero
            "vsub       $vf3, $vf0, $vf0        \n"
            "qmfc2      %[result], $vf3         \n"
            "2:                                  \n"
            : [result] "=r"(result), [macflag] "=r"(macflag)
            : [this_val] "r"(this_val)
            : "memory");
        return lhs_type(result);
    }

    template <class rhs_type>
    inline lhs_type
    max(rhs_type rhs) const
    {
        vec128_t result;
        vec128_t this_val = this->vec128;
        vec128_t rhs_val = rhs.vec128;
        asm __volatile__(
            "qmtc2      %[this_val], $vf1        \n"
            "qmtc2      %[rhs_val], $vf2          \n"
            "vmax       $vf3, $vf1, $vf2         \n"
            "qmfc2      %[result], $vf3         \n"
            : [result] "=r"(result)
            : [this_val] "r"(this_val), [rhs_val] "r"(rhs_val)
            : "memory");
        return lhs_type(result);
    }

    template <class rhs_type>
    inline lhs_type
    min(rhs_type rhs) const
    {
        vec128_t result;
        vec128_t this_val = this->vec128;
        vec128_t rhs_val = rhs.vec128;
        asm __volatile__(
            "qmtc2      %[this_val], $vf1        \n"
            "qmtc2      %[rhs_val], $vf2          \n"
            "vmini      $vf3, $vf1, $vf2         \n"
            "qmfc2      %[result], $vf3         \n"
            : [result] "=r"(result)
            : [this_val] "r"(this_val), [rhs_val] "r"(rhs_val)
            : "memory");
        return lhs_type(result);
    }

    inline void print(void) const
    {
        float x, y, z, w;
        vec128_t temp0;
        asm volatile(
            "mtsab	$0, 4		# get ready to shift right 4 bytes	\n"
            "mtc1	%5, %0		# x = value.x	\n"
            "qfsrv	%4, $0, %5 	# temp0 = value >> 8	\n"
            "mtc1	%4, %1		# y = value.y \n"
            "qfsrv	%4, $0, %4 	# temp0 >>= 8 \n"
            "mtc1	%4, %2		# z = value.z \n"
            "qfsrv	%4, $0, %4 	# temp0 >>= 8 \n"
            "mtc1	%4, %3		# w = value.w \n"
            : "=f"(x), "=f"(y), "=f"(z), "=f"(w), "=r"(temp0)
            : "r"(this->vec128));

        printf("(%f %f %f %f)\n", x, y, z, w);
    }

    inline void print(const char* vec_name) const
    {
        printf("%s: \n", vec_name);
        print();
    }
};

#endif // vector_common_h
