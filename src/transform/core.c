/* Copyright (C) 2012 Papavasileiou Dimitris                             
 *                                                                      
 * This program is free software: you can redistribute it and/or modify 
 * it under the terms of the GNU General Public License as published by 
 * the Free Software Foundation, either version 3 of the License, or    
 * (at your option) any later version.                                  
 *                                                                      
 * This program is distributed in the hope that it will be useful,      
 * but WITHOUT ANY WARRANTY; without even the implied warranty of       
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the        
 * GNU General Public License for more details.                         
 *                                                                      
 * You should have received a copy of the GNU General Public License    
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <assert.h>

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include "array/array.h"

static int size[2] = {3, 3};

#define APPLY(FUNC, TYPE)						\
    static void FUNC (double *T, double *f, TYPE *v, TYPE *r,		\
		      int rank, int *size)				\
    {									\
	if (rank == 1) {						\
	    r[0] = T[0] * v[0] + T[1] * v[1] + T[2] * v[2];		\
	    r[1] = T[3] * v[0] + T[4] * v[1] + T[5] * v[2];		\
	    r[2] = T[6] * v[0] + T[7] * v[1] + T[8] * v[2];		\
									\
	    if (f) {							\
		r[0] += f[0] - (T[0] * f[0] + T[1] * f[1] + T[2] * f[2]); \
		r[1] += f[1] - (T[3] * f[0] + T[4] * f[1] + T[5] * f[2]); \
		r[2] += f[2] - (T[6] * f[0] + T[7] * f[1] + T[8] * f[2]); \
	    }								\
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

APPLY(apply_doubles, double)
APPLY(apply_floats, float)

static int multiply (lua_State *L);
static void pusharray (lua_State *L, double *M)
{
    array_Array array;

    array.rank = 2;
    array.size = size;
    array.length = 3 * 3 * sizeof(double);
    array.type = ARRAY_TDOUBLE;
    array.values.doubles = M;
    array.free = FREE_VALUES;

    array_pusharray (L, &array);

    lua_getmetatable (L, -1);
    lua_pushcfunction (L, multiply);
    lua_setfield (L, -2, "__concat");
    lua_pop(L, 1);
}

static int concatenate (lua_State *L)
{
    array_Array *array;
    double alpha[9], beta[9], *A, *B, *C;
    int i;

    array = array_checkcompatible (L, 1,
                                   ARRAY_TYPE | ARRAY_RANK | ARRAY_SIZE,
                                   ARRAY_TDOUBLE, 2, 3, 3);
    A = array->values.doubles;
    C = alpha;
    
    for (i = 2 ; i <= lua_gettop (L) ; i += 1) {
	array = array_checkcompatible (L, i,
                                       ARRAY_TYPE | ARRAY_RANK | ARRAY_SIZE,
                                       ARRAY_TDOUBLE, 2, 3, 3);
	B = array->values.doubles;

	C[0] = A[0] * B[0] + A[1] * B[3] + A[2] * B[6];
	C[1] = A[0] * B[1] + A[1] * B[4] + A[2] * B[7];
	C[2] = A[0] * B[2] + A[1] * B[5] + A[2] * B[8];

	C[3] = A[3] * B[0] + A[4] * B[3] + A[5] * B[6];
	C[4] = A[3] * B[1] + A[4] * B[4] + A[5] * B[7];
	C[5] = A[3] * B[2] + A[4] * B[5] + A[5] * B[8];

	C[6] = A[6] * B[0] + A[7] * B[3] + A[8] * B[6];
	C[7] = A[6] * B[1] + A[7] * B[4] + A[8] * B[7];
	C[8] = A[6] * B[2] + A[7] * B[5] + A[8] * B[8];

	if (i != lua_gettop(L)) {
	    A = C;
	    C = (C == alpha) ? beta : alpha;
	}
    }

    B = malloc (9 * sizeof (double));
    memcpy (B, C, 9 * sizeof (double));
    pusharray (L, B);

    return 1;
}

static int apply (lua_State *L)
{
    array_Array *transform, *data, *result, *fixed;
    
    transform = array_checkcompatible (L, 1,
                                       ARRAY_TYPE | ARRAY_RANK | ARRAY_SIZE,
                                       ARRAY_TDOUBLE, 2, 3, 3);

    if (lua_type (L, 2) == LUA_TTABLE) {
	data = array_checkcompatible (L, 2,
                                      ARRAY_TYPE | ARRAY_RANK | ARRAY_SIZE,
                                      ARRAY_TYPE, ARRAY_TDOUBLE);
    } else {
	data = array_checkarray (L, 2);
    
	if (data->type != ARRAY_TDOUBLE &&
	    data->type != ARRAY_TFLOAT) {
	    luaL_argerror (L, 2, "Specified array has integer type.");
	}
    }

    if (data->size[data->rank - 1] != 3) {
	lua_pushstring (L,
			"Array to be transformed must be "
			"ultimately three-dimensional.");
	lua_error (L);
    }

    fixed = array_testcompatible (L, 3,
                                  ARRAY_TYPE | ARRAY_RANK | ARRAY_SIZE,
                                  ARRAY_TDOUBLE, 1, 3);

    array_createarrayv (L, data->type, NULL, data->rank, data->size);
    result = lua_touserdata (L, -1);

    if (data->type == ARRAY_TDOUBLE) {
	apply_doubles (transform->values.doubles,
		       fixed ? fixed->values.doubles : NULL,
		       data->values.doubles,
		       result->values.doubles, result->rank, result->size);
    } else {
	assert (data->type == ARRAY_TFLOAT);
	apply_floats (transform->values.doubles,
		      fixed ? fixed->values.doubles : 0,
		      data->values.floats,
		      result->values.floats, result->rank, result->size);
    }
    
    return 1;
}

static int scaling (lua_State *L)
{
    array_Array *A;
    double *M, *u;

    luaL_checkany (L, 1);

    if (lua_isnumber (L, 1)) {
	M = malloc (9 * sizeof (double));

	M[0] = M[4] = M[8] = lua_tonumber (L, 1);
    } else {
	A = array_checkcompatible (L, 1,
                                   ARRAY_TYPE | ARRAY_RANK | ARRAY_SIZE,
                                   ARRAY_TDOUBLE, 1, 3);
	u = A->values.doubles;

	M = malloc (9 * sizeof (double));

	M[0] = u[0];
	M[4] = u[1];
	M[8] = u[2];
    }
    
    M[1] = M[2] = M[3] = M[5] = M[6] = M[7] = 0;
	
    pusharray (L, M);
    
    return 1;
}

static int shear (lua_State *L)
{
    array_Array *A, *B;
    double *M, *u, *n, f;

    f = luaL_checknumber (L, 1);
    
    A = array_checkcompatible (L, 2,
                               ARRAY_TYPE | ARRAY_RANK | ARRAY_SIZE,
                               ARRAY_TDOUBLE, 1, 3);
    u = A->values.doubles;

    B = array_checkcompatible (L, 3,
                               ARRAY_TYPE | ARRAY_RANK | ARRAY_SIZE,
                               ARRAY_TDOUBLE, 1, 3);
    n = B->values.doubles;

    M = malloc (9 * sizeof (double));

    M[0] = 1 + f * u[0] * n[0];
    M[1] = f * u[0] * n[1];
    M[2] = f * u[0] * n[2];

    M[3] = f * u[1] * n[0];
    M[4] = 1 + f * u[1] * n[1];
    M[5] = f * u[1] * n[2];

    M[6] = f * u[2] * n[0];
    M[7] = f * u[2] * n[1];
    M[8] = 1 + f * u[2] * n[2];
   
    pusharray (L, M);
    
    return 1;
}

static int identity (lua_State *L)
{
    double *M;

    M = malloc (9 * sizeof (double));

    M[0] = 1; M[1] = 0; M[2] = 0;
    M[3] = 0; M[4] = 1; M[5] = 0;
    M[6] = 0; M[7] = 0; M[8] = 1;
   
    pusharray (L, M);
    
    return 1;
}

static int reflection (lua_State *L)
{
    array_Array *A;
    double *M, *u;

    A = array_checkcompatible (L, 1,
                               ARRAY_TYPE | ARRAY_RANK | ARRAY_SIZE,
                               ARRAY_TDOUBLE, 1, 3);
    u = A->values.doubles;

    M = malloc (9 * sizeof (double));

    M[0] = 1 - 2 * u[0] * u[0];
    M[1] = 2 * u[0] * u[1];
    M[2] = 2 * u[0] * u[2];

    M[3] = M[1];
    M[4] = 1 - 2 * u[1] * u[1];
    M[5] = 2 * u[1] * u[2];

    M[6] = M[2]; M[7] = M[5]; M[8] = 1 - 2 * u[2] * u[2];
   
    pusharray (L, M);
    
    return 1;
}

static int projection (lua_State *L)
{
    array_Array *A, *B;
    double *M, *u, *v;

    if (lua_gettop (L) == 1) {
	A = array_checkcompatible (L, 1,
                                   ARRAY_TYPE | ARRAY_RANK | ARRAY_SIZE,
                                   ARRAY_TDOUBLE, 1, 3);
	u = A->values.doubles;

	M = malloc (9 * sizeof (double));

	M[0] = u[0] * u[0]; M[1] = u[0] * u[1]; M[2] = u[0] * u[2];
	M[3] = M[1]; M[4] = u[1] * u[1]; M[5] = u[1] * u[2];
	M[6] = M[2]; M[7] = M[5]; M[8] = u[2] * u[2];
    } else {
	A = array_checkcompatible (L, 1,
                                   ARRAY_TYPE | ARRAY_RANK | ARRAY_SIZE,
                                   ARRAY_TDOUBLE, 1, 3);
	B = array_checkcompatible (L, 2,
                                   ARRAY_TYPE | ARRAY_RANK | ARRAY_SIZE,
                                   ARRAY_TDOUBLE, 1, 3);

	u = A->values.doubles;
	v = B->values.doubles;

	M = malloc (9 * sizeof (double));

        M[0] = v[0] * v[0] + u[0] * u[0];
	M[1] = v[0] * v[1] + u[0] * u[1];
	M[2] = v[0] * v[2] + u[0] * u[2];
	
	M[3] = M[1];
	M[4] = v[1] * v[1] + u[1] * u[1];
	M[5] = v[1] * v[2] + u[1] * u[2];

	M[6] = M[2];
	M[7] = M[5];
	M[8] = v[2] * v[2] + u[2] * u[2];
    }
   
    pusharray (L, M);
    
    return 1;
}

static int rotation (lua_State *L)
{
    double *M;

    M = malloc (9 * sizeof (double));

    if (lua_type (L, 1) == LUA_TNUMBER &&
	lua_type (L, 2) == LUA_TNUMBER) {
	double theta, c, s;
	int n;

	theta = lua_tonumber (L, 1);
	n = lua_tointeger (L, 2);
	
	c = cos(theta);
	s = sin(theta);
	
	if (n == 1) {
	    /* Rotate around x. */
	    
	    M[0] = 1; M[1] = 0; M[2] = 0; 
	    M[3] = 0; M[4] = c; M[5] = -s; 
	    M[6] = 0; M[7] = s; M[8] = c;
	} else if (n == 2) {
	    /* Rotate around y. */
	    
	    M[0] = c; M[1] = 0; M[2] = s; 
	    M[3] = 0; M[4] = 1; M[5] = 0; 
	    M[6] = -s; M[7] = 0; M[8] = c;
	} else if (n == 3) {
	    /* Rotate around z. */
	    
	    M[0] = c; M[1] = -s; M[2] = 0; 
	    M[3] = s; M[4] = c; M[5] = 0; 
	    M[6] = 0; M[7] = 0; M[8] = 1;
	} else {
	    free(M);

	    lua_pushstring (L, "Invalid rotation axis.");
	    lua_error (L);
	}
    } else if (lua_type (L, 1) == LUA_TNUMBER &&
	       array_testcompatible (L, 2,
                                     ARRAY_TYPE | ARRAY_RANK | ARRAY_SIZE,
                                     ARRAY_TDOUBLE, 1, 3)) {
	double theta, c, c_1, s, *u;

	theta = lua_tonumber (L, 1);
	u = ((array_Array *)lua_touserdata (L, 2))->values.doubles;

	c = cos(theta);
	c_1 = 1.0 - c;
	s = sin(theta);

	M[0] = c + u[0] * u[0] * c_1;
	M[1] = u[0] * u[1] * c_1 - u[2] * s;
	M[2] = u[0] * u[2] * c_1 + u[1] * s;

	M[3] = u[1] * u[0] * c_1 + u[2] * s;
	M[4] = c + u[1] * u[1] * c_1;
	M[5] = u[1] * u[2] * c_1 - u[0] * s;

	M[6] = u[2] * u[0] * c_1 - u[1] * s;
	M[7] = u[2] * u[1] * c_1 + u[0] * s;
	M[8] = c + u[2] * u[2] * c_1;
    } else {
	free(M);

	lua_pushstring (L, "Invalid rotation argument combination.");
	lua_error (L);
    }

    pusharray (L, M);
    
    return 1;
}

