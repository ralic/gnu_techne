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

#define bits(SIGNEDNESS, TYPE)                          \
    (sizeof(TYPE) * 8 - (((SIGNEDNESS int) - 1) < 0))

#define OP(FUNC, OPERATOR, TYPE)                                        \
    static void FUNC (TYPE *A, TYPE *B, TYPE *C, int n)                 \
    {                                                                   \
        int i;                                                          \
                                                                        \
        for (i = 0 ; i < n ; i += 1) {                                  \
            C[i] = A[i] OPERATOR B[i];                                  \
        }                                                               \
    }

#define OP_NORM_ADDSUB(FUNC, OPERATOR, SIGNEDNESS, TYPE)                \
    static void FUNC (SIGNEDNESS TYPE *A, SIGNEDNESS TYPE *B,           \
                      SIGNEDNESS TYPE *C, int n)                        \
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
#define OP_ADD_NORM(FUNC, SIGNEDNESS, TYPE)     \
    OP_NORM_ADDSUB(FUNC, +, SIGNEDNESS, TYPE)
#define OP_SUB_NORM(FUNC, SIGNEDNESS, TYPE)     \
    OP_NORM_ADDSUB(FUNC, -, SIGNEDNESS, TYPE)

#define OP_MUL_NORM(FUNC, SIGNEDNESS, TYPE)                             \
    static void FUNC (SIGNEDNESS TYPE *A, SIGNEDNESS TYPE *B,           \
                      SIGNEDNESS TYPE *C, int n)                        \
    {                                                                   \
        int i;                                                          \
                                                                        \
        for (i = 0 ; i < n ; i += 1) {                                  \
            C[i] = ((SIGNEDNESS long)A[i] * B[i]) >> bits(SIGNEDNESS, TYPE); \
        }                                                               \
    }

#define OP_DIV_NORM(FUNC, SIGNEDNESS, TYPE)                             \
    static void FUNC (SIGNEDNESS TYPE *A, SIGNEDNESS TYPE *B,           \
                      SIGNEDNESS TYPE *C, int n)                        \
    {                                                                   \
        int i;                                                          \
                                                                        \
        for (i = 0 ; i < n ; i += 1) {                                  \
            SIGNEDNESS long r = ((SIGNEDNESS long)A[i] << 32) / B[i];   \
            C[i] = (r >> (32 - bits(SIGNEDNESS, TYPE))) - (r << 32);    \
        }                                                               \
    }

#define DEFINE_OPERATION(OPERATION, OPERATOR)                           \
    OP_##OPERATOR(OPERATION##_doubles, double)                          \
    OP_##OPERATOR(OPERATION##_floats, float)                            \
                                                                        \
    OP_##OPERATOR(OPERATION##_uints, unsigned int)                      \
    OP_##OPERATOR(OPERATION##_ints, signed int)                         \
    OP_##OPERATOR(OPERATION##_ushorts, unsigned short)                  \
    OP_##OPERATOR(OPERATION##_shorts, signed short)                     \
    OP_##OPERATOR(OPERATION##_uchars, unsigned char)                    \
    OP_##OPERATOR(OPERATION##_chars, signed char)                       \
                                                                        \
    OP_##OPERATOR##_NORM(OPERATION##_nuints, unsigned, int)             \
    OP_##OPERATOR##_NORM(OPERATION##_nints, signed, int)                \
    OP_##OPERATOR##_NORM(OPERATION##_nushorts, unsigned, short)         \
    OP_##OPERATOR##_NORM(OPERATION##_nshorts, signed, short)            \
    OP_##OPERATOR##_NORM(OPERATION##_nuchars, unsigned, char)           \
    OP_##OPERATOR##_NORM(OPERATION##_nchars, signed, char)              \
                                                                        \
    static int pointwise_##OPERATION (lua_State *L)                     \
    {                                                                   \
        array_Array *A, *B, *C;                                         \
        int j, l;                                                       \
                                                                        \
        A = lua_touserdata (L, -2);                                     \
        B = lua_touserdata (L, -1);                                     \
                                                                        \
        C = array_createarrayv (L, A->type, NULL, A->rank, A->size);    \
                                                                        \
        for (j = 0, l = 1; j < A->rank ; l *= A->size[j], j += 1);      \
                                                                        \
        switch (A->type) {                                              \
        case ARRAY_TDOUBLE:                                             \
            OPERATION##_doubles (A->values.doubles, B->values.doubles,  \
                                 C->values.doubles, l);                 \
            break;                                                      \
        case ARRAY_TFLOAT:                                              \
            OPERATION##_floats (A->values.floats, B->values.floats,     \
                                C->values.floats, l);                   \
            break;                                                      \
        case ARRAY_TUINT:                                               \
            OPERATION##_uints (A->values.uints, B->values.uints,        \
                               C->values.uints, l);                     \
            break;                                                      \
        case ARRAY_TNUINT:                                              \
            OPERATION##_nuints (A->values.uints, B->values.uints,       \
                                C->values.uints, l);                    \
            break;                                                      \
        case ARRAY_TINT:                                                \
            OPERATION##_ints (A->values.ints, B->values.ints,           \
                              C->values.ints, l);                       \
            break;                                                      \
        case ARRAY_TNINT:                                               \
            OPERATION##_nints (A->values.ints, B->values.ints,          \
                               C->values.ints, l);                      \
            break;                                                      \
        case ARRAY_TUSHORT:                                             \
            OPERATION##_ushorts (A->values.ushorts, B->values.ushorts,  \
                                 C->values.ushorts, l);                 \
            break;                                                      \
        case ARRAY_TNUSHORT:                                            \
            OPERATION##_nushorts (A->values.ushorts, B->values.ushorts, \
                                  C->values.ushorts, l);                \
            break;                                                      \
        case ARRAY_TSHORT:                                              \
            OPERATION##_shorts (A->values.shorts, B->values.shorts,     \
                                C->values.shorts, l);                   \
            break;                                                      \
        case ARRAY_TNSHORT:                                             \
            OPERATION##_nshorts (A->values.shorts, B->values.shorts,    \
                                 C->values.shorts, l);                  \
            break;                                                      \
        case ARRAY_TUCHAR:                                              \
            OPERATION##_uchars (A->values.uchars, B->values.uchars,     \
                                C->values.uchars, l);                   \
            break;                                                      \
        case ARRAY_TNUCHAR:                                             \
            OPERATION##_nuchars (A->values.uchars, B->values.uchars,    \
                                 C->values.uchars, l);                  \
            break;                                                      \
        case ARRAY_TCHAR:                                               \
            OPERATION##_chars (A->values.chars, B->values.chars,        \
                               C->values.chars, l);                     \
            break;                                                      \
        case ARRAY_TNCHAR:                                              \
            OPERATION##_nchars (A->values.chars, B->values.chars,       \
                                C->values.chars, l);                    \
            break;                                                      \
        }                                                               \
                                                                        \
        return 1;                                                       \
    }

DEFINE_OPERATION(add, ADD)
DEFINE_OPERATION(multiply, MUL)
DEFINE_OPERATION(subtract, SUB)
DEFINE_OPERATION(divide, DIV)

#define OFFSET(FUNC, TYPE)                                      \
    static void FUNC (TYPE *A, TYPE d, TYPE *B, int n)          \
    {                                                           \
        int i;                                                  \
                                                                \
        for (i = 0 ; i < n ; i += 1) {                          \
            B[i] = d + A[i];                                    \
        }                                                       \
    }

#define OFFSETINVERSE(FUNC, TYPE)                               \
    static void FUNC (TYPE *A, TYPE d, TYPE *B, int n)          \
    {                                                           \
        int i;                                                  \
                                                                \
        for (i = 0 ; i < n ; i += 1) {                          \
            B[i] = d - A[i];                                    \
        }                                                       \
    }

