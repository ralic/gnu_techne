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

#include "array/array.h"
#include "arraymath.h"

#define I(i, j) (j * 3 + i)

static array_Array *pushtransform (lua_State *L, double *M)
{
    array_Array array, *result;

    array_initialize (&array, ARRAY_TDOUBLE, M, 2, 3, 3);
    result = array_pusharray (L, &array);

    return result;
}

static array_Array *checkreal (lua_State *L, int index)
{
    array_Array *array;

    array = array_checkarray (L, index);

    if (array->type != ARRAY_TDOUBLE &&
        array->type != ARRAY_TFLOAT) {
        luaL_argerror (L, index, "array has integral type");
    }

    return array;
}

static void checkrank (lua_State *L, int index, int rank)
{
    array_Array *array;

    array = lua_touserdata (L, index);

    if ((rank == 0 && array->rank != 1 && array->rank != 2) ||
        (rank > 0 && array->rank != rank)) {
        luaL_argerror (L, index, "array has incompatible rank");
    }
}

static void checksimilar (lua_State *L, int i, int j)
{
    array_Array *A, *B;

    A = lua_touserdata(L, i);
    B = lua_touserdata(L, j);

    if (A->type != B->type) {
        luaL_argerror (L, j, "array type doesn't match");
        lua_error (L);
    }

    if (A->rank != B->rank) {
        luaL_argerror (L, j, "array rank doesn't match");
        lua_error (L);
    }

    if (memcmp(A->size, B->size, A->rank * sizeof(int))) {
        luaL_argerror (L, j, "array size doesn't match");
        lua_error (L);
    }
}

/* Arithmetic operations API. */

#define DEFINE_OPERATION_WRAPPER(OP)            \
    static int OP (lua_State *L)                \
    {                                           \
        if (lua_type (L, 1) != LUA_TNUMBER) {   \
            array_checkarray (L, 1);            \
        }                                       \
                                                \
        if (lua_type (L, 2) != LUA_TNUMBER) {   \
            array_checkarray (L, 2);            \
        }                                       \
                                                \
        if (lua_type (L, 1) != LUA_TNUMBER &&   \
            lua_type (L, 2) != LUA_TNUMBER) {   \
            checksimilar(L, 1, 2);              \
        }                                       \
                                                \
        arraymath_##OP(L);                      \
                                                \
        return 1;                               \
    }

DEFINE_OPERATION_WRAPPER(add)
DEFINE_OPERATION_WRAPPER(multiply)
DEFINE_OPERATION_WRAPPER(subtract)
DEFINE_OPERATION_WRAPPER(divide)

DEFINE_OPERATION_WRAPPER(greater)
DEFINE_OPERATION_WRAPPER(greaterequal)
DEFINE_OPERATION_WRAPPER(less)
DEFINE_OPERATION_WRAPPER(lessequal)
DEFINE_OPERATION_WRAPPER(equal)
DEFINE_OPERATION_WRAPPER(logicaland)
DEFINE_OPERATION_WRAPPER(logicalor)

DEFINE_OPERATION_WRAPPER(power)

#define DEFINE_UNARY_OPERATION_WRAPPER(OP)      \
    static int OP (lua_State *L)                \
    {                                           \
        array_checkarray (L, 1);                \
                                                \
        arraymath_##OP(L);                      \
                                                \
        return 1;                               \
    }

DEFINE_UNARY_OPERATION_WRAPPER (negate)
DEFINE_UNARY_OPERATION_WRAPPER (absolute)
DEFINE_UNARY_OPERATION_WRAPPER (logicalnot)
DEFINE_UNARY_OPERATION_WRAPPER (range)
DEFINE_UNARY_OPERATION_WRAPPER (sum)
DEFINE_UNARY_OPERATION_WRAPPER (product)

