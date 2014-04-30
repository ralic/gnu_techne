/* Copyright (C) 2012 Papavasileiou Dimitris
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <math.h>
#include <stdlib.h>
#include <assert.h>

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include "array/array.h"

/* #define ARRAYMATH_COLUMN_MAJOR */

#define OP(FUNC, OPERATOR, TYPE)                                        \
    static void FUNC (TYPE *A, TYPE *B, TYPE *C, int n)                 \
    {                                                                   \
        int i;                                                          \
                                                                        \
        for (i = 0 ; i < n ; i += 1) {                                  \
            C[i] = A[i] OPERATOR B[i];                                  \
        }                                                               \
    }

#define OP_NORM_ADDSUB(FUNC, OPERATOR, TYPE)                            \
    static void FUNC (TYPE *A, TYPE *B, TYPE *C, int n, const double c)	\
    {                                                                   \
        int i;                                                          \
                                                                        \
        for (i = 0 ; i < n ; i += 1) {                                  \
            C[i] = A[i] OPERATOR B[i];                                  \
        }                                                               \
    }

#define OP_ADD(FUNC, TYPE) OP(FUNC, +, TYPE)
#define OP_SUB(FUNC, TYPE) OP(FUNC, -, TYPE)
#define OP_MUL(FUNC, TYPE) OP(FUNC, *, TYPE)
#define OP_DIV(FUNC, TYPE) OP(FUNC, /, TYPE)
#define OP_NADD(FUNC, TYPE) OP_NORM_ADDSUB(FUNC, +, TYPE)
#define OP_NSUB(FUNC, TYPE) OP_NORM_ADDSUB(FUNC, -, TYPE)

#define OP_NORM_MULDIV(FUNC, OPERATOR, TYPE)                            \
    static void FUNC (TYPE *A, TYPE *B, TYPE *C, int n, const double c)	\
    {                                                                   \
        int i;                                                          \
                                                                        \
        for (i = 0 ; i < n ; i += 1) {                                  \
            C[i] = A[i] OPERATOR (B[i] / c);                            \
        }                                                               \
    }

#define OP_NMUL(FUNC, TYPE) OP_NORM_MULDIV(FUNC, *, TYPE)
#define OP_NDIV(FUNC, TYPE) OP_NORM_MULDIV(FUNC, /, TYPE)

#define DEFINE_OPERATION(OPERATION, OPERATOR)                           \
    OP_##OPERATOR(OPERATION##_doubles, double)                          \
    OP_##OPERATOR(OPERATION##_floats, float)                            \
                                                                        \
    OP_##OPERATOR(OPERATION##_ulongs, unsigned long)                    \
    OP_##OPERATOR(OPERATION##_longs, signed long)                       \
    OP_##OPERATOR(OPERATION##_uints, unsigned int)                      \
    OP_##OPERATOR(OPERATION##_ints, signed int)                         \
    OP_##OPERATOR(OPERATION##_ushorts, unsigned short)                  \
    OP_##OPERATOR(OPERATION##_shorts, signed short)                     \
    OP_##OPERATOR(OPERATION##_uchars, unsigned char)                    \
    OP_##OPERATOR(OPERATION##_chars, signed char)                       \
                                                                        \
    OP_N##OPERATOR(OPERATION##_nulongs, unsigned long)                  \
    OP_N##OPERATOR(OPERATION##_nlongs, signed long)                     \
    OP_N##OPERATOR(OPERATION##_nuints, unsigned int)                    \
    OP_N##OPERATOR(OPERATION##_nints, signed int)                       \
    OP_N##OPERATOR(OPERATION##_nushorts, unsigned short)                \
    OP_N##OPERATOR(OPERATION##_nshorts, signed short)                   \
    OP_N##OPERATOR(OPERATION##_nuchars, unsigned char)                  \
    OP_N##OPERATOR(OPERATION##_nchars, signed char)                     \
                                                                        \
    int arraymath_##OPERATION (lua_State *L)                            \
    {									\
        array_Array *A, *B, *C;						\
        int j, l;							\
                                                                        \
        A = lua_touserdata (L, -2);					\
        B = lua_touserdata (L, -1);					\
                                                                        \
        C = array_createarrayv (L, A->type, NULL, A->rank, A->size);	\
                                                                        \
        for (j = 0, l = 1; j < A->rank ; l *= A->size[j], j += 1);	\
                                                                        \
        switch (A->type) {						\
        case ARRAY_TDOUBLE:						\
            OPERATION##_doubles (A->values.doubles, B->values.doubles,	\
                                 C->values.doubles, l);                 \
            break;							\
        case ARRAY_TFLOAT:						\
            OPERATION##_floats (A->values.floats, B->values.floats,	\
                                C->values.floats, l);                   \
            break;							\
        case ARRAY_TULONG:						\
            OPERATION##_ulongs (A->values.ulongs, B->values.ulongs,	\
                                C->values.ulongs, l);                   \
            break;							\
        case ARRAY_TNULONG:						\
            OPERATION##_nulongs (A->values.ulongs, B->values.ulongs,	\
                                 C->values.ulongs, l, ULONG_MAX);       \
            break;							\
        case ARRAY_TLONG:						\
            OPERATION##_longs (A->values.longs, B->values.longs,	\
                               C->values.longs, l);			\
            break;							\
        case ARRAY_TNLONG:						\
            OPERATION##_nlongs (A->values.longs, B->values.longs,	\
                                C->values.longs, l, LONG_MAX);          \
            break;							\
        case ARRAY_TUINT:						\
            OPERATION##_uints (A->values.uints, B->values.uints,	\
                               C->values.uints, l);			\
            break;							\
        case ARRAY_TNUINT:						\
            OPERATION##_nuints (A->values.uints, B->values.uints,	\
                                C->values.uints, l, UINT_MAX);          \
            break;							\
        case ARRAY_TINT:						\
            OPERATION##_ints (A->values.ints, B->values.ints,		\
                              C->values.ints, l);			\
            break;							\
        case ARRAY_TNINT:						\
            OPERATION##_nints (A->values.ints, B->values.ints,		\
                               C->values.ints, l, INT_MAX);             \
            break;							\
        case ARRAY_TUSHORT:						\
            OPERATION##_ushorts (A->values.ushorts, B->values.ushorts,	\
                                 C->values.ushorts, l);                 \
            break;							\
        case ARRAY_TNUSHORT:						\
            OPERATION##_nushorts (A->values.ushorts, B->values.ushorts,	\
                                  C->values.ushorts, l, USHRT_MAX);     \
            break;							\
        case ARRAY_TSHORT:						\
            OPERATION##_shorts (A->values.shorts, B->values.shorts,	\
                                C->values.shorts, l);                   \
            break;							\
        case ARRAY_TNSHORT:						\
            OPERATION##_nshorts (A->values.shorts, B->values.shorts,	\
                                 C->values.shorts, l, SHRT_MAX);        \
            break;							\
        case ARRAY_TUCHAR:						\
            OPERATION##_uchars (A->values.uchars, B->values.uchars,	\
                                C->values.uchars, l);                   \
            break;							\
        case ARRAY_TNUCHAR:						\
            OPERATION##_nuchars (A->values.uchars, B->values.uchars,	\
                                 C->values.uchars, l, UCHAR_MAX);       \
            break;							\
        case ARRAY_TCHAR:						\
            OPERATION##_chars (A->values.chars, B->values.chars,	\
                               C->values.chars, l);			\
            break;							\
        case ARRAY_TNCHAR:						\
            OPERATION##_nchars (A->values.chars, B->values.chars,	\
                                C->values.chars, l, CHAR_MAX);          \
            break;							\
        }								\
                                                                        \
        return 1;							\
    }