#define INVERSEOFFSET(FUNC, TYPE)                               \
    static void FUNC (TYPE *A, TYPE d, TYPE *B, int n)          \
    {                                                           \
        int i;                                                  \
                                                                \
        for (i = 0 ; i < n ; i += 1) {                          \
            B[i] = A[i] - d;                                    \
        }                                                       \
    }

#define DEFINE_OFFSET_OPERATION(OPERATION, MACRO)                       \
    MACRO(OPERATION##_doubles, double)                                  \
    MACRO(OPERATION##_floats, float)                                    \
                                                                        \
    MACRO(OPERATION##_uints, unsigned int)                              \
    MACRO(OPERATION##_ints, signed int)                                 \
    MACRO(OPERATION##_ushorts, unsigned short)                          \
    MACRO(OPERATION##_shorts, signed short)                             \
    MACRO(OPERATION##_uchars, unsigned char)                            \
    MACRO(OPERATION##_chars, signed char)                               \
                                                                        \
    static int scalar_##OPERATION (lua_State *L)                        \
    {                                                                   \
        array_Array *A, *B;                                             \
        lua_Number d;                                                   \
        int j, l;                                                       \
                                                                        \
        A = lua_touserdata (L, -2);                                     \
        d = lua_tonumber (L, -1);                                       \
                                                                        \
        for (j = 0, l = 1; j < A->rank ; l *= A->size[j], j += 1);      \
                                                                        \
        B = array_createarrayv (L, A->type, NULL, A->rank, A->size);    \
                                                                        \
        switch (A->type) {                                              \
        case ARRAY_TDOUBLE:                                             \
            OPERATION##_doubles (A->values.doubles, d,                  \
                                 B->values.doubles, l);                 \
                break;                                                  \
        case ARRAY_TFLOAT:                                              \
            OPERATION##_floats (A->values.floats, d,                    \
                                B->values.floats, l);                   \
                break;                                                  \
        case ARRAY_TUINT:                                               \
            OPERATION##_uints (A->values.uints, d, B->values.uints, l); \
                break;                                                  \
        case ARRAY_TINT:                                                \
            OPERATION##_ints (A->values.ints, d, B->values.ints, l);    \
                break;                                                  \
        case ARRAY_TUSHORT:                                             \
            OPERATION##_ushorts (A->values.ushorts, d,                  \
                                 B->values.ushorts, l);                 \
                break;                                                  \
        case ARRAY_TSHORT:                                              \
            OPERATION##_shorts (A->values.shorts, d,                    \
                                B->values.shorts, l);                   \
                break;                                                  \
        case ARRAY_TUCHAR:                                              \
            OPERATION##_uchars (A->values.uchars, d,                    \
                                B->values.uchars, l);                   \
                break;                                                  \
        case ARRAY_TCHAR:                                               \
            OPERATION##_chars (A->values.chars, d,                      \
                               B->values.chars, l);                     \
                break;                                                  \
        case ARRAY_TNUINT:                                              \
            OPERATION##_uints (A->values.uints, d * UINT_MAX,           \
                               B->values.uints, l);                     \
                break;                                                  \
        case ARRAY_TNINT:                                               \
            OPERATION##_ints (A->values.ints, d * INT_MAX,              \
                              B->values.ints, l);                       \
                break;                                                  \
        case ARRAY_TNUSHORT:                                            \
            OPERATION##_ushorts (A->values.ushorts, d * USHRT_MAX,      \
                                 B->values.ushorts, l);                 \
                break;                                                  \
        case ARRAY_TNSHORT:                                             \
            OPERATION##_shorts (A->values.shorts, d * SHRT_MAX,         \
                                B->values.shorts, l);                   \
                break;                                                  \
        case ARRAY_TNUCHAR:                                             \
            OPERATION##_uchars (A->values.uchars, d * UCHAR_MAX,        \
                                B->values.uchars, l);                   \
                break;                                                  \
        case ARRAY_TNCHAR:                                              \
            OPERATION##_chars (A->values.chars, d * CHAR_MAX,           \
                               B->values.chars, l);                     \
                break;                                                  \
        }                                                               \
                                                                        \
        return 1;                                                       \
    }

DEFINE_OFFSET_OPERATION(offset, OFFSET)
DEFINE_OFFSET_OPERATION(offsetinverse, OFFSETINVERSE)
DEFINE_OFFSET_OPERATION(inverseoffset, INVERSEOFFSET)

#define SCALE(FUNC, TYPE)                                       \
    static void FUNC (TYPE *A, TYPE c, TYPE *B, int n)          \
    {                                                           \
        int i;                                                  \
                                                                \
        for (i = 0 ; i < n ; i += 1) {                          \
            B[i] = c * A[i];                                    \
        }                                                       \
    }

#define SCALE_NORM(FUNC, SIGNEDNESS, TYPE)                              \
    static void FUNC (SIGNEDNESS TYPE *A, SIGNEDNESS TYPE d,            \
                      SIGNEDNESS TYPE *B, int n)                        \
    {                                                                   \
        int i;                                                          \
                                                                        \
        for (i = 0 ; i < n ; i += 1) {                                  \
            B[i] = ((SIGNEDNESS long)d * A[i]) >> bits(SIGNEDNESS, TYPE); \
        }                                                               \
    }

#define SCALEINVERSE(FUNC, TYPE)                                \
    static void FUNC (TYPE *A, lua_Number d, TYPE *B, int n)    \
    {                                                           \
        int i;                                                  \
                                                                \
        for (i = 0 ; i < n ; i += 1) {                          \
            B[i] = d / A[i];                                    \
        }                                                       \
    }

#define SCALEINVERSE_NORM(FUNC, SIGNEDNESS, TYPE)                       \
    static void FUNC (SIGNEDNESS TYPE *A, SIGNEDNESS TYPE d,            \
                          SIGNEDNESS TYPE *B, int n)                    \
    {                                                                   \
        int i;                                                          \
                                                                        \
        for (i = 0 ; i < n ; i += 1) {                                  \
            SIGNEDNESS long r = ((SIGNEDNESS long)d << 32) / A[i];      \
            B[i] = (r >> (32 - bits(SIGNEDNESS, TYPE))) - (r >> 32);    \
        }                                                               \
    }

#define INVERSESCALE(FUNC, TYPE)                                \
    static void FUNC (TYPE *A, lua_Number d, TYPE *B, int n)    \
    {                                                           \
        int i;                                                  \
                                                                \
        for (i = 0 ; i < n ; i += 1) {                          \
            B[i] = A[i] / d;                                    \
        }                                                       \
    }

#define INVERSESCALE_NORM(FUNC, SIGNEDNESS, TYPE)                       \
    static void FUNC (SIGNEDNESS TYPE *A, SIGNEDNESS TYPE d,            \
                      SIGNEDNESS TYPE *B, int n)                        \
    {                                                                   \
        int i;                                                          \
                                                                        \
        for (i = 0 ; i < n ; i += 1) {                                  \
            SIGNEDNESS long r = ((SIGNEDNESS long)A[i] << 32) / d;      \
            B[i] = (r >> (32 - bits(SIGNEDNESS, TYPE))) - (r >> 32);    \
        }                                                               \
    }