#define DEFINE_TRANSCENDENTAL_WRAPPER(FUNC, OP) \
    static int FUNC (lua_State *L)              \
    {                                           \
        array_checkarray (L, 1);                \
                                                \
        arraymath_##OP(L);                      \
                                                \
        return 1;                               \
    }

DEFINE_TRANSCENDENTAL_WRAPPER(ceiling, ceiling)
DEFINE_TRANSCENDENTAL_WRAPPER(_floor, floor)
DEFINE_TRANSCENDENTAL_WRAPPER(sine, sine)
DEFINE_TRANSCENDENTAL_WRAPPER(cosine, cosine)
DEFINE_TRANSCENDENTAL_WRAPPER(tangent, tangent)
DEFINE_TRANSCENDENTAL_WRAPPER(arcsine, arcsine)
DEFINE_TRANSCENDENTAL_WRAPPER(arccosine, arccosine)
DEFINE_TRANSCENDENTAL_WRAPPER(arctangent, arctangent)
DEFINE_TRANSCENDENTAL_WRAPPER(logarithm, logarithm)

static int clamp (lua_State *L)
{
    array_checkarray (L, 1);
    luaL_checknumber (L, 2);
    luaL_checknumber (L, 3);

    arraymath_clamp(L);

    return 1;
}

/* Linear algebra API. */

static int dot (lua_State *L)
{
    checkreal(L, 1);
    checkreal(L, 2);
    checkrank(L, 1, 1);
    checkrank(L, 2, 1);
    checksimilar (L, 1, 2);

    lua_pushnumber (L, arraymath_dot(L));

    return 1;
}

static int cross (lua_State *L)
{
    array_Array *A;

    A = checkreal(L, 1);
    checkreal(L, 2);
    checkrank(L, 1, 1);
    checkrank(L, 2, 1);
    checksimilar (L, 1, 2);

    if (A->size[0] != 3) {
        lua_pushstring (L, "Vectors must be three-dimensional.");
        lua_error (L);
    }

    arraymath_cross(L);

    return 1;
}

static int normalize (lua_State *L)
{
    checkreal(L, 1);
    checkrank (L, 1, 1);
    arraymath_normalize(L);

    return 1;
}

static int length (lua_State *L)
{
    checkreal(L, 1);
    checkrank (L, 1, 1);
    lua_pushnumber (L, arraymath_length(L));

    return 1;
}

static int distance (lua_State *L)
{
    checkreal(L, 1);
    checkreal(L, 2);
    checkrank(L, 1, 1);
    checkrank(L, 2, 1);
    checksimilar (L, 1, 2);

    lua_pushnumber (L, arraymath_distance(L));

    return 1;
}

static int matrixmultiply (lua_State *L)
{
    array_Array *A, *B;

    A = checkreal (L, 1);
    B = checkreal (L, 2);

    checkrank (L, 1, 0);
    checkrank (L, 2, A->rank == 1 ? 2 : 0);

    if (A->type != B->type ||
        (A->rank == 1 && A->size[0] != B->size[1]) ||
        ((B->rank == 1 || A->rank == B->rank) && A->size[1] != B->size[0])) {

        luaL_argerror (L, 2, "operands are incompatible");
    }

    arraymath_matrixmultiply(L);

    return 1;
}

/* Geometric transform API. */

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

        arraymath_apply (L, 3);
    } else {
        arraymath_apply (L, 0);
    }

    return 1;
}

static int scaling (lua_State *L)
{
    array_Array *A;
    double M[9], *u;

    luaL_checkany (L, 1);

    if (lua_isnumber (L, 1)) {
        M[I(0, 0)] = M[I(1, 1)] = M[I(2, 2)] = lua_tonumber (L, 1);
    } else {
        A = array_checkcompatible (L, 1,
                                   ARRAY_TYPE | ARRAY_RANK | ARRAY_SIZE,
                                   ARRAY_TDOUBLE, 1, 3);
        u = A->values.doubles;

        M[I(0, 0)] = u[0];
        M[I(1, 1)] = u[1];
        M[I(2, 2)] = u[2];
    }

    M[I(0, 1)] = M[I(0, 2)] = M[I(1, 0)] =
        M[I(1, 2)] = M[I(2, 0)] = M[I(2, 1)] = 0;

    pushtransform (L, M);

    return 1;
}

