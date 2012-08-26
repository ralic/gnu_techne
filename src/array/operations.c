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

#include <string.h>
#include <stdlib.h>
#include <math.h>

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include "array.h"

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
    int array_##OPERATION (lua_State *L)                                \
    {									\
	array_Array *A, *B, *C;						\
	int j, l;							\
									\
	A = array_checkarray (L, -2);					\
	B = array_checkarray (L, -1);					\
									\
	if (A->rank != B->rank) {					\
	    lua_pushstring (L, "Array dimensions don't match.");	\
	    lua_error (L);						\
	}								\
									\
	if (A->type != B->type) {					\
	    lua_pushstring (L, "Array types don't match.");		\
	    lua_error (L);						\
	}								\
									\
	for (j = 0 ,l = 1; j < A->rank ; j += 1) {			\
	    if (A->size[j] != B->size[j]) {				\
		lua_pushstring (L, "Array sizes don't match.");		\
		lua_error (L);						\
	    }								\
									\
	    l *= A->size[j];						\
	}								\
									\
	array_createarrayv (L, A->type, NULL, A->rank, A->size);	\
	C = (array_Array *)lua_touserdata (L, -1);			\
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

#define SCALE(FUNC, TYPE)					\
    static void FUNC (TYPE *A, lua_Number c, TYPE *B, int n)	\
    {								\
	int i;							\
								\
	for (i = 0 ; i < n ; i += 1) {				\
	    B[i] = c * A[i];					\
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

int array_scale (lua_State *L)
{
    array_Array *A, *B;
    double c;
    int j, l;

    A = array_checkarray (L, -2);
    c = luaL_checknumber (L, -1);

    for (j = 0 ,l = 1; j < A->rank ; l *= A->size[j], j += 1);

    array_createarrayv (L, A->type, NULL, A->rank, A->size);
    B = (array_Array *)lua_touserdata (L, -1);

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
    static void FUNC (TYPE *A, lua_Number e, TYPE *B, int n, const double c)	\
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

int array_raise (lua_State *L)
{
    array_Array *A, *B;
    double c;
    int j, l;

    A = array_checkarray (L, -2);
    c = luaL_checknumber (L, -1);

    for (j = 0, l = 1; j < A->rank ; l *= A->size[j], j += 1);

    array_createarrayv (L, A->type, NULL, A->rank, A->size);
    B = (array_Array *)lua_touserdata (L, -1);

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