DEFINE_OPERATION(add, ADD)
DEFINE_OPERATION(multiply, MUL)
DEFINE_OPERATION(subtract, SUB)
DEFINE_OPERATION(divide, DIV)

#define COMPARE(FUNC, OPERATOR, TYPE)                                   \
    static void FUNC (TYPE *A, TYPE *B, int *C, int n)                  \
    {                                                                   \
        int i;                                                          \
                                                                        \
        for (i = 0 ; i < n ; i += 1) {                                  \
            C[i] = A[i] OPERATOR B[i];                                  \
        }                                                               \
    }

#define DEFINE_COMPARISON(OPERATION, OPERATOR)                          \
    COMPARE(OPERATION##_doubles, OPERATOR, double)                      \
    COMPARE(OPERATION##_floats, OPERATOR, float)                        \
                                                                        \
    COMPARE(OPERATION##_ulongs, OPERATOR, unsigned long)                \
    COMPARE(OPERATION##_longs, OPERATOR, signed long)                   \
    COMPARE(OPERATION##_uints, OPERATOR, unsigned int)                  \
    COMPARE(OPERATION##_ints, OPERATOR, signed int)                     \
    COMPARE(OPERATION##_ushorts, OPERATOR, unsigned short)              \
    COMPARE(OPERATION##_shorts, OPERATOR, signed short)                 \
    COMPARE(OPERATION##_uchars, OPERATOR, unsigned char)                \
    COMPARE(OPERATION##_chars, OPERATOR, signed char)                   \
                                                                        \
    int arraymath_##OPERATION (lua_State *L)                            \
    {									\
        array_Array *A, *B, *C;						\
        int j, l;							\
                                                                        \
        A = lua_touserdata (L, -2);					\
        B = lua_touserdata (L, -1);					\
                                                                        \
        C = array_createarrayv (L, ARRAY_TINT, NULL, A->rank, A->size);	\
                                                                        \
        for (j = 0, l = 1; j < A->rank ; l *= A->size[j], j += 1);	\
                                                                        \
        switch (abs(A->type)) {						\
        case ARRAY_TDOUBLE:						\
            OPERATION##_doubles (A->values.doubles, B->values.doubles,	\
                                 C->values.ints, l);                    \
                break;							\
        case ARRAY_TFLOAT:						\
            OPERATION##_floats (A->values.floats, B->values.floats,	\
                                C->values.ints, l);                     \
                break;							\
        case ARRAY_TULONG:						\
            OPERATION##_ulongs (A->values.ulongs, B->values.ulongs,	\
                                C->values.ints, l);                     \
                break;							\
        case ARRAY_TLONG:						\
            OPERATION##_longs (A->values.longs, B->values.longs,	\
                               C->values.ints, l);			\
                break;							\
        case ARRAY_TUINT:						\
            OPERATION##_uints (A->values.uints, B->values.uints,	\
                               C->values.ints, l);			\
                break;							\
        case ARRAY_TINT:						\
            OPERATION##_ints (A->values.ints, B->values.ints,		\
                              C->values.ints, l);			\
                break;							\
        case ARRAY_TUSHORT:						\
            OPERATION##_ushorts (A->values.ushorts, B->values.ushorts,	\
                                 C->values.ints, l);                    \
                break;							\
        case ARRAY_TSHORT:						\
            OPERATION##_shorts (A->values.shorts, B->values.shorts,	\
                                C->values.ints, l);                     \
                break;							\
        case ARRAY_TUCHAR:						\
            OPERATION##_uchars (A->values.uchars, B->values.uchars,	\
                                C->values.ints, l);                     \
                break;							\
        case ARRAY_TCHAR:						\
            OPERATION##_chars (A->values.chars, B->values.chars,	\
                               C->values.ints, l);			\
                break;							\
        }								\
                                                                        \
        return 1;							\
    }

DEFINE_COMPARISON(greater, >)
DEFINE_COMPARISON(greaterequal, >=)
DEFINE_COMPARISON(less, <)
DEFINE_COMPARISON(lessequal, <=)
DEFINE_COMPARISON(equal, ==)

#define SCALE(FUNC, TYPE)					\
    static void FUNC (TYPE *A, lua_Number c, TYPE *B, int n)	\
    {								\
        int i;							\
                                                                \
        for (i = 0 ; i < n ; i += 1) {				\
            B[i] = c * A[i];                                    \
        }							\
    }

SCALE(scale_doubles, double)
SCALE(scale_floats, float)
SCALE(scale_ulongs, unsigned long)
SCALE(scale_longs, signed long)
SCALE(scale_uints, unsigned int)
SCALE(scale_ints, signed int)
SCALE(scale_ushorts, unsigned short)
SCALE(scale_shorts, signed short)
SCALE(scale_uchars, unsigned char)
SCALE(scale_chars, signed char)

int arraymath_scale (lua_State *L)
{
    array_Array *A, *B;
    double c;
    int j, l;

    A = lua_touserdata (L, -2);
    c = lua_tonumber (L, -1);

    for (j = 0 ,l = 1; j < A->rank ; l *= A->size[j], j += 1);

    B = array_createarrayv (L, A->type, NULL, A->rank, A->size);

    switch (abs(A->type)) {
    case ARRAY_TDOUBLE:
        scale_doubles (A->values.doubles, c, B->values.doubles, l);
        break;
    case ARRAY_TFLOAT:
        scale_floats (A->values.floats, c, B->values.floats, l);
        break;
    case ARRAY_TULONG:
        scale_ulongs (A->values.ulongs, c, B->values.ulongs, l);
        break;
    case ARRAY_TLONG:
        scale_longs (A->values.longs, c, B->values.longs, l);
        break;
    case ARRAY_TUINT:
        scale_uints (A->values.uints, c, B->values.uints, l);
        break;
    case ARRAY_TINT:
        scale_ints (A->values.ints, c, B->values.ints, l);
        break;
    case ARRAY_TUSHORT:
        scale_ushorts (A->values.ushorts, c, B->values.ushorts, l);
        break;
    case ARRAY_TSHORT:
        scale_shorts (A->values.shorts, c, B->values.shorts, l);
        break;
    case ARRAY_TUCHAR:
        scale_uchars (A->values.uchars, c, B->values.uchars, l);
        break;
    case ARRAY_TCHAR:
        scale_chars (A->values.chars, c, B->values.chars, l);
        break;
    }

    return 1;
}