static int shear (lua_State *L)
{
    array_Array *A, *B;
    double M[9], *u, *n, f;

    f = luaL_checknumber (L, 1);

    A = array_checkcompatible (L, 2,
                               ARRAY_TYPE | ARRAY_RANK | ARRAY_SIZE,
                               ARRAY_TDOUBLE, 1, 3);
    u = A->values.doubles;

    B = array_checkcompatible (L, 3,
                               ARRAY_TYPE | ARRAY_RANK | ARRAY_SIZE,
                               ARRAY_TDOUBLE, 1, 3);
    n = B->values.doubles;

    M[I(0, 0)] = 1 + f * u[0] * n[0];
    M[I(0, 1)] = f * u[0] * n[1];
    M[I(0, 2)] = f * u[0] * n[2];

    M[I(1, 0)] = f * u[1] * n[0];
    M[I(1, 1)] = 1 + f * u[1] * n[1];
    M[I(1, 2)] = f * u[1] * n[2];

    M[I(2, 0)] = f * u[2] * n[0];
    M[I(2, 1)] = f * u[2] * n[1];
    M[I(2, 2)] = 1 + f * u[2] * n[2];

    pushtransform (L, M);

    return 1;
}

static int reflection (lua_State *L)
{
    array_Array *A;
    double M[9], *u;

    A = array_checkcompatible (L, 1,
                               ARRAY_TYPE | ARRAY_RANK | ARRAY_SIZE,
                               ARRAY_TDOUBLE, 1, 3);
    u = A->values.doubles;

    M[I(0, 0)] = 1 - 2 * u[0] * u[0];
    M[I(0, 1)] = 2 * u[0] * u[1];
    M[I(0, 2)] = 2 * u[0] * u[2];

    M[I(1, 0)] = M[I(0, 1)];
    M[I(1, 1)] = 1 - 2 * u[1] * u[1];
    M[I(1, 2)] = 2 * u[1] * u[2];

    M[I(2, 0)] = M[I(0, 2)];
    M[I(2, 1)] = M[I(1, 2)];
    M[I(2, 2)] = 1 - 2 * u[2] * u[2];

    pushtransform (L, M);

    return 1;
}

static int projection (lua_State *L)
{
    array_Array *A, *B;
    double M[9], *u, *v;

    if (lua_gettop (L) == 1) {
        A = array_checkcompatible (L, 1,
                                   ARRAY_TYPE | ARRAY_RANK | ARRAY_SIZE,
                                   ARRAY_TDOUBLE, 1, 3);
        u = A->values.doubles;

        M[I(0, 0)] = u[0] * u[0];
        M[I(0, 1)] = u[0] * u[1];
        M[I(0, 2)] = u[0] * u[2];

        M[I(1, 0)] = M[I(0, 1)];
        M[I(1, 1)] = u[1] * u[1];
        M[I(1, 2)] = u[1] * u[2];

        M[I(2, 0)] = M[I(0, 2)];
        M[I(2, 1)] = M[I(1, 2)];
        M[I(2, 2)] = u[2] * u[2];
    } else {
        A = array_checkcompatible (L, 1,
                                   ARRAY_TYPE | ARRAY_RANK | ARRAY_SIZE,
                                   ARRAY_TDOUBLE, 1, 3);
        B = array_checkcompatible (L, 2,
                                   ARRAY_TYPE | ARRAY_RANK | ARRAY_SIZE,
                                   ARRAY_TDOUBLE, 1, 3);

        u = A->values.doubles;
        v = B->values.doubles;

        M[I(0, 0)] = v[0] * v[0] + u[0] * u[0];
        M[I(0, 1)] = v[0] * v[1] + u[0] * u[1];
        M[I(0, 2)] = v[0] * v[2] + u[0] * u[2];

        M[I(1, 0)] = M[I(0, 1)];
        M[I(1, 1)] = v[1] * v[1] + u[1] * u[1];
        M[I(1, 2)] = v[1] * v[2] + u[1] * u[2];

        M[I(2, 0)] = M[I(0, 2)];
        M[I(2, 1)] = M[I(1, 2)];
        M[I(2, 2)] = v[2] * v[2] + u[2] * u[2];
    }

    pushtransform (L, M);

    return 1;
}