#define DEFINE_SCALE_OPERATION(OPERATION, MACRO)                        \
    MACRO(OPERATION##_doubles, double)                                  \
    MACRO(OPERATION##_floats, float)                                    \
                                                                        \
    MACRO(OPERATION##_uints, unsigned int)                              \
    MACRO(OPERATION##_ints, signed int)                                 \
    MACRO(OPERATION##_ushorts, unsigned short)                          \
    MACRO(OPERATION##_shorts, signed short)                             \
    MACRO(OPERATION##_uchars, unsigned char)                            \
    MACRO(OPERATION##_chars, signed char)                               \
                                                                        \
    MACRO##_NORM(OPERATION##_norm_uints, unsigned, int)                 \
    MACRO##_NORM(OPERATION##_norm_ints, signed, int)                    \
    MACRO##_NORM(OPERATION##_norm_ushorts, unsigned, short)             \
    MACRO##_NORM(OPERATION##_norm_shorts, signed, short)                \
    MACRO##_NORM(OPERATION##_norm_uchars, unsigned, char)               \
    MACRO##_NORM(OPERATION##_norm_chars, signed, char)                  \
                                                                        \
    static int scalar_##OPERATION (lua_State *L)                        \
    {                                                                   \
        array_Array *A, *B;                                             \
        lua_Number d;                                                   \
        int j, l;                                                       \
                                                                        \
        A = lua_touserdata (L, -2);                                     \
        d = lua_tonumber (L, -1);                                       \
                                                                        \
        for (j = 0, l = 1; j < A->rank ; l *= A->size[j], j += 1);      \
                                                                        \
        B = array_createarrayv (L, A->type, NULL, A->rank, A->size);    \
                                                                        \
        switch (A->type) {                                              \
        case ARRAY_TDOUBLE:                                             \
            OPERATION##_doubles (A->values.doubles, d,                  \
                                 B->values.doubles, l);                 \
                break;                                                  \
        case ARRAY_TFLOAT:                                              \
            OPERATION##_floats (A->values.floats, d,                    \
                                B->values.floats, l);                   \
                break;                                                  \
        case ARRAY_TUINT:                                               \
            OPERATION##_uints (A->values.uints, d, B->values.uints, l); \
                break;                                                  \
        case ARRAY_TINT:                                                \
            OPERATION##_ints (A->values.ints, d, B->values.ints, l);    \
                break;                                                  \
        case ARRAY_TUSHORT:                                             \
            OPERATION##_ushorts (A->values.ushorts, d,                  \
                                 B->values.ushorts, l);                 \
                break;                                                  \
        case ARRAY_TSHORT:                                              \
            OPERATION##_shorts (A->values.shorts, d,                    \
                                B->values.shorts, l);                   \
                break;                                                  \
        case ARRAY_TUCHAR:                                              \
            OPERATION##_uchars (A->values.uchars, d,                    \
                                B->values.uchars, l);                   \
                break;                                                  \
        case ARRAY_TCHAR:                                               \
            OPERATION##_chars (A->values.chars, d,                      \
                               B->values.chars, l);                     \
                break;                                                  \
        case ARRAY_TNUINT:                                              \
            OPERATION##_norm_uints (A->values.uints, d * UINT_MAX,      \
                               B->values.uints, l);                     \
                break;                                                  \
        case ARRAY_TNINT:                                               \
            OPERATION##_norm_ints (A->values.ints, d * INT_MAX,         \
                              B->values.ints, l);                       \
                break;                                                  \
        case ARRAY_TNUSHORT:                                            \
            OPERATION##_norm_ushorts (A->values.ushorts, d * USHRT_MAX, \
                                 B->values.ushorts, l);                 \
                break;                                                  \
        case ARRAY_TNSHORT:                                             \
            OPERATION##_norm_shorts (A->values.shorts, d * SHRT_MAX,    \
                                B->values.shorts, l);                   \
                break;                                                  \
        case ARRAY_TNUCHAR:                                             \
            OPERATION##_norm_uchars (A->values.uchars, d * UCHAR_MAX,   \
                                B->values.uchars, l);                   \
                break;                                                  \
        case ARRAY_TNCHAR:                                              \
            OPERATION##_norm_chars (A->values.chars, d * CHAR_MAX,      \
                               B->values.chars, l);                     \
                break;                                                  \
        }                                                               \
                                                                        \
        return 1;                                                       \
    }

DEFINE_SCALE_OPERATION(scale, SCALE)
DEFINE_SCALE_OPERATION(scaleinverse, SCALEINVERSE)
DEFINE_SCALE_OPERATION(inversescale, INVERSESCALE)

int arraymath_add(lua_State *L)
{
    if (lua_type(L, 1) == LUA_TNUMBER) {
        lua_insert(L, 1);
        return scalar_offset(L);
    } else if (lua_type(L, 2) == LUA_TNUMBER) {
        return scalar_offset(L);
    } else {
        return pointwise_add(L);
    }
}

int arraymath_subtract(lua_State *L)
{
    if (lua_type(L, 1) == LUA_TNUMBER) {
        lua_insert(L, 1);

        return scalar_offsetinverse(L);
    } else if (lua_type(L, 2) == LUA_TNUMBER) {
        return scalar_inverseoffset(L);
    } else {
        return pointwise_subtract(L);
    }
}

int arraymath_multiply(lua_State *L)
{
    if (lua_type(L, 1) == LUA_TNUMBER) {
        lua_insert(L, 1);
        return scalar_scale(L);
    } else if (lua_type(L, 2) == LUA_TNUMBER) {
        return scalar_scale(L);
    } else {
        return pointwise_multiply(L);
    }
}

int arraymath_divide(lua_State *L)
{
    if (lua_type(L, 1) == LUA_TNUMBER) {
        lua_insert(L, 1);

        return scalar_scaleinverse(L);
    } else if (lua_type(L, 2) == LUA_TNUMBER) {
        return scalar_inversescale(L);
    } else {
        return pointwise_divide(L);
    }
}

#define BINARY_LOGICAL_OPERATOR(FUNC, OPERATOR, TYPE)                   \
    static void FUNC (TYPE *A, TYPE *B, int *C, int n)                  \
    {                                                                   \
        int i;                                                          \
                                                                        \
        for (i = 0 ; i < n ; i += 1) {                                  \
            C[i] = A[i] OPERATOR B[i];                                  \
        }                                                               \
    }                                                                   \
                                                                        \
    static void left_scalar_##FUNC (TYPE a, TYPE *B, int *C,            \
                                    int n)                              \
    {                                                                   \
        int i;                                                          \
                                                                        \
        for (i = 0 ; i < n ; i += 1) {                                  \
            C[i] = a OPERATOR B[i];                                     \
        }                                                               \
    }                                                                   \
                                                                        \
    static void right_scalar_##FUNC (TYPE *A, TYPE b, int *C,           \
                                     int n)                             \
    {                                                                   \
        int i;                                                          \
                                                                        \
        for (i = 0 ; i < n ; i += 1) {                                  \
            C[i] = A[i] OPERATOR b;                                     \
        }                                                               \
    }                                                                   \