static array_Array *checkproper (lua_State *L, int index, int rank)
{
    array_Array *array;
    
    array = array_checkarray (L, index);
    
    if (array->type != ARRAY_TDOUBLE &&
	array->type != ARRAY_TFLOAT) {
	luaL_argerror (L, index, "Specified array has integer type.");
    }
    
    if ((rank == 0 && array->rank != 1 && array->rank != 2) ||
	(rank > 0 && array->rank != rank)) {
	luaL_argerror (L, index,
		       "Specified array has incompatible "
		       "dimensionality.");
    }

    return array;
}

static void checksimilar (lua_State *L,
			     array_Array *A, array_Array *B,
			     int rank)
{
    if (A->type != B->type) {
	lua_pushstring (L, "Array types don't match.");
	lua_error (L);
    }

    if (A->size[0] != B->size[0]) {
	lua_pushstring (L, "Array sizes don't match.");
	lua_error (L);
    }

    if (rank != 0 && A->size[0] != rank) {
	lua_pushstring (L, "Arrays have improper dimension.");
	lua_error (L);
    }
}

static int basis (lua_State *L)
{
    array_Array *array;
    double *M;
    int i, j, n;

    n = luaL_checknumber (L, 1);
    i = luaL_checknumber (L, 2);

    array_createarray (L, ARRAY_TDOUBLE, NULL, 1, n);
    array = lua_touserdata (L, -1);

    M = array->values.doubles;

    for (j = 0 ; j < n ; j += 1) {
	M[j] = (j == (i - 1));
    }
    
    return 1;
}