static int rotation (lua_State *L)
{
    array_Array *array;
    double M[9];

    if ((array = array_testcompatible (L, 1,
                                       ARRAY_TYPE | ARRAY_RANK | ARRAY_SIZE,
                                       ARRAY_TDOUBLE, 1, 4))) {
        double *q, xx, xy, xz, xt, yy, yz, yt, zz, zt;

        q = array->values.doubles;

        /* Convert the quaternion to a rotation matrix. */

        xx = q[0] * q[0];
        xy = q[0] * q[1];
        xz = q[0] * q[2];
        xt = q[0] * q[3];
        yy = q[1] * q[1];
        yz = q[1] * q[2];
        yt = q[1] * q[3];
        zz = q[2] * q[2];
        zt = q[2] * q[3];

        M[I(0, 0)] = 1 - 2 * (yy + zz);
        M[I(0, 1)] = 2 * (xy - zt);
        M[I(0, 2)] = 2 * (xz + yt);

        M[I(1, 0)] = 2 * (xy + zt);
        M[I(1, 1)] = 1 - 2 * (xx + zz);
        M[I(1, 2)] = 2 * (yz - xt);

        M[I(2, 0)] = 2 * (xz - yt);
        M[I(2, 1)] = 2 * (yz + xt);
        M[I(2, 2)] = 1 - 2 * (xx + yy);
    } else if (lua_type (L, 1) == LUA_TNUMBER &&
        lua_type (L, 2) == LUA_TNUMBER) {
        double theta, c, s;
        int n;

        theta = lua_tonumber (L, 1);
        n = lua_tointeger (L, 2);

        c = cos(theta);
        s = sin(theta);

        if (n == 1) {
            /* Rotate around x. */

            M[I(0, 0)] = 1; M[I(0, 1)] = 0; M[I(0, 2)] = 0;
            M[I(1, 0)] = 0; M[I(1, 1)] = c; M[I(1, 2)] = -s;
            M[I(2, 0)] = 0; M[I(2, 1)] = s; M[I(2, 2)] = c;
        } else if (n == 2) {
            /* Rotate around y. */

            M[I(0, 0)] = c; M[I(0, 1)] = 0; M[I(0, 2)] = s;
            M[I(1, 0)] = 0; M[I(1, 1)] = 1; M[I(1, 2)] = 0;
            M[I(2, 0)] = -s; M[I(2, 1)] = 0; M[I(2, 2)] = c;
        } else if (n == 3) {
            /* Rotate around z. */

            M[I(0, 0)] = c; M[I(0, 1)] = -s; M[I(0, 2)] = 0;
            M[I(1, 0)] = s; M[I(1, 1)] = c; M[I(1, 2)] = 0;
            M[I(2, 0)] = 0; M[I(2, 1)] = 0; M[I(2, 2)] = 1;
        } else {
            lua_pushstring (L, "Invalid rotation axis.");
            lua_error (L);
        }
    } else if (lua_type (L, 1) == LUA_TNUMBER &&
               (array = array_testcompatible (L, 2,
                                              ARRAY_TYPE | ARRAY_RANK |
                                              ARRAY_SIZE,
                                              ARRAY_TDOUBLE, 1, 3))) {
        double theta, c, c_1, s, *u;

        theta = lua_tonumber (L, 1);
        u = array->values.doubles;

        c = cos(theta);
        c_1 = 1.0 - c;
        s = sin(theta);

        M[I(0, 0)] = c + u[0] * u[0] * c_1;
        M[I(0, 1)] = u[0] * u[1] * c_1 - u[2] * s;
        M[I(0, 2)] = u[0] * u[2] * c_1 + u[1] * s;

        M[I(1, 0)] = u[1] * u[0] * c_1 + u[2] * s;
        M[I(1, 1)] = c + u[1] * u[1] * c_1;
        M[I(1, 2)] = u[1] * u[2] * c_1 - u[0] * s;

        M[I(2, 0)] = u[2] * u[0] * c_1 - u[1] * s;
        M[I(2, 1)] = u[2] * u[1] * c_1 + u[0] * s;
        M[I(2, 2)] = c + u[2] * u[2] * c_1;
    } else {
        lua_pushstring (L, "Invalid rotation argument combination.");
        lua_error (L);
    }

    pushtransform (L, M);

    return 1;
}