#define DEFINE_BINARY_LOGICAL_OPERATOR(OPERATION, OPERATOR)             \
    BINARY_LOGICAL_OPERATOR(OPERATION##_doubles, OPERATOR, double)      \
    BINARY_LOGICAL_OPERATOR(OPERATION##_floats, OPERATOR, float)        \
                                                                        \
    BINARY_LOGICAL_OPERATOR(OPERATION##_uints, OPERATOR, unsigned int)  \
    BINARY_LOGICAL_OPERATOR(OPERATION##_ints, OPERATOR, signed int)     \
    BINARY_LOGICAL_OPERATOR(OPERATION##_ushorts, OPERATOR, unsigned short) \
    BINARY_LOGICAL_OPERATOR(OPERATION##_shorts, OPERATOR, signed short) \
    BINARY_LOGICAL_OPERATOR(OPERATION##_uchars, OPERATOR, unsigned char) \
    BINARY_LOGICAL_OPERATOR(OPERATION##_chars, OPERATOR, signed char)   \
                                                                        \
    static int pointwise_##OPERATION (lua_State *L)                     \
    {                                                                   \
        array_Array *A, *B, *C;                                         \
        int j, l;                                                       \
                                                                        \
        A = lua_touserdata (L, -2);                                     \
        B = lua_touserdata (L, -1);                                     \
                                                                        \
        C = array_createarrayv (L, ARRAY_TINT, NULL, A->rank, A->size); \
                                                                        \
        for (j = 0, l = 1; j < A->rank ; l *= A->size[j], j += 1);      \
                                                                        \
        switch (abs(A->type)) {                                         \
        case ARRAY_TDOUBLE:                                             \
            OPERATION##_doubles (A->values.doubles, B->values.doubles,  \
                                 C->values.ints, l);                    \
                break;                                                  \
        case ARRAY_TFLOAT:                                              \
            OPERATION##_floats (A->values.floats, B->values.floats,     \
                                C->values.ints, l);                     \
                break;                                                  \
        case ARRAY_TUINT:                                               \
            OPERATION##_uints (A->values.uints, B->values.uints,        \
                               C->values.ints, l);                      \
                break;                                                  \
        case ARRAY_TINT:                                                \
            OPERATION##_ints (A->values.ints, B->values.ints,           \
                              C->values.ints, l);                       \
                break;                                                  \
        case ARRAY_TUSHORT:                                             \
            OPERATION##_ushorts (A->values.ushorts, B->values.ushorts,  \
                                 C->values.ints, l);                    \
                break;                                                  \
        case ARRAY_TSHORT:                                              \
            OPERATION##_shorts (A->values.shorts, B->values.shorts,     \
                                C->values.ints, l);                     \
                break;                                                  \
        case ARRAY_TUCHAR:                                              \
            OPERATION##_uchars (A->values.uchars, B->values.uchars,     \
                                C->values.ints, l);                     \
                break;                                                  \
        case ARRAY_TCHAR:                                               \
            OPERATION##_chars (A->values.chars, B->values.chars,        \
                               C->values.ints, l);                      \
                break;                                                  \
        }                                                               \
                                                                        \
        return 1;                                                       \
    }                                                                   \
                                                                        \
    static int left_scalar_##OPERATION (lua_State *L)                   \
    {                                                                   \
        array_Array *B, *C;                                             \
        lua_Number a;                                                   \
        int j, l;                                                       \
                                                                        \
        a = lua_tonumber (L, -2);                                       \
        B = lua_touserdata (L, -1);                                     \
                                                                        \
        C = array_createarrayv (L, ARRAY_TINT, NULL, B->rank, B->size); \
                                                                        \
        for (j = 0, l = 1; j < B->rank ; l *= B->size[j], j += 1);      \
                                                                        \
        switch (abs(B->type)) {                                         \
        case ARRAY_TDOUBLE:                                             \
            left_scalar_##OPERATION##_doubles (a, B->values.doubles,    \
                                               C->values.ints, l);      \
                break;                                                  \
        case ARRAY_TFLOAT:                                              \
            left_scalar_##OPERATION##_floats (a, B->values.floats,      \
                                              C->values.ints, l);       \
                break;                                                  \
        case ARRAY_TUINT:                                               \
            left_scalar_##OPERATION##_uints (B->type < 0 ? a * UINT_MAX : a, \
                                             B->values.uints,           \
                                             C->values.ints, l);        \
                break;                                                  \
        case ARRAY_TINT:                                                \
            left_scalar_##OPERATION##_ints (B->type < 0 ? a * INT_MAX : a, \
                                            B->values.ints,             \
                                            C->values.ints, l);         \
                break;                                                  \
        case ARRAY_TUSHORT:                                             \
            left_scalar_##OPERATION##_ushorts (                         \
                B->type < 0 ? a * USHRT_MAX : a,                        \
                B->values.ushorts,                                      \
                C->values.ints, l);                                     \
                break;                                                  \
        case ARRAY_TSHORT:                                              \
            left_scalar_##OPERATION##_shorts (B->type < 0 ? a * SHRT_MAX : a, \
                                              B->values.shorts,         \
                                              C->values.ints, l);       \
                break;                                                  \
        case ARRAY_TUCHAR:                                              \
            left_scalar_##OPERATION##_uchars (B->type < 0 ? a * UCHAR_MAX : a, \
                                              B->values.uchars,         \
                                              C->values.ints, l);       \
                break;                                                  \
        case ARRAY_TCHAR:                                               \
            left_scalar_##OPERATION##_chars (B->type < 0 ? a * CHAR_MAX : a, \
                                             B->values.chars,           \
                                             C->values.ints, l);        \
                break;                                                  \
        }                                                               \
                                                                        \
        return 1;                                                       \
    }                                                                   \
                                                                        \
    static int right_scalar_##OPERATION (lua_State *L)                  \
    {                                                                   \
        array_Array *A, *C;                                             \
        lua_Number b;                                                   \
        int j, l;                                                       \
                                                                        \
        A = lua_touserdata (L, -2);                                     \
        b = lua_tonumber (L, -1);                                       \
                                                                        \
        C = array_createarrayv (L, ARRAY_TINT, NULL, A->rank, A->size); \
                                                                        \
        for (j = 0, l = 1; j < A->rank ; l *= A->size[j], j += 1);      \
                                                                        \
        switch (abs(A->type)) {                                         \
        case ARRAY_TDOUBLE:                                             \
            right_scalar_##OPERATION##_doubles (A->values.doubles, b,   \
                                                C->values.ints, l);     \
                break;                                                  \
        case ARRAY_TFLOAT:                                              \
            right_scalar_##OPERATION##_floats (A->values.floats, b,     \
                                               C->values.ints, l);      \
                break;                                                  \
        case ARRAY_TUINT:                                               \
            right_scalar_##OPERATION##_uints (                          \
                A->values.uints,                                        \
                A->type < 0 ? b * UINT_MAX : b,                         \
                C->values.ints, l);                                     \
                break;                                                  \
        case ARRAY_TINT:                                                \
            right_scalar_##OPERATION##_ints (                           \
                A->values.ints,                                         \
                A->type < 0 ? b * INT_MAX : b,                          \
                C->values.ints, l);                                     \
                break;                                                  \
        case ARRAY_TUSHORT:                                             \
            right_scalar_##OPERATION##_ushorts (                        \
                A->values.ushorts,                                      \
                A->type < 0 ? b * USHRT_MAX : b,                        \
                C->values.ints, l);                                     \
                break;                                                  \
        case ARRAY_TSHORT:                                              \
            right_scalar_##OPERATION##_shorts (                         \
                A->values.shorts,                                       \
                A->type < 0 ? b * SHRT_MAX : b,                         \
                C->values.ints, l);                                     \
                break;                                                  \
        case ARRAY_TUCHAR:                                              \
            right_scalar_##OPERATION##_uchars (                         \
                A->values.uchars,                                       \
                A->type < 0 ? b * UCHAR_MAX : b,                        \
                C->values.ints, l);                                     \
                break;                                                  \
        case ARRAY_TCHAR:                                               \
            right_scalar_##OPERATION##_chars (                          \
                A->values.chars,                                        \
                A->type < 0 ? b * CHAR_MAX : b,                         \
                C->values.ints, l);                                     \
                break;                                                  \
        }                                                               \
                                                                        \
        return 1;                                                       \
    }                                                                   \
                                                                        \
    int arraymath_##OPERATION (lua_State *L)                            \
    {                                                                   \
        if (lua_type(L, 1) == LUA_TNUMBER) {                            \
            return left_scalar_##OPERATION(L);                          \
        } else if (lua_type(L, 2) == LUA_TNUMBER) {                     \
            return right_scalar_##OPERATION(L);                         \
        } else {                                                        \
            return pointwise_##OPERATION(L);                            \
        }                                                               \
    }