#define OFFSET(FUNC, TYPE)					\
    static void FUNC (TYPE *A, TYPE d, TYPE *B, int n)          \
    {								\
        int i;							\
                                                                \
        for (i = 0 ; i < n ; i += 1) {				\
            B[i] = d + A[i];					\
        }							\
    }

OFFSET(offset_doubles, double)
OFFSET(offset_floats, float)

OFFSET(offset_ulongs, unsigned long)
OFFSET(offset_longs, signed long)
OFFSET(offset_uints, unsigned int)
OFFSET(offset_ints, signed int)
OFFSET(offset_ushorts, unsigned short)
OFFSET(offset_shorts, signed short)
OFFSET(offset_uchars, unsigned char)
OFFSET(offset_chars, signed char)

int arraymath_offset (lua_State *L)
{
    array_Array *A, *B;
    lua_Number d;
    int j, l;

    A = lua_touserdata (L, -2);
    d = lua_tonumber (L, -1);

    for (j = 0, l = 1; j < A->rank ; l *= A->size[j], j += 1);

    B = array_createarrayv (L, A->type, NULL, A->rank, A->size);

    switch (A->type) {
    case ARRAY_TDOUBLE:
        offset_doubles (A->values.doubles, d, B->values.doubles, l);
        break;
    case ARRAY_TFLOAT:
        offset_floats (A->values.floats, d, B->values.floats, l);
        break;
    case ARRAY_TULONG:
        offset_ulongs (A->values.ulongs, d, B->values.ulongs, l);
        break;
    case ARRAY_TLONG:
        offset_longs (A->values.longs, d, B->values.longs, l);
        break;
    case ARRAY_TUINT:
        offset_uints (A->values.uints, d, B->values.uints, l);
        break;
    case ARRAY_TINT:
        offset_ints (A->values.ints, d, B->values.ints, l);
        break;
    case ARRAY_TUSHORT:
        offset_ushorts (A->values.ushorts, d, B->values.ushorts, l);
        break;
    case ARRAY_TSHORT:
        offset_shorts (A->values.shorts, d, B->values.shorts, l);
        break;
    case ARRAY_TUCHAR:
        offset_uchars (A->values.uchars, d, B->values.uchars, l);
        break;
    case ARRAY_TCHAR:
        offset_chars (A->values.chars, d, B->values.chars, l);
        break;
    case ARRAY_TNULONG:
        offset_ulongs (A->values.ulongs, d * ULONG_MAX, B->values.ulongs, l);
        break;
    case ARRAY_TNLONG:
        offset_longs (A->values.longs, d * LONG_MAX, B->values.longs, l);
        break;
    case ARRAY_TNUINT:
        offset_uints (A->values.uints, d * UINT_MAX, B->values.uints, l);
        break;
    case ARRAY_TNINT:
        offset_ints (A->values.ints, d * INT_MAX, B->values.ints, l);
        break;
    case ARRAY_TNUSHORT:
        offset_ushorts (A->values.ushorts, d * USHRT_MAX, B->values.ushorts, l);
        break;
    case ARRAY_TNSHORT:
        offset_shorts (A->values.shorts, d * SHRT_MAX, B->values.shorts, l);
        break;
    case ARRAY_TNUCHAR:
        offset_uchars (A->values.uchars, d * UCHAR_MAX, B->values.uchars, l);
        break;
    case ARRAY_TNCHAR:
        offset_chars (A->values.chars, d * CHAR_MAX, B->values.chars, l);
        break;
    }

    return 1;
}

int arraymath_scaleoffset (lua_State *L)
{
    array_Array *A, *B;
    lua_Number c, d;
    int j, l;

    A = lua_touserdata (L, -3);
    c = lua_tonumber (L, -2);
    d = lua_tonumber (L, -1);

    for (j = 0, l = 1; j < A->rank ; l *= A->size[j], j += 1);

    B = array_createarrayv (L, A->type, NULL, A->rank, A->size);

    switch (A->type) {
    case ARRAY_TDOUBLE:
        scale_doubles (A->values.doubles, c, B->values.doubles, l);
        offset_doubles (B->values.doubles, d, B->values.doubles, l);
        break;
    case ARRAY_TFLOAT:
        scale_floats (A->values.floats, c, B->values.floats, l);
        offset_floats (B->values.floats, d, B->values.floats, l);
        break;
    case ARRAY_TULONG:
        scale_ulongs (A->values.ulongs, c, B->values.ulongs, l);
        offset_ulongs (B->values.ulongs, d, B->values.ulongs, l);
        break;
    case ARRAY_TLONG:
        scale_longs (A->values.longs, c, B->values.longs, l);
        offset_longs (B->values.longs, d, B->values.longs, l);
        break;
    case ARRAY_TUINT:
        scale_uints (A->values.uints, c, B->values.uints, l);
        offset_uints (B->values.uints, d, B->values.uints, l);
        break;
    case ARRAY_TINT:
        scale_ints (A->values.ints, c, B->values.ints, l);
        offset_ints (B->values.ints, d, B->values.ints, l);
        break;
    case ARRAY_TUSHORT:
        scale_ushorts (A->values.ushorts, c, B->values.ushorts, l);
        offset_ushorts (B->values.ushorts, d, B->values.ushorts, l);
        break;
    case ARRAY_TSHORT:
        scale_shorts (A->values.shorts, c, B->values.shorts, l);
        offset_shorts (B->values.shorts, d, B->values.shorts, l);
        break;
    case ARRAY_TUCHAR:
        scale_uchars (A->values.uchars, c, B->values.uchars, l);
        offset_uchars (B->values.uchars, d, B->values.uchars, l);
        break;
    case ARRAY_TCHAR:
        scale_chars (A->values.chars, c, B->values.chars, l);
        offset_chars (B->values.chars, d, B->values.chars, l);
        break;
    case ARRAY_TNULONG:
        scale_ulongs (A->values.ulongs, c, B->values.ulongs, l);
        offset_ulongs (B->values.ulongs, d * ULONG_MAX, B->values.ulongs, l);
        break;
    case ARRAY_TNLONG:
        scale_longs (A->values.longs, c, B->values.longs, l);
        offset_longs (B->values.longs, d * LONG_MAX, B->values.longs, l);
        break;
    case ARRAY_TNUINT:
        scale_uints (A->values.uints, c, B->values.uints, l);
        offset_uints (B->values.uints, d * UINT_MAX, B->values.uints, l);
        break;
    case ARRAY_TNINT:
        scale_ints (A->values.ints, c, B->values.ints, l);
        offset_ints (B->values.ints, d * INT_MAX, B->values.ints, l);
        break;
    case ARRAY_TNUSHORT:
        scale_ushorts (A->values.ushorts, c, B->values.ushorts, l);
        offset_ushorts (B->values.ushorts, d * USHRT_MAX, B->values.ushorts, l);
        break;
    case ARRAY_TNSHORT:
        scale_shorts (A->values.shorts, c, B->values.shorts, l);
        offset_shorts (B->values.shorts, d * SHRT_MAX, B->values.shorts, l);
        break;
    case ARRAY_TNUCHAR:
        scale_uchars (A->values.uchars, c, B->values.uchars, l);
        offset_uchars (B->values.uchars, d * UCHAR_MAX, B->values.uchars, l);
        break;
    case ARRAY_TNCHAR:
        scale_chars (A->values.chars, c, B->values.chars, l);
        offset_chars (B->values.chars, d * CHAR_MAX, B->values.chars, l);
        break;
    }

    return 1;
}

