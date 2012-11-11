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

static array_Array *pusharray (lua_State *L, double *M)
{
    array_Array array, *result;

    array.rank = 2;
    array.size = size;
    array.length = 3 * 3 * sizeof(double);
    array.type = ARRAY_TDOUBLE;
    array.values.doubles = M;
    array.free = FREE_VALUES;

    result = array_pusharray (L, &array);

    /* lua_getmetatable (L, -1); */
    /* lua_pushcfunction (L, multiply); */
    /* lua_setfield (L, -2, "__concat"); */
    /* lua_pop(L, 1); */

    return result;
}

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

static array_Array *transform_apply (lua_State *L, int i)
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
	assert (data->type == ARRAY_TFLOAT);
	apply_floats (transform->values.doubles,
		      fixed ? fixed->values.doubles : 0,
		      data->values.floats,
		      result->values.floats, result->rank, result->size);
    }
    
    return result;
}

array_Array *transform_concatenate (lua_State *L, int n)
{
    array_Array *array;
    double alpha[9], beta[9], *A, *B, *C;
    int i;

    array = lua_touserdata (L, -n);
    A = array->values.doubles;
    C = alpha;
    
    for (i = -n + 1 ; i <= -1 ; i += 1) {
	array = lua_touserdata (L, i);
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

	if (i != -1) {
	    A = C;
	    C = (C == alpha) ? beta : alpha;
	}
    }

    lua_pop(L, n);
    
    B = malloc (9 * sizeof (double));
    memcpy (B, C, 9 * sizeof (double));

    return pusharray (L, B);
}

/* Lua API. */

static int apply (lua_State *L)
{
    array_Array *data;
    
    array_checkcompatible (L, 1,
			   ARRAY_TYPE | ARRAY_RANK | ARRAY_SIZE,
			   ARRAY_TDOUBLE, 2, 3, 3);

    if (lua_type (L, 2) == LUA_TTABLE) {
	data = array_checkcompatible (L, 2, ARRAY_TYPE, ARRAY_TDOUBLE);
    } else {
	data = array_checkarray (L, 2);
    }
    
    if (data->type != ARRAY_TDOUBLE &&
	data->type != ARRAY_TFLOAT) {
	luaL_argerror (L, 2, "Specified array has integer type.");
    }

    if (data->size[data->rank - 1] != 3) {
	lua_pushstring (L,
			"Array to be transformed must be "
			"ultimately three-dimensional.");
	lua_error (L);
    }

    if (lua_gettop (L) > 2) {
	array_checkcompatible (L, 3,
			       ARRAY_TYPE | ARRAY_RANK | ARRAY_SIZE,
			       ARRAY_TDOUBLE, 1, 3);

	transform_apply (L, 3);
    } else {
	transform_apply (L, 0);
    }	
    
    return 1;
}

static int concatenate (lua_State *L)
{
    int i, n;

    n = lua_gettop(L);

    for (i = 1 ; i <= n ; i += 1) {
	array_checkcompatible (L, i,
			       ARRAY_TYPE | ARRAY_RANK | ARRAY_SIZE,
			       ARRAY_TDOUBLE, 2, 3, 3);
    }

    transform_concatenate(L, n);
    
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
    array_Array *array;
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
	       (array = array_testcompatible (L, 2,
                                              ARRAY_TYPE | ARRAY_RANK | ARRAY_SIZE,
                                              ARRAY_TDOUBLE, 1, 3))) {
	double theta, c, c_1, s, *u;

	theta = lua_tonumber (L, 1);
	u = array->values.doubles;

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

static int basis (lua_State *L)
{
    array_Array *array;
    double *M;
    int i, j, n;

    n = luaL_checknumber (L, 1);
    i = luaL_checknumber (L, 2);

    array = array_createarray (L, ARRAY_TDOUBLE, NULL, 1, n);

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

	array = array_createarray (L, ARRAY_TDOUBLE, NULL, 2, n, n);

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
	
	array = array_createarray (L, ARRAY_TDOUBLE, NULL, 2, n, n);
	M = array->values.doubles;
    
	for (j = 0 ; j < n ; j += 1) {
	    for (k = 0 ; k < n ; k += 1) {
		M[j * n + k] = (j == k) ? diagonal->values.doubles[j] : 0;
	    }
	}
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
	
	{NULL, NULL}
    };

    luaL_newlib (L, api);

    return 1;
}