DEFINE_BINARY_LOGICAL_OPERATOR(greater, >)
DEFINE_BINARY_LOGICAL_OPERATOR(greaterequal, >=)
DEFINE_BINARY_LOGICAL_OPERATOR(less, <)
DEFINE_BINARY_LOGICAL_OPERATOR(lessequal, <=)
DEFINE_BINARY_LOGICAL_OPERATOR(equal, ==)
DEFINE_BINARY_LOGICAL_OPERATOR(logicaland, &&)
DEFINE_BINARY_LOGICAL_OPERATOR(logicalor, ||)

#define NOT(FUNC, TYPE)                         \
    static void FUNC (TYPE *A, int *C, int n)   \
    {                                           \
        int i;                                  \
                                                \
        for (i = 0 ; i < n ; i += 1) {          \
            C[i] = !A[i];                       \
        }                                       \
    }

NOT(not_doubles, double)
NOT(not_floats, float)

NOT(not_uints, unsigned int)
NOT(not_ints, signed int)
NOT(not_ushorts, unsigned short)
NOT(not_shorts, signed short)
NOT(not_uchars, unsigned char)
NOT(not_chars, signed char)

int arraymath_logicalnot (lua_State *L)
{
    array_Array *A, *C;
    int j, l;

    A = lua_touserdata (L, -1);

    C = array_createarrayv (L, ARRAY_TINT, NULL, A->rank, A->size);

    for (j = 0, l = 1; j < A->rank ; l *= A->size[j], j += 1);

    switch (abs(A->type)) {
    case ARRAY_TDOUBLE:
        not_doubles (A->values.doubles, C->values.ints, l);
        break;
    case ARRAY_TFLOAT:
        not_floats (A->values.floats, C->values.ints, l);
        break;
    case ARRAY_TUINT:
        not_uints (A->values.uints, C->values.ints, l);
        break;
    case ARRAY_TINT:
        not_ints (A->values.ints, C->values.ints, l);
        break;
    case ARRAY_TUSHORT:
        not_ushorts (A->values.ushorts, C->values.ints, l);
        break;
    case ARRAY_TSHORT:
        not_shorts (A->values.shorts, C->values.ints, l);
        break;
    case ARRAY_TUCHAR:
        not_uchars (A->values.uchars, C->values.ints, l);
        break;
    case ARRAY_TCHAR:
        not_chars (A->values.chars, C->values.ints, l);
        break;
    }

    return 1;
}

#define NEGATE(FUNC, TYPE)                      \
    static void FUNC (TYPE *A, TYPE *C, int n)  \
    {                                           \
        int i;                                  \
                                                \
        for (i = 0 ; i < n ; i += 1) {          \
            C[i] = -A[i];                       \
        }                                       \
    }

NEGATE(negate_doubles, double)
NEGATE(negate_floats, float)

NEGATE(negate_ints, signed int)
NEGATE(negate_shorts, signed short)
NEGATE(negate_chars, signed char)

int arraymath_negate (lua_State *L)
{
    array_Array *A, *C;
    int j, l;

    A = lua_touserdata (L, -1);

    C = array_createarrayv (L, A->type, NULL, A->rank, A->size);

    for (j = 0, l = 1; j < A->rank ; l *= A->size[j], j += 1);

    switch (abs(A->type)) {
    case ARRAY_TDOUBLE:
        negate_doubles (A->values.doubles, C->values.doubles, l);
        break;
    case ARRAY_TFLOAT:
        negate_floats (A->values.floats, C->values.floats, l);
        break;
    case ARRAY_TINT:
        negate_ints (A->values.ints, C->values.ints, l);
        break;
    case ARRAY_TSHORT:
        negate_shorts (A->values.shorts, C->values.shorts, l);
        break;
    case ARRAY_TCHAR:
        negate_chars (A->values.chars, C->values.chars, l);
        break;
    }

    return 1;
}

#define UNARY(FUNC, FUNC_2, TYPE)               \
    static void FUNC (TYPE *A, TYPE *C, int n)  \
    {                                           \
        int i;                                  \
                                                \
        for (i = 0 ; i < n ; i += 1) {          \
            C[i] = FUNC_2(A[i]);                \
        }                                       \
    }