#define CLAMP(FUNC, TYPE)					\
    static void FUNC (TYPE *A, lua_Number a, lua_Number b, TYPE *B, int n) \
    {								\
        int i;							\
                                                                \
        for (i = 0 ; i < n ; i += 1) {				\
            B[i] = A[i] <= a ? a : (A[i] >= b ? b : A[i]);      \
        }							\
    }

CLAMP(clamp_doubles, double)
CLAMP(clamp_floats, float)
CLAMP(clamp_ulongs, unsigned long)
CLAMP(clamp_longs, signed long)
CLAMP(clamp_uints, unsigned int)
CLAMP(clamp_ints, signed int)
CLAMP(clamp_ushorts, unsigned short)
CLAMP(clamp_shorts, signed short)
CLAMP(clamp_uchars, unsigned char)
CLAMP(clamp_chars, signed char)

int arraymath_clamp (lua_State *L)
{
    array_Array *A, *B;
    lua_Number a, b;
    int j, l;

    A = lua_touserdata (L, -3);
    a = lua_tonumber (L, -2);
    b = lua_tonumber (L, -1);

    for (j = 0 ,l = 1; j < A->rank ; l *= A->size[j], j += 1);

    B = array_createarrayv (L, A->type, NULL, A->rank, A->size);

    switch (abs(A->type)) {
    case ARRAY_TDOUBLE:
        clamp_doubles (A->values.doubles, a, b, B->values.doubles, l);
        break;
    case ARRAY_TFLOAT:
        clamp_floats (A->values.floats, a, b, B->values.floats, l);
        break;
    case ARRAY_TULONG:
        clamp_ulongs (A->values.ulongs, a, b, B->values.ulongs, l);
        break;
    case ARRAY_TLONG:
        clamp_longs (A->values.longs, a, b, B->values.longs, l);
        break;
    case ARRAY_TUINT:
        clamp_uints (A->values.uints, a, b, B->values.uints, l);
        break;
    case ARRAY_TINT:
        clamp_ints (A->values.ints, a, b, B->values.ints, l);
        break;
    case ARRAY_TUSHORT:
        clamp_ushorts (A->values.ushorts, a, b, B->values.ushorts, l);
        break;
    case ARRAY_TSHORT:
        clamp_shorts (A->values.shorts, a, b, B->values.shorts, l);
        break;
    case ARRAY_TUCHAR:
        clamp_uchars (A->values.uchars, a, b, B->values.uchars, l);
        break;
    case ARRAY_TCHAR:
        clamp_chars (A->values.chars, a, b, B->values.chars, l);
        break;
    }

    return 1;
}

#define RAISE(FUNC, TYPE)                                               \
    static void FUNC (TYPE *A, lua_Number e, TYPE *B, int n)            \
    {                                                                   \
        int i;                                                          \
                                                                        \
        for (i = 0 ; i < n ; i += 1) {                                  \
            B[i] = pow(A[i], e);                                        \
        }                                                               \
    }

#define RAISE_NORM(FUNC, TYPE)                                          \
    static void FUNC (TYPE *A, lua_Number e, TYPE *B, int n,            \
                      const double c)                                   \
    {                                                                   \
        int i;                                                          \
                                                                        \
        for (i = 0 ; i < n ; i += 1) {                                  \
            B[i] = pow(A[i] / c, e) * c;                                \
        }                                                               \
    }

RAISE(raise_doubles, double)
RAISE(raise_floats, float)

RAISE(raise_ulongs, unsigned long)
RAISE(raise_longs, signed long)
RAISE(raise_uints, unsigned int)
RAISE(raise_ints, signed int)
RAISE(raise_ushorts, unsigned short)
RAISE(raise_shorts, signed short)
RAISE(raise_uchars, unsigned char)
RAISE(raise_chars, signed char)

RAISE_NORM(raise_nulongs, unsigned long)
RAISE_NORM(raise_nlongs, signed long)
RAISE_NORM(raise_nuints, unsigned int)
RAISE_NORM(raise_nints, signed int)
RAISE_NORM(raise_nushorts, unsigned short)
RAISE_NORM(raise_nshorts, signed short)
RAISE_NORM(raise_nuchars, unsigned char)
RAISE_NORM(raise_nchars, signed char)