static int diagonal (lua_State *L)
{
    array_Array *array, *diagonal;
    double *M, d;
    int j, k, n;

    if (lua_type (L, 1) == LUA_TNUMBER) {
	n = luaL_checknumber (L, 1);
	d = luaL_checknumber (L, 2);

	array_createarray (L, ARRAY_TDOUBLE, NULL, 2, n, n);
	array = lua_touserdata (L, -1);

	M = array->values.doubles;

	for (j = 0 ; j < n ; j += 1) {
	    for (k = 0 ; k < n ; k += 1) {
		M[j * n + k] = (j == k) ? d : 0;
	    }
	}
    } else {
	diagonal = array_checkcompatible (L, 1,
                                          ARRAY_TYPE | ARRAY_RANK | ARRAY_SIZE,
                                          ARRAY_TDOUBLE, 1, 0);
	n = diagonal->size[0];
	
	array_createarray (L, ARRAY_TDOUBLE, NULL, 2, n, n);

	array = lua_touserdata (L, -1);
	M = array->values.doubles;
    
	for (j = 0 ; j < n ; j += 1) {
	    for (k = 0 ; k < n ; k += 1) {
		M[j * n + k] = (j == k) ? diagonal->values.doubles[j] : 0;
	    }
	}
    }
    
    return 1;
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

static int dot (lua_State *L)
{
    array_Array *A, *B;
    double d;

    A = checkproper (L, 1, 1);
    B = checkproper (L, 2, 1);
    
    checksimilar (L, A, B, 0);
    
    if (A->type == ARRAY_TDOUBLE) {
	d = dot_doubles (A->values.doubles, B->values.doubles, A->size[0]);
    } else {
	d = dot_floats (A->values.floats, B->values.floats, A->size[0]);
    }

    lua_pushnumber (L, d);
    
    return 1;
}

static int lengthsquared (lua_State *L)
{
    array_Array *A;
    double d;

    A = checkproper (L, 1, 1);
    
    if (A->type == ARRAY_TDOUBLE) {
	d = dot_doubles (A->values.doubles, A->values.doubles, A->size[0]);
    } else {
	d = dot_floats (A->values.floats, A->values.floats, A->size[0]);
    }

    lua_pushnumber (L, d);
    
    return 1;
}

static int length (lua_State *L)
{
    array_Array *A;
    double d;

    A = checkproper (L, 1, 1);
    
    if (A->type == ARRAY_TDOUBLE) {
	d = sqrt(dot_doubles (A->values.doubles,
			      A->values.doubles,
			      A->size[0]));
    } else {
	d = sqrt(dot_floats (A->values.floats,
			     A->values.floats,
			     A->size[0]));
    }

    lua_pushnumber (L, d);
    
    return 1;
}

static int normalize (lua_State *L)
{
    array_Array *A, *B;
    double d;
    int i;

    A = checkproper (L, 1, 1);

    array_createarray (L, A->type, NULL, 1, A->size[0]);
    B = lua_touserdata (L, -1);
    
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

    lua_pushnumber (L, d);
    
    return 1;
}

static int distancesquared (lua_State *L)
{
    array_Array *A, *B;
    double d;

    A = checkproper (L, 1, 1);
    B = checkproper (L, 2, 1);
    
    checksimilar (L, A, B, 0);
    
    if (A->type == ARRAY_TDOUBLE) {
	d = distance_doubles (A->values.doubles, B->values.doubles, A->size[0]);
    } else {
	d = distance_floats (A->values.floats, B->values.floats, A->size[0]);
    }

    lua_pushnumber (L, d);
    
    return 1;
}

static int distance (lua_State *L)
{
    array_Array *A, *B;
    double d;

    A = checkproper (L, 1, 1);
    B = checkproper (L, 2, 1);
    
    checksimilar (L, A, B, 0);
    
    if (A->type == ARRAY_TDOUBLE) {
	d = sqrt(distance_doubles (A->values.doubles,
				   B->values.doubles,
				   A->size[0]));
    } else {
	d = sqrt(distance_floats (A->values.floats,
				  B->values.floats,
				  A->size[0]));
    }

    lua_pushnumber (L, d);
    
    return 1;
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

static int transpose (lua_State *L)
{
    array_Array *A, *B;

    A = checkproper (L, 1, 2);

    array_createarray (L, A->type, NULL,
		       A->rank, A->size[1], A->size[0]);
    B = (array_Array *)lua_touserdata (L, -1);

    if (A->type == ARRAY_TDOUBLE) {
	transpose_doubles (A->values.doubles,
			   B->values.doubles,
			   A->size[0], A->size[1]);
    } else {
	transpose_floats (A->values.floats,
			  B->values.floats,
			  A->size[0], A->size[1]);
    }
    
    return 1;
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

static int multiply (lua_State *L)
{
    array_Array *A, *B, *C;

    A = checkproper (L, 1, 2);
    B = checkproper (L, 2, 0);

    if (A->size[1] != B->size[0]) {
	lua_pushstring (L, "Arrays are incompatible.");
	lua_error (L);
    }

    if (B->rank == 1) {
	array_createarray (L, A->type, NULL, 1, A->size[0]);
	C = (array_Array *)lua_touserdata (L, -1);

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
	array_createarray (L, A->type, NULL, 2, A->size[0], B->size[1]);
	C = (array_Array *)lua_touserdata (L, -1);

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
    
    return 1;
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

static int cross (lua_State *L)
{
    array_Array *A, *B, *C;

    A = checkproper (L, 1, 1);
    B = checkproper (L, 2, 1);
    
    checksimilar (L, A, B, 3);

    array_createarray (L, A->type, NULL, 1, 3);
    C = (array_Array *)lua_touserdata (L, -1);
    
    if (A->type == ARRAY_TDOUBLE) {
	cross_doubles (A->values.doubles,
		       B->values.doubles,
		       C->values.doubles);
    } else {
	cross_floats (A->values.floats,
		      B->values.floats,
		      C->values.floats);
    }
    
    return 1;
}

#define INTERPOLATE(FUNC, TYPE)						\
    static void FUNC (TYPE *A, TYPE *B, double c, TYPE *C, int n)	\
    {									\
        int i;								\
									\
        for (i = 0 ; i < n ; i += 1) {					\
	    C[i] = A[i] + c * (B[i] - A[i]);				\
	}								\
    }

INTERPOLATE(interpolate_doubles, double)
INTERPOLATE(interpolate_floats, float)

static int interpolate (lua_State *L)
{
    array_Array *A, *B, *C;
    double c;

    A = checkproper (L, 1, 1);
    B = checkproper (L, 2, 1);
    c = luaL_checknumber (L, 3);
    
    checksimilar (L, A, B, 3);

    array_createarray (L, A->type, NULL, 1, A->size[0]);
    C = (array_Array *)lua_touserdata (L, -1);
    
    if (A->type == ARRAY_TDOUBLE) {
	interpolate_doubles (A->values.doubles,
			     B->values.doubles,
			     c,
			     C->values.doubles,
			     A->size[0]);
    } else {
	interpolate_floats (A->values.floats,
			    B->values.floats,
			    c,
			    C->values.floats,
			    A->size[0]);
    }
    
    return 1;
}

int luaopen_transform_core (lua_State *L)
{
    const luaL_Reg api[] = {
	{"identity", identity},
	{"rotation", rotation},
	{"shear", shear},
	{"projection", projection},
	{"scaling", scaling},
	{"reflection", reflection},
	{"concatenate", concatenate},
	{"apply", apply},

	{"basis", basis},
	{"diagonal", diagonal},
	{"dot", dot},
	{"cross", cross},
	{"interpolate", interpolate},
	{"lengthsquared", lengthsquared},
	{"length", length},
	{"normalize", normalize},
	{"distancesquared", distancesquared},
	{"distance", distance},
	{"transpose", transpose},
	{"multiply", multiply},
	
	{NULL, NULL}
    };

    luaL_newlib (L, api);

    return 1;
}