#define DEFINE_UNARY_OPERATOR(FUNC, FUNC_2, FUNC_3, FUNC_4)             \
    UNARY(FUNC##_doubles, FUNC_2, double)                               \
    UNARY(FUNC##_floats, FUNC_3, float)                                 \
                                                                        \
    UNARY(FUNC##_uints, FUNC_4, unsigned int)                           \
    UNARY(FUNC##_ints, FUNC_4, signed int)                              \
    UNARY(FUNC##_ushorts, FUNC_4, unsigned short)                       \
    UNARY(FUNC##_shorts, FUNC_4, signed short)                          \
    UNARY(FUNC##_uchars, FUNC_4, unsigned char)                         \
    UNARY(FUNC##_chars, FUNC_4, signed char)                            \
                                                                        \
    int arraymath_##FUNC (lua_State *L)                                 \
    {                                                                   \
        array_Array *A, *C;                                             \
        int j, l;                                                       \
                                                                        \
        A = lua_touserdata (L, -1);                                     \
                                                                        \
        C = array_createarrayv (L, A->type, NULL, A->rank, A->size);    \
                                                                        \
        for (j = 0, l = 1; j < A->rank ; l *= A->size[j], j += 1);      \
                                                                        \
        switch (abs(A->type)) {                                         \
        case ARRAY_TDOUBLE:                                             \
            FUNC##_doubles (A->values.doubles, C->values.doubles, l);   \
                break;                                                  \
        case ARRAY_TFLOAT:                                              \
            FUNC##_floats (A->values.floats, C->values.floats, l);      \
                break;                                                  \
        case ARRAY_TUINT:                                               \
            FUNC##_uints (A->values.uints, C->values.uints, l);         \
                break;                                                  \
        case ARRAY_TINT:                                                \
            FUNC##_ints (A->values.ints, C->values.ints, l);            \
                break;                                                  \
        case ARRAY_TUSHORT:                                             \
            FUNC##_ushorts (A->values.ushorts, C->values.ushorts, l);   \
                break;                                                  \
        case ARRAY_TSHORT:                                              \
            FUNC##_shorts (A->values.shorts, C->values.shorts, l);      \
                break;                                                  \
        case ARRAY_TUCHAR:                                              \
            FUNC##_uchars (A->values.uchars, C->values.uchars, l);      \
                break;                                                  \
        case ARRAY_TCHAR:                                               \
            FUNC##_chars (A->values.chars, C->values.chars, l);         \
                break;                                                  \
        }                                                               \
                                                                        \
        return 1;                                                       \
    }

DEFINE_UNARY_OPERATOR(absolute, fabs, fabsf, abs)

#define CLAMP(FUNC, TYPE)                                               \
    static void FUNC (TYPE *A, TYPE a, TYPE b, TYPE *B,                 \
                      int n)                                            \
    {                                                                   \
        int i;                                                          \
                                                                        \
        for (i = 0 ; i < n ; i += 1) {                                  \
            B[i] = A[i] <= a ? a : (A[i] >= b ? b : A[i]);              \
        }                                                               \
    }

CLAMP(clamp_doubles, double)
CLAMP(clamp_floats, float)
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

    for (j = 0, l = 1; j < A->rank ; l *= A->size[j], j += 1);

    B = array_createarrayv (L, A->type, NULL, A->rank, A->size);

    switch (A->type) {
    case ARRAY_TDOUBLE:
        clamp_doubles (A->values.doubles, a, b, B->values.doubles, l);
        break;
    case ARRAY_TFLOAT:
        clamp_floats (A->values.floats, a, b, B->values.floats, l);
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
    case ARRAY_TNUINT:
        clamp_uints (A->values.uints, a * UINT_MAX, b * UINT_MAX,
                     B->values.uints, l);
        break;
    case ARRAY_TNINT:
        clamp_ints (A->values.ints, a * INT_MAX, b * INT_MAX,
                    B->values.ints, l);
        break;
    case ARRAY_TNUSHORT:
        clamp_ushorts (A->values.ushorts, a * USHRT_MAX, b * USHRT_MAX,
                       B->values.ushorts, l);
        break;
    case ARRAY_TNSHORT:
        clamp_shorts (A->values.shorts, a * SHRT_MAX, b * SHRT_MAX,
                      B->values.shorts, l);
        break;
    case ARRAY_TNUCHAR:
        clamp_uchars (A->values.uchars, a * UCHAR_MAX, b * UCHAR_MAX,
                      B->values.uchars, l);
        break;
    case ARRAY_TNCHAR:
        clamp_chars (A->values.chars, a * CHAR_MAX, b * CHAR_MAX,
                     B->values.chars, l);
        break;
    }

    return 1;
}

#define RANGE_2(FUNC, TYPE)                                             \
    static void FUNC (TYPE *A, TYPE *B, int n)                          \
    {                                                                   \
        int i;                                                          \
                                                                        \
        B[0] = B[1] = A[0];                                             \
                                                                        \
        for (i = 1 ; i < n ; i += 1) {                                  \
            if (A[i] < B[0]) {                                          \
                B[0] = A[i];                                            \
            }                                                           \
                                                                        \
            if (A[i] > B[1]) {                                          \
                B[1] = A[i];                                            \
            }                                                           \
        }                                                               \
    }

#define RANGE(FUNC, TYPE)                                               \
    RANGE_2(FUNC##_2, TYPE)                                             \
    static void FUNC (TYPE *A, TYPE *B, int k, int n)                   \
    {                                                                   \
        int j;                                                          \
                                                                        \
        for (j = 0 ; j < n / k ; j += 1) {                              \
            FUNC##_2 (&A[j * k], &B[j * 2], k);                         \
        }                                                               \
    }                                                                   \

RANGE(range_doubles, double)
RANGE(range_floats, float)

RANGE(range_uints, unsigned int)
RANGE(range_ints, signed int)
RANGE(range_ushorts, unsigned short)
RANGE(range_shorts, signed short)
RANGE(range_uchars, unsigned char)
RANGE(range_chars, signed char)

int arraymath_range (lua_State *L)
{
    array_Array *A, *B;
    int j, l, k;

    A = lua_touserdata (L, -1);

    for (j = 0, l = 1; j < A->rank ; l *= A->size[j], j += 1);
    k = A->size[A->rank - 1];

    {
        int i, size[A->rank];

        for (i = 0 ; i < A->rank - 1 ; i += 1) {
            size[i] = A->size[i];
        }

        size[A->rank - 1] = 2;

        B = array_createarrayv (L, A->type, NULL, A->rank, size);
    }

    switch (abs(A->type)) {
    case ARRAY_TDOUBLE:
        range_doubles (A->values.doubles, B->values.doubles,
                       k, l);
        break;
    case ARRAY_TFLOAT:
        range_floats (A->values.floats, B->values.floats,
                      k, l);
        break;
    case ARRAY_TUINT:
        range_uints (A->values.uints, B->values.uints,
                     k, l);
        break;
    case ARRAY_TINT:
        range_ints (A->values.ints, B->values.ints,
                    k, l);
        break;
    case ARRAY_TUSHORT:
        range_ushorts (A->values.ushorts, B->values.ushorts,
                       k, l);
        break;
    case ARRAY_TSHORT:
        range_shorts (A->values.shorts, B->values.shorts,
                      k, l);
        break;
    case ARRAY_TUCHAR:
        range_uchars (A->values.uchars, B->values.uchars,
                      k, l);
        break;
    case ARRAY_TCHAR:
        range_chars (A->values.chars, B->values.chars,
                     k, l);
        break;
    default:
        assert(0);
    }

    return 1;
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
    static void FUNC (TYPE *A, TYPE *S, int k, int n)                   \
    {                                                                   \
        int i;                                                          \
                                                                        \
        for (i = 0 ; i < n / k ; i += 1) {                              \
            FUNC##_2(&A[i * k], &S[i], k);                              \
        }                                                               \
    }

SUM(sum_doubles, double)
SUM(sum_floats, float)

SUM(sum_uints, unsigned int)
SUM(sum_ints, signed int)
SUM(sum_ushorts, unsigned short)
SUM(sum_shorts, signed short)
SUM(sum_uchars, unsigned char)
SUM(sum_chars, signed char)

int arraymath_sum (lua_State *L)
{
    array_Array *A, *S;
    int j, l, k;

    A = lua_touserdata (L, -1);

    for (j = 0, l = 1; j < A->rank ; l *= A->size[j], j += 1);
    k = A->size[A->rank - 1];

    if (A->rank > 1) {
        S = array_createarrayv (L, A->type, NULL, A->rank - 1, A->size);
    } else {
        S = array_createarray (L, A->type, NULL, 1, 1);
    }

    switch (abs(A->type)) {
    case ARRAY_TDOUBLE:
        sum_doubles (A->values.doubles, S->values.doubles, k, l);
        break;
    case ARRAY_TFLOAT:
        sum_floats (A->values.floats, S->values.floats, k, l);
        break;
    case ARRAY_TUINT:
        sum_uints (A->values.uints, S->values.uints, k, l);
        break;
    case ARRAY_TINT:
        sum_ints (A->values.ints, S->values.ints, k, l);
        break;
    case ARRAY_TUSHORT:
        sum_ushorts (A->values.ushorts, S->values.ushorts, k, l);
        break;
    case ARRAY_TSHORT:
        sum_shorts (A->values.shorts, S->values.shorts, k, l);
        break;
    case ARRAY_TUCHAR:
        sum_uchars (A->values.uchars, S->values.uchars, k, l);
        break;
    case ARRAY_TCHAR:
        sum_chars (A->values.chars, S->values.chars, k, l);
        break;
    default:
        assert(0);
    }

    if (A->rank == 1) {
        lua_pushinteger(L, 1);
        lua_gettable (L, -2);
        lua_replace (L, -2);
    }

    return 1;
}

#define PRODUCT(FUNC, TYPE)                                             \
    static void FUNC##_2 (TYPE *A, TYPE *S, int n)                      \
    {                                                                   \
        int i;                                                          \
                                                                        \
        *S = A[0];                                                      \
                                                                        \
        for (i = 1 ; i < n ; i += 1) {                                  \
            *S *= A[i];                                                 \
        }                                                               \
    }                                                                   \
                                                                        \
    static void FUNC (TYPE *A, TYPE *S, int k, int n)                   \
    {                                                                   \
        int i;                                                          \
                                                                        \
        for (i = 0 ; i < n / k ; i += 1) {                              \
            FUNC##_2(&A[i * k], &S[i], k);                              \
        }                                                               \
    }

#define PRODUCT_NORM(FUNC, SIGNEDNESS, TYPE)                            \
    static void FUNC##_2 (SIGNEDNESS TYPE *A, SIGNEDNESS TYPE *S,       \
                          int n)                                        \
    {                                                                   \
        int i;                                                          \
                                                                        \
        *S = A[0];                                                      \
                                                                        \
        for (i = 1 ; i < n ; i += 1) {                                  \
            *S = ((SIGNEDNESS long)(*S) * A[i]) >> bits(SIGNEDNESS, TYPE); \
        }                                                               \
    }                                                                   \
                                                                        \
    static void FUNC (SIGNEDNESS TYPE *A, SIGNEDNESS TYPE *S,           \
                      int k, int n)                                     \
    {                                                                   \
        int i;                                                          \
                                                                        \
        for (i = 0 ; i < n / k ; i += 1) {                              \
            FUNC##_2(&A[i * k], &S[i], k);                              \
        }                                                               \
    }

PRODUCT(product_doubles, double)
PRODUCT(product_floats, float)

PRODUCT(product_uints, unsigned int)
PRODUCT(product_ints, signed int)
PRODUCT(product_ushorts, unsigned short)
PRODUCT(product_shorts, signed short)
PRODUCT(product_uchars, unsigned char)
PRODUCT(product_chars, signed char)

PRODUCT_NORM(product_nuints, unsigned, int)
PRODUCT_NORM(product_nints, signed, int)
PRODUCT_NORM(product_nushorts, unsigned, short)
PRODUCT_NORM(product_nshorts, signed, short)
PRODUCT_NORM(product_nuchars, unsigned, char)
PRODUCT_NORM(product_nchars, signed, char)

int arraymath_product (lua_State *L)
{
    array_Array *A, *S;
    int j, l, k;

    A = lua_touserdata (L, -1);

    for (j = 0, l = 1; j < A->rank ; l *= A->size[j], j += 1);
    k = A->size[A->rank - 1];

    if (A->rank > 1) {
        S = array_createarrayv (L, A->type, NULL, A->rank - 1, A->size);
    } else {
        S = array_createarray (L, A->type, NULL, 1, 1);
    }

    switch (A->type) {
    case ARRAY_TDOUBLE:
        product_doubles (A->values.doubles, S->values.doubles, k, l);
        break;
    case ARRAY_TFLOAT:
        product_floats (A->values.floats, S->values.floats, k, l);
        break;
    case ARRAY_TUINT:
        product_uints (A->values.uints, S->values.uints, k, l);
        break;
    case ARRAY_TINT:
        product_ints (A->values.ints, S->values.ints, k, l);
        break;
    case ARRAY_TUSHORT:
        product_ushorts (A->values.ushorts, S->values.ushorts, k, l);
        break;
    case ARRAY_TSHORT:
        product_shorts (A->values.shorts, S->values.shorts, k, l);
        break;
    case ARRAY_TUCHAR:
        product_uchars (A->values.uchars, S->values.uchars, k, l);
        break;
    case ARRAY_TCHAR:
        product_chars (A->values.chars, S->values.chars, k, l);
        break;
    case ARRAY_TNUINT:
        product_nuints (A->values.uints, S->values.uints, k, l);
        break;
    case ARRAY_TNINT:
        product_nints (A->values.ints, S->values.ints, k, l);
        break;
    case ARRAY_TNUSHORT:
        product_nushorts (A->values.ushorts, S->values.ushorts, k, l);
        break;
    case ARRAY_TNSHORT:
        product_nshorts (A->values.shorts, S->values.shorts, k, l);
        break;
    case ARRAY_TNUCHAR:
        product_nuchars (A->values.uchars, S->values.uchars, k, l);
        break;
    case ARRAY_TNCHAR:
        product_nchars (A->values.chars, S->values.chars, k, l);
        break;
    default:
        assert(0);
    }

    if (A->rank == 1) {
        lua_pushinteger(L, 1);
        lua_gettable (L, -2);
        lua_replace (L, -2);
    }

    return 1;
}
#define POWER(FUNC, TYPE)                                        \
    static void FUNC (TYPE *A, TYPE *B, TYPE *C, int n)                 \
    {                                                                   \
        int i;                                                          \
                                                                        \
        for (i = 0 ; i < n ; i += 1) {                                  \
            C[i] = pow(A[i], B[i]);                                     \
        }                                                               \
    }                                                                   \
                                                                        \
    static void left_scalar_##FUNC (lua_Number a, TYPE *B, TYPE *C,     \
                                    int n)                              \
    {                                                                   \
        int i;                                                          \
                                                                        \
        for (i = 0 ; i < n ; i += 1) {                                  \
            C[i] = pow(a, B[i]);                                        \
        }                                                               \
    }                                                                   \
                                                                        \
    static void right_scalar_##FUNC (TYPE *A, lua_Number b, TYPE *C,    \
                                     int n)                             \
    {                                                                   \
        int i;                                                          \
                                                                        \
        for (i = 0 ; i < n ; i += 1) {                                  \
            C[i] = pow(A[i], b);                                        \
        }                                                               \
    }

POWER(power_doubles, double)
POWER(power_floats, float)

static int pointwise_power (lua_State *L)
{
    array_Array *A, *B, *C;
    int j, l;

    A = lua_touserdata (L, -2);
    B = lua_touserdata (L, -1);

    for (j = 0, l = 1; j < A->rank ; l *= A->size[j], j += 1);

    C = array_createarrayv (L, A->type, NULL, A->rank, A->size);

    if (A->type == ARRAY_TDOUBLE) {
        power_doubles (A->values.doubles, B->values.doubles,
                              C->values.doubles, l);
    } else {
        power_floats (A->values.floats, B->values.floats,
                             C->values.floats, l);
    }

    return 1;
}

static int right_scalar_power (lua_State *L)
{
    array_Array *A, *C;
    double b;
    int j, l;

    A = lua_touserdata (L, -2);
    b = lua_tonumber (L, -1);

    for (j = 0, l = 1; j < A->rank ; l *= A->size[j], j += 1);

    C = array_createarrayv (L, A->type, NULL, A->rank, A->size);

    if (A->type == ARRAY_TDOUBLE) {
        right_scalar_power_doubles (A->values.doubles, b,
                                           C->values.doubles, l);
    } else {
        right_scalar_power_floats (A->values.floats, b,
                                          C->values.floats, l);
    }

    return 1;
}

static int left_scalar_power (lua_State *L)
{
    array_Array *B, *C;
    double a;
    int j, l;

    a = lua_tonumber (L, -2);
    B = lua_touserdata (L, -1);

    for (j = 0, l = 1; j < B->rank ; l *= B->size[j], j += 1);

    C = array_createarrayv (L, B->type, NULL, B->rank, B->size);

    if (B->type == ARRAY_TDOUBLE) {
        left_scalar_power_doubles (a, B->values.doubles,
                                          C->values.doubles, l);
    } else {
        left_scalar_power_floats (a, B->values.floats,
                                         C->values.floats, l);
    }

    return 1;
}

int arraymath_power (lua_State *L)
{
    if (lua_type(L, 1) == LUA_TNUMBER) {
        return left_scalar_power(L);
    } else if (lua_type(L, 2) == LUA_TNUMBER) {
        return right_scalar_power(L);
    } else {
        return pointwise_power(L);
    }
}

#define TRANSCENDENTAL(FUNC, FUNC_2, TYPE)                              \
    static void FUNC (TYPE *A, TYPE *C, int n)                          \
    {                                                                   \
        int i;                                                          \
                                                                        \
        for (i = 0 ; i < n ; i += 1) {                                  \
            C[i] = FUNC_2(A[i]);                                        \
        }                                                               \
    }                                                                   \

#define DEFINE_TRANSCENDENTAL_FUNCTION(FUNC, FUNC_2)                    \
    TRANSCENDENTAL(FUNC##_doubles, FUNC_2, double)                      \
    TRANSCENDENTAL(FUNC##_floats, FUNC_2##f, float)                     \
                                                                        \
    int arraymath_##FUNC (lua_State *L)                                 \
    {                                                                   \
        array_Array *A, *C;                                             \
        int j, l;                                                       \
                                                                        \
        A = lua_touserdata (L, -1);                                     \
                                                                        \
        for (j = 0, l = 1; j < A->rank ; l *= A->size[j], j += 1);      \
                                                                        \
        C = array_createarrayv (L, A->type, NULL, A->rank, A->size);    \
                                                                        \
        if (A->type == ARRAY_TDOUBLE) {                                 \
            FUNC##_doubles (A->values.doubles, C->values.doubles, l);   \
        } else {                                                        \
            FUNC##_floats (A->values.floats, C->values.floats, l);      \
        }                                                               \
                                                                        \
        return 1;                                                       \
    }

DEFINE_TRANSCENDENTAL_FUNCTION(ceiling, ceil)
DEFINE_TRANSCENDENTAL_FUNCTION(floor, floor)
DEFINE_TRANSCENDENTAL_FUNCTION(sine, sin)
DEFINE_TRANSCENDENTAL_FUNCTION(cosine, cos)
DEFINE_TRANSCENDENTAL_FUNCTION(tangent, tan)
DEFINE_TRANSCENDENTAL_FUNCTION(arcsine, asin)
DEFINE_TRANSCENDENTAL_FUNCTION(arccosine, acos)
DEFINE_TRANSCENDENTAL_FUNCTION(arctangent, atan)
DEFINE_TRANSCENDENTAL_FUNCTION(logarithm, log)

#define DOT(FUNC, TYPE)                                         \
    static double FUNC (TYPE *A, TYPE *B, int n)                \
    {                                                           \
        double d;                                               \
        int i;                                                  \
                                                                \
        for (i = 0, d = 0 ; i < n ; d += A[i] * B[i], i += 1);  \
                                                                \
        return d;                                               \
    }

DOT(dot_doubles, double)
DOT(dot_floats, float)

#define DISTANCE(FUNC, TYPE)                                            \
    static double FUNC (TYPE *A, TYPE *B, int n)                        \
    {                                                                   \
        double d, r;                                                    \
        int i;                                                          \
                                                                        \
        for (i = 0, d = 0;                                              \
             i < n;                                                     \
             r = A[i] - B[i], d += r * r, i += 1);                      \
                                                                        \
        return d;                                                       \
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

#define CROSS(FUNC, TYPE)                                               \
    static void FUNC (TYPE *A, TYPE *B, TYPE *C)                        \
    {                                                                   \
        C[0] = A[1] * B[2] - A[2] * B[1];                               \
        C[1] = A[2] * B[0] - A[0] * B[2];                               \
        C[2] = A[0] * B[1] - A[1] * B[0];                               \
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

#define VECTOR_MATRIX(FUNC, TYPE)                                       \
    static void FUNC (TYPE *A, TYPE *B, TYPE *C, int n, int m)          \
    {                                                                   \
        int i, j;                                                       \
                                                                        \
        for (i = 0 ; i < m ; i += 1) {                                  \
            C[i] = 0;                                                   \
                                                                        \
            for (j = 0 ; j < n ; j += 1) {                              \
                C[i] += A[j] * B[j * m + i];                            \
            }                                                           \
        }                                                               \
    }

VECTOR_MATRIX(vector_matrix_doubles, double)
VECTOR_MATRIX(vector_matrix_floats, float)

#define MATRIX_VECTOR(FUNC, TYPE)                                       \
    static void FUNC (TYPE *A, TYPE *B, TYPE *C, int n, int m)          \
    {                                                                   \
        int i, j;                                                       \
                                                                        \
        for (i = 0 ; i < n ; i += 1) {                                  \
            C[i] = 0;                                                   \
                                                                        \
            for (j = 0 ; j < m ; j += 1) {                              \
                C[i] += A[i * m + j] * B[j];                            \
            }                                                           \
        }                                                               \
    }

MATRIX_VECTOR(matrix_vector_doubles, double)
MATRIX_VECTOR(matrix_vector_floats, float)

#define MATRIX_MATRIX(FUNC, TYPE)                                       \
    static void FUNC (TYPE *A, TYPE *B, TYPE *C,                        \
                      int n, int m, int s, int t)                       \
    {                                                                   \
        int i, j, k;                                                    \
                                                                        \
        for (i = 0 ; i < n ; i += 1) {                                  \
            for (k = 0 ; k < t ; k += 1) {                              \
                C[i * t + k] = 0;                                       \
                                                                        \
                for (j = 0 ; j < m ; j += 1) {                          \
                    C[i * t + k] += A[i * m + j] * B[j * t + k];        \
                }                                                       \
            }                                                           \
        }                                                               \
    }

MATRIX_MATRIX(matrix_matrix_doubles, double)
MATRIX_MATRIX(matrix_matrix_floats, float)

array_Array *arraymath_matrixmultiply (lua_State *L)
{
    array_Array *A, *B, *C;

    A = lua_touserdata (L, -2);
    B = lua_touserdata (L, -1);

    if (A->rank == 1) {
        C = array_createarray (L, A->type, NULL, 1, B->size[1]);

        if (A->type == ARRAY_TDOUBLE) {
            vector_matrix_doubles (A->values.doubles,
                                   B->values.doubles,
                                   C->values.doubles,
                                   B->size[0], B->size[1]);
        } else {
            vector_matrix_floats (A->values.floats,
                                  B->values.floats,
                                  C->values.floats,
                                  B->size[0], B->size[1]);
        }
    } else if (B->rank == 1) {
        C = array_createarray (L, A->type, NULL, 1, A->size[0]);

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
    } else {
        C = array_createarray (L, A->type, NULL, 2, A->size[0], B->size[1]);

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
    }

    return C;
}

#define APPLY_REAL(FUNC, TYPE)                                          \
    static void FUNC##_real (double *T, double *f, TYPE *v, TYPE *r)    \
    {                                                                   \
        r[0] = T[0] * v[0] + T[1] * v[1] + T[2] * v[2];                 \
        r[1] = T[3] * v[0] + T[4] * v[1] + T[5] * v[2];                 \
        r[2] = T[6] * v[0] + T[7] * v[1] + T[8] * v[2];                 \
                                                                        \
        if (f) {                                                        \
            r[0] += f[0] - (T[0] * f[0] + T[1] * f[1] + T[2] * f[2]);   \
            r[1] += f[1] - (T[3] * f[0] + T[4] * f[1] + T[5] * f[2]);   \
            r[2] += f[2] - (T[6] * f[0] + T[7] * f[1] + T[8] * f[2]);   \
        }                                                               \
    }

#define APPLY(FUNC, TYPE)                                               \
    static void FUNC (double *T, double *f, TYPE *v, TYPE *r,           \
                      int rank, int *size)                              \
    {                                                                   \
        if (rank == 1) {                                                \
            FUNC##_real(T, f, v, r);                                    \
        } else {                                                        \
            int j, d;                                                   \
                                                                        \
            for (j = 1, d = 1 ; j < rank ; d *= size[j], j += 1);       \
                                                                        \
            for (j = 0 ; j < size[0] ; j += 1) {                        \
                FUNC (T, f, &v[j * d], &r[j * d], rank - 1, &size[1]);  \
            }                                                           \
        }                                                               \
    }                                                                   \

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
