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

double array_vector_dot (lua_State *L)
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

void array_vector_cross (lua_State *L)
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

double array_vector_squaredlength (lua_State *L)
{
    array_Array *A;
    double d;

    A = lua_touserdata(L, -1);
    
    if (A->type == ARRAY_TDOUBLE) {
	d = dot_doubles (A->values.doubles, A->values.doubles, A->size[0]);
    } else {
	d = dot_floats (A->values.floats, A->values.floats, A->size[0]);
    }

    return d;
}

double array_vector_length (lua_State *L)
{
    return sqrt(array_vector_squaredlength(L));
}

double array_vector_squareddistance (lua_State *L)
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

    return d;
}

double array_vector_distance (lua_State *L)
{
    return sqrt(array_vector_squareddistance(L));
}

array_Array *array_vector_normalize (lua_State *L)
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

#define TRANSPOSE(FUNC, TYPE)						\
    static void FUNC (TYPE *A, TYPE *B, int n, int m)			\
    {									\
	int i, j;							\
									\
	for (i = 0 ; i < n ; i += 1) {					\
	    for (j = 0 ; j < m ; j += 1) {				\
		B[j * n + i] = A[i * m + j];				\
	    }								\
	}								\
    }

TRANSPOSE(transpose_doubles, double)
TRANSPOSE(transpose_floats, float)

void array_matrix_transpose (lua_State *L)
{
    array_Array *A, *B;

    A = lua_touserdata (L, -1);
    B = array_createarray (L, A->type, NULL,
                           A->rank, A->size[1], A->size[0]);

    if (A->type == ARRAY_TDOUBLE) {
	transpose_doubles (A->values.doubles,
			   B->values.doubles,
			   A->size[0], A->size[1]);
    } else {
	transpose_floats (A->values.floats,
			  B->values.floats,
			  A->size[0], A->size[1]);
    }
}

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

MATRIX_VECTOR(matrix_vector_doubles, double)
MATRIX_VECTOR(matrix_vector_floats, float)

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

MATRIX_MATRIX(matrix_matrix_doubles, double)
MATRIX_MATRIX(matrix_matrix_floats, float)

void array_matrix_multiply (lua_State *L)
{
    array_Array *A, *B, *C;

    A = lua_touserdata (L, -2);
    B = lua_touserdata (L, -1);

    if (B->rank == 1) {
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
}

/* Lua API wrappers. */

static array_Array *checktype (lua_State *L, int index)
{
    array_Array *array;
    
    array = array_checkarray (L, index);
    
    if (array->type != ARRAY_TDOUBLE &&
	array->type != ARRAY_TFLOAT) {
	luaL_argerror (L, index, "Specified array has integral type.");
    }

    return array;
}

static array_Array *checkrank (lua_State *L, int index, int rank)
{
    array_Array *array;
    
    array = checktype (L, index);
    
    if ((rank == 0 && array->rank != 1 && array->rank != 2) ||
	(rank > 0 && array->rank != rank)) {
	luaL_argerror (L, index,
		       "Specified array has incompatible "
		       "dimensionality.");
    }

    return array;
}

static array_Array *checksimilar (lua_State *L, int i, int j, int rank)
{
    array_Array *A, *B;

    A = checkrank(L, i, rank);
    B = checkrank(L, j, rank);
    
    if (A->type != B->type) {
	lua_pushstring (L, "Array types don't match.");
	lua_error (L);
    }

    if (memcmp(A->size, B->size, A->rank * sizeof(int))) {
	lua_pushstring (L, "Array sizes don't match.");
	lua_error (L);
    }

    return A;
}

static int dot (lua_State *L)
{
    checksimilar (L, 1, 2, 1);
    
    lua_pushnumber (L, array_vector_dot(L));
    
    return 1;
}

static int cross (lua_State *L)
{
    array_Array *A;
    
    A = checksimilar (L, 1, 2, 1);

    if (A->size[0] != 3) {
	lua_pushstring (L, "Vectors must be three-dimensional.");
	lua_error (L);
    }

    array_vector_cross(L);
    
    return 1;
}

static int normalize (lua_State *L)
{
    checkrank (L, 1, 1);
    array_vector_normalize(L);
    
    return 1;
}

static int squaredlength (lua_State *L)
{
    checkrank (L, 1, 1);
    lua_pushnumber (L, array_vector_squaredlength(L));
    
    return 1;
}

static int length (lua_State *L)
{
    checkrank (L, 1, 1);
    lua_pushnumber (L, array_vector_length(L));
    
    return 1;
}

static int squareddistance (lua_State *L)
{
    checksimilar (L, 1, 2, 1);

    lua_pushnumber (L, array_vector_squareddistance(L));
    
    return 1;
}

static int distance (lua_State *L)
{
    checksimilar (L, 1, 2, 1);

    lua_pushnumber (L, array_vector_distance(L));
    
    return 1;
}

static int transpose (lua_State *L)
{
    array_Array *A;

    A = checkrank (L, 1, 2);

    if (A->size[0] != A->size[1]) {
	lua_pushstring (L, "Matrix is not square.");
	lua_error (L);
    }

    array_matrix_transpose(L);
    
    return 1;
}

static int multiply (lua_State *L)
{
    array_Array *A, *B;

    A = checkrank (L, 1, 2);
    B = checkrank (L, 2, 0);

    if (A->size[1] != B->size[0]) {
	lua_pushstring (L, "Matrices are incompatible.");
	lua_error (L);
    }

    array_matrix_multiply(L);
    
    return 1;
}

int luaopen_array_algebra (lua_State *L)
{
    const luaL_Reg api[] = {
	{"dot", dot},
	{"cross", cross},
	{"squaredlength", squaredlength},
	{"length", length},
	{"normalize", normalize},
	{"squareddistance", squareddistance},
	{"distance", distance},
	{"transpose", transpose},
	{"multiply", multiply},
	    
	{NULL, NULL}
    };


#if LUA_VERSION_NUM == 501
    lua_newtable(L);
    luaL_register (L, NULL, api);
#else
    luaL_newlib (L, api);
#endif
    
    return 1;
}