int arraymath_raise (lua_State *L)
{
    array_Array *A, *B;
    double c;
    int j, l;

    A = lua_touserdata (L, -2);
    c = lua_tonumber (L, -1);

    for (j = 0, l = 1; j < A->rank ; l *= A->size[j], j += 1);

    B = array_createarrayv (L, A->type, NULL, A->rank, A->size);

    switch (A->type) {
    case ARRAY_TDOUBLE:
        raise_doubles (A->values.doubles, c, B->values.doubles, l);
        break;
    case ARRAY_TFLOAT:
        raise_floats (A->values.floats, c, B->values.floats, l);
        break;
    case ARRAY_TULONG:
        raise_ulongs (A->values.ulongs, c, B->values.ulongs, l);
        break;
    case ARRAY_TNULONG:
        raise_nulongs (A->values.ulongs, c, B->values.ulongs, l, ULONG_MAX);
        break;
    case ARRAY_TLONG:
        raise_longs (A->values.longs, c, B->values.longs, l);
        break;
    case ARRAY_TNLONG:
        raise_nlongs (A->values.longs, c, B->values.longs, l, LONG_MAX);
        break;
    case ARRAY_TUINT:
        raise_uints (A->values.uints, c, B->values.uints, l);
        break;
    case ARRAY_TNUINT:
        raise_nuints (A->values.uints, c, B->values.uints, l, UINT_MAX);
        break;
    case ARRAY_TINT:
        raise_ints (A->values.ints, c, B->values.ints, l);
        break;
    case ARRAY_TNINT:
        raise_nints (A->values.ints, c, B->values.ints, l, INT_MAX);
        break;
    case ARRAY_TUSHORT:
        raise_ushorts (A->values.ushorts, c, B->values.ushorts, l);
        break;
    case ARRAY_TNUSHORT:
        raise_nushorts (A->values.ushorts, c, B->values.ushorts, l, USHRT_MAX);
        break;
    case ARRAY_TSHORT:
        raise_shorts (A->values.shorts, c, B->values.shorts, l);
        break;
    case ARRAY_TNSHORT:
        raise_nshorts (A->values.shorts, c, B->values.shorts, l, SHRT_MAX);
        break;
    case ARRAY_TUCHAR:
        raise_uchars (A->values.uchars, c, B->values.uchars, l);
        break;
    case ARRAY_TNUCHAR:
        raise_nuchars (A->values.uchars, c, B->values.uchars, l, UCHAR_MAX);
        break;
    case ARRAY_TCHAR:
        raise_chars (A->values.chars, c, B->values.chars, l);
        break;
    case ARRAY_TNCHAR:
        raise_nchars (A->values.chars, c, B->values.chars, l, CHAR_MAX);
        break;
    }

    return 1;
}

#define RANGE(FUNC, TYPE)                                               \
    static void FUNC (TYPE *A, lua_Number *m, lua_Number *M, int n)     \
    {                                                                   \
        TYPE m_u, M_u;                                                  \
        int i;                                                          \
                                                                        \
        m_u = M_u = A[0];                                               \
                                                                        \
        for (i = 1 ; i < n ; i += 1) {                                  \
            if (A[i] < m_u) {                                           \
                m_u = A[i];                                             \
            }                                                           \
                                                                        \
            if (A[i] > M_u) {                                           \
                M_u = A[i];                                             \
            }                                                           \
        }                                                               \
                                                                        \
        *m = (lua_Number)m_u;                                           \
        *M = (lua_Number)M_u;                                           \
    }

#define RANGE_NORM(FUNC, TYPE)                                          \
    static void FUNC (TYPE *A, lua_Number *m, lua_Number *M, int n, const double c) \
    {                                                                   \
        TYPE m_u, M_u;                                                  \
        int i;                                                          \
                                                                        \
        m_u = M_u = A[0];                                               \
                                                                        \
        for (i = 1 ; i < n ; i += 1) {                                  \
            if (A[i] < m_u) {                                           \
                m_u = A[i];                                             \
            }                                                           \
                                                                        \
            if (A[i] > M_u) {                                           \
                M_u = A[i];                                             \
            }                                                           \
        }                                                               \
                                                                        \
        *m = (lua_Number)(m_u / c);                                     \
        *M = (lua_Number)(M_u / c);                                     \
    }

RANGE(range_doubles, double)
RANGE(range_floats, float)

RANGE(range_ulongs, unsigned long)
RANGE(range_longs, signed long)
RANGE(range_uints, unsigned int)
RANGE(range_ints, signed int)
RANGE(range_ushorts, unsigned short)
RANGE(range_shorts, signed short)
RANGE(range_uchars, unsigned char)
RANGE(range_chars, signed char)

RANGE_NORM(range_nulongs, unsigned long)
RANGE_NORM(range_nlongs, signed long)
RANGE_NORM(range_nuints, unsigned int)
RANGE_NORM(range_nints, signed int)
RANGE_NORM(range_nushorts, unsigned short)
RANGE_NORM(range_nshorts, signed short)
RANGE_NORM(range_nuchars, unsigned char)
RANGE_NORM(range_nchars, signed char)

int arraymath_range (lua_State *L)
{
    array_Array *A;
    lua_Number m = 0, M = 0;
    int j, l;

    A = lua_touserdata (L, -1);

    for (j = 0, l = 1; j < A->rank ; l *= A->size[j], j += 1);

    switch (A->type) {
    case ARRAY_TDOUBLE:
        range_doubles (A->values.doubles, &m, &M, l);
        break;
    case ARRAY_TFLOAT:
        range_floats (A->values.floats, &m, &M, l);
        break;
    case ARRAY_TULONG:
        range_ulongs (A->values.ulongs, &m, &M, l);
        break;
    case ARRAY_TNULONG:
        range_nulongs (A->values.ulongs, &m, &M, l, ULONG_MAX);
        break;
    case ARRAY_TLONG:
        range_longs (A->values.longs, &m, &M, l);
        break;
    case ARRAY_TNLONG:
        range_nlongs (A->values.longs, &m, &M, l, LONG_MAX);
        break;
    case ARRAY_TUINT:
        range_uints (A->values.uints, &m, &M, l);
        break;
    case ARRAY_TNUINT:
        range_nuints (A->values.uints, &m, &M, l, UINT_MAX);
        break;
    case ARRAY_TINT:
        range_ints (A->values.ints, &m, &M, l);
        break;
    case ARRAY_TNINT:
        range_nints (A->values.ints, &m, &M, l, INT_MAX);
        break;
    case ARRAY_TUSHORT:
        range_ushorts (A->values.ushorts, &m, &M, l);
        break;
    case ARRAY_TNUSHORT:
        range_nushorts (A->values.ushorts, &m, &M, l, USHRT_MAX);
        break;
    case ARRAY_TSHORT:
        range_shorts (A->values.shorts, &m, &M, l);
        break;
    case ARRAY_TNSHORT:
        range_nshorts (A->values.shorts, &m, &M, l, SHRT_MAX);
        break;
    case ARRAY_TUCHAR:
        range_uchars (A->values.uchars, &m, &M, l);
        break;
    case ARRAY_TNUCHAR:
        range_nuchars (A->values.uchars, &m, &M, l, UCHAR_MAX);
        break;
    case ARRAY_TCHAR:
        range_chars (A->values.chars, &m, &M, l);
        break;
    case ARRAY_TNCHAR:
        range_nchars (A->values.chars, &m, &M, l, CHAR_MAX);
        break;
    default:
        assert(0);
    }

    lua_pushnumber (L, m);
    lua_pushnumber (L, M);

    return 2;
}