/* Miscellaneous matrices and vectors API. */

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

int luaopen_arraymath_core (lua_State *L)
{
    const luaL_Reg api[] = {
        {"add", add},
        {"multiply", multiply},
        {"subtract", subtract},
        {"divide", divide},
        {"negate", negate},

        {"greater", greater},
        {"greaterequal", greaterequal},
        {"less", less},
        {"lessequal", lessequal},
        {"equal", equal},
        {"logicaland", logicaland},
        {"logicalor", logicalor},
        {"logicalnot", logicalnot},

        {"absolute", absolute},
        {"clamp", clamp},
        {"range", range},
        {"sum", sum},
        {"product", product},

        {"floor", _floor},
        {"ceiling", ceiling},
        {"power", power},
        {"arcsine", arcsine},
        {"arccosine", arccosine},
        {"arctangent", arctangent},
        {"sine", sine},
        {"cosine", cosine},
        {"tangent", tangent},
        {"logarithm", logarithm},

        {"dot", dot},
        {"cross", cross},
        {"length", length},
        {"normalize", normalize},
        {"distance", distance},
        {"matrixmultiply", matrixmultiply},

        {"rotation", rotation},
        {"shear", shear},
        {"projection", projection},
        {"scaling", scaling},
        {"reflection", reflection},
        {"apply", apply},

        {"basis", basis},
        {"diagonal", diagonal},

        {NULL, NULL}
    };

#if LUA_VERSION_NUM == 501
    luaL_register (L, "arraymath", api);
#else
    luaL_newlib (L, api);
#endif

    /* Get the metatable of arrays. */

    array_createarray (L, ARRAY_TDOUBLE, NULL, 1, 1);
    lua_getmetatable(L, -1);
    lua_replace(L, -2);

    lua_pushstring(L, "__add");
    lua_pushcfunction(L, (lua_CFunction)add);
    lua_settable(L, -3);

    lua_pushstring(L, "__sub");
    lua_pushcfunction(L, (lua_CFunction)subtract);
    lua_settable(L, -3);

    lua_pushstring(L, "__mul");
    lua_pushcfunction(L, (lua_CFunction)multiply);
    lua_settable(L, -3);

    lua_pushstring(L, "__div");
    lua_pushcfunction(L, (lua_CFunction)divide);
    lua_settable(L, -3);

    lua_pushstring(L, "__unm");
    lua_pushcfunction(L, (lua_CFunction)negate);
    lua_settable(L, -3);

    lua_pushstring(L, "__pow");
    lua_pushcfunction(L, (lua_CFunction)power);
    lua_settable(L, -3);

    lua_pushstring(L, "__concat");
    lua_pushcfunction(L, (lua_CFunction)matrixmultiply);
    lua_settable(L, -3);

    lua_pop(L, 1);

    return 1;
}