#define SUM(FUNC, TYPE)                                                 \
    static void FUNC##_2 (TYPE *A, TYPE *S, int n)                      \
    {                                                                   \
        int i;                                                          \
                                                                        \
        *S = A[0];                                                      \
                                                                        \
        for (i = 1 ; i < n ; i += 1) {                                  \
            *S += A[i];                                                 \
        }                                                               \
    }                                                                   \
                                                                        \
    static void FUNC (TYPE *A, TYPE *S, lua_Number *s, int k, int n)    \
    {                                                                   \
        int i;                                                          \
                                                                        \
        if(S) {                                                         \
            for (i = 0 ; i < n / k ; i += 1) {                          \
                FUNC##_2(&A[i * k], &S[i], k);                          \
            }                                                           \
        } else {                                                        \
            TYPE a;                                                     \
                                                                        \
            FUNC##_2(A, &a, k);                                         \
            *s = (lua_Number)a;                                         \
        }                                                               \
    }

SUM(sum_doubles, double)
SUM(sum_floats, float)

SUM(sum_ulongs, unsigned long)
SUM(sum_longs, signed long)
SUM(sum_uints, unsigned int)
SUM(sum_ints, signed int)
SUM(sum_ushorts, unsigned short)
SUM(sum_shorts, signed short)
SUM(sum_uchars, unsigned char)
SUM(sum_chars, signed char)

int arraymath_sum (lua_State *L)
{
    array_Array *A, *S;
    lua_Number s = 0;
    void *p;
    int j, l, k;

    A = lua_touserdata (L, -1);

    for (j = 0, l = 1; j < A->rank ; l *= A->size[j], j += 1);
    k = A->size[A->rank - 1];

    if (A->rank > 1) {
        S = array_createarrayv (L, A->type, NULL, A->rank - 1, A->size);
        p = S->values.any;
    } else {
        p = NULL;
    }

    switch (A->type) {
    case ARRAY_TDOUBLE:
        sum_doubles (A->values.doubles, p, &s, k, l);
        break;
    case ARRAY_TFLOAT:
        sum_floats (A->values.floats, p, &s, k, l);
        break;
    case ARRAY_TULONG: case ARRAY_TNULONG:
        sum_ulongs (A->values.ulongs, p, &s, k, l);
        break;
    case ARRAY_TLONG: case ARRAY_TNLONG:
        sum_longs (A->values.longs, p, &s, k, l);
        break;
    case ARRAY_TUINT: case ARRAY_TNUINT:
        sum_uints (A->values.uints, p, &s, k, l);
        break;
    case ARRAY_TINT: case ARRAY_TNINT:
        sum_ints (A->values.ints, p, &s, k, l);
        break;
    case ARRAY_TUSHORT: case ARRAY_TNUSHORT:
        sum_ushorts (A->values.ushorts, p, &s, k, l);
        break;
    case ARRAY_TSHORT: case ARRAY_TNSHORT:
        sum_shorts (A->values.shorts, p, &s, k, l);
        break;
    case ARRAY_TUCHAR: case ARRAY_TNUCHAR:
        sum_uchars (A->values.uchars, p, &s, k, l);
        break;
    case ARRAY_TCHAR: case ARRAY_TNCHAR:
        sum_chars (A->values.chars, p, &s, k, l);
        break;
    default:
        assert(0);
    }

    if (A->rank == 1) {
        lua_pushnumber (L, s);
    }

    return 1;
}

#define COMBINE(FUNC, TYPE)						\
    static void FUNC (TYPE *A, TYPE *B, double c, double d,             \
                      TYPE *C, int n)                                   \
    {									\
        int i;								\
                                                                        \
        for (i = 0 ; i < n ; i += 1) {					\
            C[i] = c * A[i] + d * B[i];                                 \
        }								\
    }

COMBINE(combine_doubles, double)
COMBINE(combine_floats, float)

COMBINE(combine_ulongs, unsigned long)
COMBINE(combine_longs, signed long)
COMBINE(combine_uints, unsigned int)
COMBINE(combine_ints, signed int)
COMBINE(combine_ushorts, unsigned short)
COMBINE(combine_shorts, signed short)
COMBINE(combine_uchars, unsigned char)
COMBINE(combine_chars, signed char)

COMBINE(combine_nulongs, unsigned long)
COMBINE(combine_nlongs, signed long)
COMBINE(combine_nuints, unsigned int)
COMBINE(combine_nints, signed int)
COMBINE(combine_nushorts, unsigned short)
COMBINE(combine_nshorts, signed short)
COMBINE(combine_nuchars, unsigned char)
COMBINE(combine_nchars, signed char)

void arraymath_combine (lua_State *L)
{
    array_Array *A, *B, *C;
    double c, d;
    int j, l;

    A = lua_touserdata (L, -4);
    B = lua_touserdata (L, -3);
    c = lua_tonumber (L, -2);
    d = lua_tonumber (L, -1);

    for (j = 0, l = 1; j < A->rank ; l *= A->size[j], j += 1);

    C = array_createarrayv (L, A->type, NULL, A->rank, A->size);

    switch (A->type) {
    case ARRAY_TDOUBLE:
        combine_doubles (A->values.doubles, B->values.doubles, c, d,
                         C->values.doubles, l);
        break;
    case ARRAY_TFLOAT:
        combine_floats (A->values.floats, B->values.floats, c, d,
                        C->values.floats, l);
        break;
    case ARRAY_TULONG:
        combine_ulongs (A->values.ulongs, B->values.ulongs, c, d,
                        C->values.ulongs, l);
        break;
    case ARRAY_TNULONG:
        combine_nulongs (A->values.ulongs, B->values.ulongs, c, d,
                         C->values.ulongs, l);
        break;
    case ARRAY_TLONG:
        combine_longs (A->values.longs, B->values.longs, c, d,
                       C->values.longs, l);
        break;
    case ARRAY_TNLONG:
        combine_nlongs (A->values.longs, B->values.longs, c, d,
                        C->values.longs, l);
        break;
    case ARRAY_TUINT:
        combine_uints (A->values.uints, B->values.uints, c, d,
                       C->values.uints, l);
        break;
    case ARRAY_TNUINT:
        combine_nuints (A->values.uints, B->values.uints, c, d,
                        C->values.uints, l);
        break;
    case ARRAY_TINT:
        combine_ints (A->values.ints, B->values.ints, c, d,
                      C->values.ints, l);
        break;
    case ARRAY_TNINT:
        combine_nints (A->values.ints, B->values.ints, c, d,
                       C->values.ints, l);
        break;
    case ARRAY_TUSHORT:
        combine_ushorts (A->values.ushorts, B->values.ushorts, c, d,
                         C->values.ushorts, l);
        break;
    case ARRAY_TNUSHORT:
        combine_nushorts (A->values.ushorts, B->values.ushorts, c, d,
                          C->values.ushorts, l);
        break;
    case ARRAY_TSHORT:
        combine_shorts (A->values.shorts, B->values.shorts, c, d,
                        C->values.shorts, l);
        break;
    case ARRAY_TNSHORT:
        combine_nshorts (A->values.shorts, B->values.shorts, c, d,
                         C->values.shorts, l);
        break;
    case ARRAY_TUCHAR:
        combine_uchars (A->values.uchars, B->values.uchars, c, d,
                        C->values.uchars, l);
        break;
    case ARRAY_TNUCHAR:
        combine_nuchars (A->values.uchars, B->values.uchars, c, d,
                         C->values.uchars, l);
        break;
    case ARRAY_TCHAR:
        combine_chars (A->values.chars, B->values.chars, c, d,
                       C->values.chars, l);
        break;
    case ARRAY_TNCHAR:
        combine_nchars (A->values.chars, B->values.chars, c, d,
                        C->values.chars, l);
        break;
    }
}

#define DOT(FUNC, TYPE)						\
    static double FUNC (TYPE *A, TYPE *B, int n)		\
    {								\
        double d;						\
        int i;							\
                                                                \
        for (i = 0, d = 0 ; i < n ; d += A[i] * B[i], i += 1);	\
                                                                \
        return d;						\
    }

DOT(dot_doubles, double)
DOT(dot_floats, float)

#define DISTANCE(FUNC, TYPE)						\
    static double FUNC (TYPE *A, TYPE *B, int n)			\
    {									\
        double d, r;							\
        int i;								\
                                                                        \
        for (i = 0, d = 0;						\
             i < n;							\
             r = A[i] - B[i], d += r * r, i += 1);			\
                                                                        \
        return d;							\
    }

DISTANCE(distance_doubles, double)
DISTANCE(distance_floats, float)

double arraymath_dot (lua_State *L)
{
    array_Array *A, *B;
    double d;

    A = lua_touserdata(L, -2);
    B = lua_touserdata(L, -1);

    if (A->type == ARRAY_TDOUBLE) {
        d = dot_doubles (A->values.doubles, B->values.doubles, A->size[0]);
    } else {
        d = dot_floats (A->values.floats, B->values.floats, A->size[0]);
    }

    return d;
}

#define CROSS(FUNC, TYPE)						\
    static void FUNC (TYPE *A, TYPE *B, TYPE *C)			\
    {									\
        C[0] = A[1] * B[2] - A[2] * B[1];				\
        C[1] = A[2] * B[0] - A[0] * B[2];				\
        C[2] = A[0] * B[1] - A[1] * B[0];				\
    }

CROSS(cross_doubles, double)
CROSS(cross_floats, float)

void arraymath_cross (lua_State *L)
{
    array_Array *A, *B, *C;

    A = lua_touserdata (L, -2);
    B = lua_touserdata (L, -1);

    C = array_createarray (L, A->type, NULL, 1, 3);

    if (A->type == ARRAY_TDOUBLE) {
        cross_doubles (A->values.doubles,
                       B->values.doubles,
                       C->values.doubles);
    } else {
        cross_floats (A->values.floats,
                      B->values.floats,
                      C->values.floats);
    }
}

double arraymath_length (lua_State *L)
{
    array_Array *A;
    double d;

    A = lua_touserdata(L, -1);

    if (A->type == ARRAY_TDOUBLE) {
        d = dot_doubles (A->values.doubles, A->values.doubles, A->size[0]);
    } else {
        d = dot_floats (A->values.floats, A->values.floats, A->size[0]);
    }

    return sqrt(d);
}

double arraymath_distance (lua_State *L)
{
    array_Array *A, *B;
    double d;

    A = lua_touserdata (L, -2);
    B = lua_touserdata (L, -1);

    if (A->type == ARRAY_TDOUBLE) {
        d = distance_doubles (A->values.doubles, B->values.doubles, A->size[0]);
    } else {
        d = distance_floats (A->values.floats, B->values.floats, A->size[0]);
    }

    return sqrt(d);
}

array_Array *arraymath_normalize (lua_State *L)
{
    array_Array *A, *B;
    double d;
    int i;

    A = lua_touserdata(L, -1);
    B = array_createarray (L, A->type, NULL, 1, A->size[0]);

    if (A->type == ARRAY_TDOUBLE) {
        double *u, *v;

        d = sqrt(dot_doubles (A->values.doubles,
                              A->values.doubles,
                              A->size[0]));

        u = B->values.doubles;
        v = A->values.doubles;

        for (i = 0 ; i < A->size[0] ; i += 1) {
            u[i] = v[i] / d;
        }
    } else {
        float *u, *v;

        d = sqrt(dot_floats (A->values.floats,
                             A->values.floats,
                             A->size[0]));

        u = B->values.floats;
        v = A->values.floats;

        for (i = 0 ; i < A->size[0] ; i += 1) {
            u[i] = v[i] / d;
        }
    }

    lua_remove (L, -2);

    return B;
}

#ifdef ARRAYMATH_COLUMN_MAJOR
#define MATRIX_VECTOR(FUNC, TYPE)					\
    static void FUNC (TYPE *A, TYPE *B, TYPE *C, int n, int m)		\
    {									\
        int i, j;							\
                                                                        \
        for (i = 0 ; i < m ; i += 1) {					\
            C[i] = 0;							\
                                                                        \
            for (j = 0 ; j < n ; j += 1) {				\
                C[i] += A[j * m + i] * B[j];				\
            }								\
        }								\
    }
#else
#define MATRIX_VECTOR(FUNC, TYPE)					\
    static void FUNC (TYPE *A, TYPE *B, TYPE *C, int n, int m)		\
    {									\
        int i, j;							\
                                                                        \
        for (i = 0 ; i < n ; i += 1) {					\
            C[i] = 0;							\
                                                                        \
            for (j = 0 ; j < m ; j += 1) {				\
                C[i] += A[i * m + j] * B[j];				\
            }								\
        }								\
    }
#endif

MATRIX_VECTOR(matrix_vector_doubles, double)
MATRIX_VECTOR(matrix_vector_floats, float)

#ifdef ARRAYMATH_COLUMN_MAJOR
#define MATRIX_MATRIX(FUNC, TYPE)					\
    static void FUNC (TYPE *A, TYPE *B, TYPE *C,			\
                      int n, int m, int s, int t)			\
    {									\
        int i, j, k;							\
                                                                        \
        for (k = 0 ; k < s ; k += 1) {					\
            for (i = 0 ; i < m ; i += 1) {				\
                C[k * m + i] = 0;					\
                                                                        \
                for (j = 0 ; j < n ; j += 1) {				\
                    C[k * m + i] += A[j * m + i] * B[k * t + j];	\
                }							\
            }								\
        }								\
    }
#else
#define MATRIX_MATRIX(FUNC, TYPE)					\
    static void FUNC (TYPE *A, TYPE *B, TYPE *C,			\
                      int n, int m, int s, int t)			\
    {									\
        int i, j, k;							\
                                                                        \
        for (i = 0 ; i < n ; i += 1) {					\
            for (k = 0 ; k < t ; k += 1) {				\
                C[i * t + k] = 0;					\
                                                                        \
                for (j = 0 ; j < m ; j += 1) {				\
                    C[i * t + k] += A[i * m + j] * B[j * t + k];	\
                }							\
            }								\
        }								\
    }
#endif

MATRIX_MATRIX(matrix_matrix_doubles, double)
MATRIX_MATRIX(matrix_matrix_floats, float)

array_Array *arraymath_matrix_multiply (lua_State *L)
{
    array_Array *A, *B, *C;

    A = lua_touserdata (L, -2);
    B = lua_touserdata (L, -1);

    if (B->rank == 2) {
#ifdef ARRAYMATH_COLUMN_MAJOR
        C = array_createarray (L, A->type, NULL, 2, B->size[0], A->size[1]);
#else
        C = array_createarray (L, A->type, NULL, 2, A->size[0], B->size[1]);
#endif

        if (A->type == ARRAY_TDOUBLE) {
            matrix_matrix_doubles (A->values.doubles,
                                   B->values.doubles,
                                   C->values.doubles,
                                   A->size[0], A->size[1],
                                   B->size[0], B->size[1]);
        } else {
            matrix_matrix_floats (A->values.floats,
                                  B->values.floats,
                                  C->values.floats,
                                  A->size[0], A->size[1],
                                  B->size[0], B->size[1]);
        }
    } else {
#ifdef ARRAYMATH_COLUMN_MAJOR
        C = array_createarray (L, A->type, NULL, 1, A->size[1]);
#else
        C = array_createarray (L, A->type, NULL, 1, A->size[0]);
#endif

        if (A->type == ARRAY_TDOUBLE) {
            matrix_vector_doubles (A->values.doubles,
                                   B->values.doubles,
                                   C->values.doubles,
                                   A->size[0], A->size[1]);
        } else {
            matrix_vector_floats (A->values.floats,
                                  B->values.floats,
                                  C->values.floats,
                                  A->size[0], A->size[1]);
        }
    }

    return C;
}

array_Array *arraymath_matrix_multiplyadd (lua_State *L)
{
    array_Array *A, *B;
    int j, l;

    B = lua_touserdata (L, -1);
    lua_insert(L, -3);
    A = arraymath_matrix_multiply (L);
    lua_pushvalue(L, -4);
    lua_remove(L, -5);
    lua_insert(L, -2);

    for (j = 0, l = 1; j < B->rank ; l *= B->size[j], j += 1);

    if (A->type == ARRAY_TDOUBLE) {
        add_doubles(A->values.doubles, B->values.doubles,
                    A->values.doubles, l);
    } else {
        add_floats(A->values.floats, B->values.floats,
                   A->values.floats, l);
    }

    return A;
}

#ifdef ARRAYMATH_COLUMN_MAJOR
#define APPLY_REAL(FUNC, TYPE)						\
    static void FUNC##_real (double *T, double *f, TYPE *v, TYPE *r)    \
    {                                                                   \
        r[0] = T[0] * v[0] + T[3] * v[1] + T[6] * v[2];                 \
        r[1] = T[1] * v[0] + T[4] * v[1] + T[7] * v[2];                 \
        r[2] = T[2] * v[0] + T[5] * v[1] + T[8] * v[2];                 \
                                                                        \
        if (f) {							\
            r[0] += f[0] - (T[0] * f[0] + T[3] * f[1] + T[6] * f[2]);   \
            r[1] += f[1] - (T[1] * f[0] + T[4] * f[1] + T[7] * f[2]);   \
            r[2] += f[2] - (T[2] * f[0] + T[5] * f[1] + T[8] * f[2]);   \
        }								\
    }
#else
#define APPLY_REAL(FUNC, TYPE)						\
    static void FUNC##_real (double *T, double *f, TYPE *v, TYPE *r)    \
    {                                                                   \
        r[0] = T[0] * v[0] + T[1] * v[1] + T[2] * v[2];                 \
        r[1] = T[3] * v[0] + T[4] * v[1] + T[5] * v[2];                 \
        r[2] = T[6] * v[0] + T[7] * v[1] + T[8] * v[2];                 \
                                                                        \
        if (f) {							\
            r[0] += f[0] - (T[0] * f[0] + T[1] * f[1] + T[2] * f[2]);   \
            r[1] += f[1] - (T[3] * f[0] + T[4] * f[1] + T[5] * f[2]);   \
            r[2] += f[2] - (T[6] * f[0] + T[7] * f[1] + T[8] * f[2]);   \
        }								\
    }
#endif

#define APPLY(FUNC, TYPE)						\
    static void FUNC (double *T, double *f, TYPE *v, TYPE *r,		\
                      int rank, int *size)				\
    {									\
        if (rank == 1) {						\
            FUNC##_real(T, f, v, r);                                    \
        } else {							\
            int j, d;							\
                                                                        \
            for (j = 1, d = 1 ; j < rank ; d *= size[j], j += 1);	\
                                                                        \
            for (j = 0 ; j < size[0] ; j += 1) {			\
                FUNC (T, f, &v[j * d], &r[j * d], rank - 1, &size[1]);	\
            }								\
        }								\
    }									\

APPLY_REAL(apply_doubles, double)
APPLY_REAL(apply_floats, float)

APPLY(apply_doubles, double)
APPLY(apply_floats, float)

array_Array *arraymath_apply (lua_State *L, int i)
{
    array_Array *transform, *data, *result, *fixed;

    transform = lua_touserdata (L, -2);
    data = lua_touserdata (L, -1);

    if (i > 0) {
        fixed = lua_touserdata (L, i);
    } else {
        fixed = NULL;
    }

    result = array_createarrayv (L, data->type, NULL, data->rank, data->size);

    if (data->type == ARRAY_TDOUBLE) {
        apply_doubles (transform->values.doubles,
                       fixed ? fixed->values.doubles : NULL,
                       data->values.doubles,
                       result->values.doubles, result->rank, result->size);
    } else {
        apply_floats (transform->values.doubles,
                      fixed ? fixed->values.doubles : 0,
                      data->values.floats,
                      result->values.floats, result->rank, result->size);
    }

    return result;
}
