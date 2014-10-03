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

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#include <assert.h>

#include "array.h"

#if LUA_VERSION_NUM == 501
#define lua_rawlen lua_objlen
#endif

#define absolute(_L, _i) (_i < 0 ? lua_gettop (_L) + _i + 1 : _i)

static int metatable = LUA_REFNIL;
static const void *signature;

static int __index (lua_State *L);
static int __newindex (lua_State *L);
static int __gc (lua_State *L);
static int __len (lua_State *L);
static int __ipairs (lua_State *L);
static int __tostring (lua_State *L);

static array_Array *testarray (lua_State *L, int index);

static size_t sizeof_element (array_Type type)
{
    switch (abs(type)) {
    case ARRAY_TDOUBLE:
        return sizeof (double);
    case ARRAY_TFLOAT:
        return sizeof (float);
    case ARRAY_TUINT:
        return sizeof (unsigned int);
    case ARRAY_TINT:
        return sizeof (int);
    case ARRAY_TUSHORT:
        return sizeof (unsigned short);
    case ARRAY_TSHORT:
        return sizeof (short);
    case ARRAY_TUCHAR:
        return sizeof (unsigned char);
    case ARRAY_TCHAR:
        return sizeof (char);
    }

    return 0;
}

static void write_element (array_Array *array, int i, lua_Number value)
{
    switch (array->type) {
    case ARRAY_TDOUBLE:
        array->values.doubles[i] = value;
        break;
    case ARRAY_TFLOAT:
        array->values.floats[i] = value;
        break;
    case ARRAY_TUINT:
        array->values.uints[i] = value;
        break;
    case ARRAY_TNUINT:
        array->values.uints[i] = value * UINT_MAX;
        break;
    case ARRAY_TINT:
        array->values.ints[i] = value;
        break;
    case ARRAY_TNINT:
        array->values.ints[i] = value * INT_MAX;
        break;
    case ARRAY_TUSHORT:
        array->values.ushorts[i] = value;
        break;
    case ARRAY_TNUSHORT:
        array->values.ushorts[i] = value * USHRT_MAX;
        break;
    case ARRAY_TSHORT:
        array->values.shorts[i] = value;
        break;
    case ARRAY_TNSHORT:
        array->values.shorts[i] = value * SHRT_MAX;
        break;
    case ARRAY_TUCHAR:
        array->values.uchars[i] = value;
        break;
    case ARRAY_TNUCHAR:
        array->values.uchars[i] = value * UCHAR_MAX;
        break;
    case ARRAY_TCHAR:
        array->values.chars[i] = value;
        break;
    case ARRAY_TNCHAR:
        array->values.chars[i] = value * CHAR_MAX;
        break;
    }
}

static lua_Number read_element (array_Array *array, int i)
{
    switch (array->type) {
    case ARRAY_TDOUBLE:
        return (lua_Number)array->values.doubles[i];
    case ARRAY_TFLOAT:
        return (lua_Number)array->values.floats[i];
    case ARRAY_TUINT:
        return (lua_Number)array->values.uints[i];
    case ARRAY_TNUINT:
        return (lua_Number)array->values.uints[i] / UINT_MAX;
    case ARRAY_TINT:
        return (lua_Number)array->values.ints[i];
    case ARRAY_TNINT:
        return (lua_Number)array->values.ints[i] / INT_MAX;
    case ARRAY_TUSHORT:
        return (lua_Number)array->values.ushorts[i];
    case ARRAY_TNUSHORT:
        return (lua_Number)array->values.ushorts[i] / USHRT_MAX;
    case ARRAY_TSHORT:
        return (lua_Number)array->values.shorts[i];
    case ARRAY_TNSHORT:
        return (lua_Number)array->values.shorts[i] / SHRT_MAX;
    case ARRAY_TUCHAR:
        return (lua_Number)array->values.uchars[i];
    case ARRAY_TNUCHAR:
        return (lua_Number)array->values.uchars[i] / UCHAR_MAX;
    case ARRAY_TCHAR:
        return (lua_Number)array->values.chars[i];
    case ARRAY_TNCHAR:
        return (lua_Number)array->values.chars[i] / CHAR_MAX;
    }

    return 0;
}

static void *reference_element (array_Array *array, int i)
{
    switch (abs(array->type)) {
    case ARRAY_TDOUBLE:
        return &array->values.doubles[i];
    case ARRAY_TFLOAT:
        return &array->values.floats[i];
    case ARRAY_TUINT:
        return &array->values.uints[i];
    case ARRAY_TINT:
        return &array->values.ints[i];
    case ARRAY_TUSHORT:
        return &array->values.ushorts[i];
    case ARRAY_TSHORT:
        return &array->values.shorts[i];
    case ARRAY_TUCHAR:
        return &array->values.uchars[i];
    case ARRAY_TCHAR:
        return &array->values.chars[i];
    }

    return NULL;
}

static void copy_values (array_Array *array, void *values, int i, int j, int n)
{
    int w;

    w = sizeof_element(array->type);
    memcpy(((char *)array->values.any) + i * w, ((char *)values) + j * w, n * w);
}

static void copy_elements (array_Array *to, array_Array *from, int i, int j, int n)
{
    assert(from->type == to->type);
    copy_values (to, from->values.any, i, j, n);
}

static void zero_elements (array_Array *array, int i, int n)
{
    int w;

    w = sizeof_element(array->type);
    memset(((char *)array->values.any) + i * w, 0, n * w);
}

static void dump (lua_State *L, int index, array_Array *array,
                  int l, int b)
{
    array_Array *subarray;
    int i, d;

    index = absolute (L, index);
    subarray = testarray(L, index);

    if (subarray) {
        if (subarray->type != array->type) {
            lua_pushfstring (L,
                             "Inconsistent array structure (subarray "
                             "at depth %d has unsuitable type).", l);
            lua_error (L);
        }

        if (subarray->rank != array->rank - l) {
            lua_pushfstring (L,
                             "Inconsistent array structure (subarray "
                             "at depth %d should have rank %d but "
                             "has %d).",
                             l, array->rank - l, subarray->rank);
            lua_error (L);
        }

        for (i = 0, d = 1 ; i < array->rank - l ; d *= subarray->size[i], i += 1) {
            if (subarray->size[i] != array->size[l + i]) {
                lua_pushfstring (L,
                                 "Inconsistent array structure (subarray "
                                 "at depth %d should have size %d along "
                                 "dimension %d but has %d).",
                                 l, array->size[l + i], i + 1, subarray->size[i]);
                lua_error (L);
            }
        }

        copy_elements(array, subarray, b, 0, d);
    } else {
        if (lua_rawlen (L, index) != array->size[l]) {
            lua_pushfstring (L,
                             "Inconsistent array structure (subarray "
                             "at depth %d should have %d elements but "
                             "has %d).",
                             l, array->size[l], lua_rawlen (L, -1));
            lua_error (L);
        }

        for (i = l + 1, d = 1;
             i < array->rank;
             d *= array->size[i], i += 1);

        for (i = 0 ; i < array->size[l] ; i += 1) {
            lua_rawgeti (L, index, i + 1);

            if (lua_type (L, -1) == LUA_TNUMBER) {
                write_element(array, b + i, lua_tonumber (L, -1));
            } else {
                dump (L, -1, array, l + 1, b + i * d);
            }

            lua_pop (L, 1);
        }
    }
}

static array_Array *construct (lua_State *L, array_Array *array)
{
    array_Array *new;

    assert(array->rank > 0);
    assert(array->length >= 0);
    assert(array->size);
    assert(array->values.any);;

    new = (array_Array *)lua_newuserdata(L, sizeof(array_Array));
    *new = *array;

    if (metatable == LUA_REFNIL) {
        lua_newtable (L);

        lua_pushstring(L, "__ipairs");
        lua_pushcfunction(L, (lua_CFunction)__ipairs);
        lua_settable(L, -3);
        lua_pushstring(L, "__pairs");
        lua_pushcfunction(L, (lua_CFunction)__ipairs);
        lua_settable(L, -3);
        lua_pushstring(L, "__len");
        lua_pushcfunction(L, (lua_CFunction)__len);
        lua_settable(L, -3);
        lua_pushstring(L, "__index");
        lua_pushcfunction(L, (lua_CFunction)__index);
        lua_settable(L, -3);
        lua_pushstring(L, "__newindex");
        lua_pushcfunction(L, (lua_CFunction)__newindex);
        lua_settable(L, -3);
        lua_pushstring(L, "__tostring");
        lua_pushcfunction(L, (lua_CFunction)__tostring);
        lua_settable(L, -3);
        lua_pushstring(L, "__gc");
        lua_pushcfunction(L, (lua_CFunction)__gc);
        lua_settable(L, -3);
        lua_pushstring(L, "__array");
        lua_pushboolean(L, 1);
        lua_settable(L, -3);

        signature = lua_topointer (L, -1);
        metatable = luaL_ref (L, LUA_REGISTRYINDEX);
    }

    lua_rawgeti (L, LUA_REGISTRYINDEX, metatable);
    lua_setmetatable(L, -2);

    return new;
}

static int __gc (lua_State *L)
{
    array_Array *array;

    array = lua_touserdata(L, 1);

    if (array->free == FREE_SIZE || array->free == FREE_BOTH) {
        free(array->size);
    }

    if (array->free == FREE_VALUES || array->free == FREE_BOTH) {
        free(array->values.any);
    }

    return 0;
}

static int __tostring (lua_State *L)
{
    const char *typenames[] = {"double", "float", "unsigned int", "int",
                               "unsigned short", "short",
                               "unsigned char", "char"};
    array_Array *array;
    int i;

    array = lua_touserdata(L, 1);

    lua_pushfstring (L, "<%s%s array[%d",
                     array->type < 0 ? "normalized " : "",
                     typenames[abs(array->type) - 1], array->size[0]);

    for (i = 1 ; i < array->rank ; i += 1) {
        lua_pushfstring (L, ", %d", array->size[i]);
    }

    lua_pushstring (L, "]>");

    lua_concat (L, i + 1);

    return 1;
}

static int __len (lua_State *L)
{
    array_Array *array;

    array = lua_touserdata(L, 1);

    lua_pushinteger (L, array->size[0]);

    return 1;
}

static int __next (lua_State *L)
{
    array_Array *array;
    int k;

    array = lua_touserdata(L, 1);
    k = lua_tointeger (L, 2);

    lua_pop (L, 1);

    if (k < array->size[0]) {
        lua_pushinteger (L, k + 1);
        lua_pushinteger (L, k + 1);
        lua_gettable (L, 1);

        return 2;
    } else {
        lua_pushnil (L);

        return 1;
    }
}

static int __ipairs (lua_State *L)
{
    lua_pushcfunction (L, __next);
    lua_insert (L, 1);
    lua_pushnil (L);

    return 3;
}

static int __index (lua_State *L)
{
    array_Array *array;
    int i;

    array = lua_touserdata(L, 1);
    i = lua_tointeger (L, 2);

    if (i > 0 && i <= array->size[0]) {
        if (array->rank == 1) {
            lua_pushnumber (L, read_element(array,
                                            lua_tointeger (L, 2) - 1));
        } else {
            array_Array subarray;
            int j, d;

            for (j = 1, d = 1;
                 j < array->rank;
                 d *= array->size[j], j += 1);

            subarray.type = array->type;
            subarray.free = FREE_NOTHING;
            subarray.rank = array->rank - 1;
            subarray.size = &array->size[1];
            subarray.length = array->length / array->size[0];
            subarray.values.any = reference_element(array, (i - 1) * d);

            construct (L, &subarray);

            /* Make a reference to the superarray since we're pointing
             * to its data. */

            lua_createtable(L, 1, 0);
            lua_pushvalue(L, 1);
            lua_rawseti(L, -2, 1);
            lua_setuservalue(L, -2);
        }
    } else {
        lua_pushnil (L);
    }

    return 1;
}

static int __newindex (lua_State *L)
{
    array_Array *array;
    int i;

    array = lua_touserdata(L, 1);
    i = lua_tointeger (L, 2);

    if (i > 0 && i <= array->size[0]) {
        if (array->rank == 1) {
            write_element (array,
                           lua_tointeger (L, 2) - 1,
                           lua_tonumber (L, 3));
        } else if (lua_istable (L, 3)) {
            array_Array subarray;
            int j, r, d;

            for (r = 0;
                 lua_type (L, -1) == LUA_TTABLE;
                 r += 1, lua_rawgeti (L, -1, 1)) {
                if (array->size[r + 1] != lua_rawlen (L, -1)) {
                    lua_pushstring (L, "Array sizes don't match.");
                    lua_error (L);
                }
            }

            if (r != array->rank - 1) {
                lua_pushstring (L, "Array dimensions don't match.");
                lua_error (L);
            }

            lua_settop (L, 3);

            for (j = 1, d = 1;
                 j < array->rank;
                 d *= array->size[j], j += 1);

            subarray.type = array->type;
            subarray.free = FREE_NOTHING;
            subarray.rank = array->rank - 1;
            subarray.size = &array->size[1];
            subarray.values.any = reference_element(array, (i - 1) * d);

            dump (L, -1, &subarray, 0, 0);
        } else {
            array_Array *subarray;
            int j, d;

            subarray = lua_touserdata(L, 3);

            for (j = 1, d = 1;
                 j < array->rank;
                 d *= array->size[j], j += 1);

            if (subarray->rank != array->rank - 1) {
                lua_pushstring (L, "Array dimensions don't match.");
                lua_error (L);
            }

            if (subarray->type != array->type) {
                lua_pushstring (L, "Array types don't match.");
                lua_error (L);
            }

            for (j = 1 ; j < array->rank ; j += 1) {
                if (subarray->size[j - 1] != array->size[j]) {
                    lua_pushstring (L, "Array sizes don't match.");
                    lua_error (L);
                }
            }

            copy_elements(array, subarray, (i - 1) * d, 0, d);
        }
    } else {
        lua_pushstring (L, "Index out of array bounds.");
        lua_error (L);
    }

    return 0;
}

static array_Array *fromtable (lua_State *L, int index, array_Array *array)
{
    array_Array *subarray;
    int i, l, r, h;

    luaL_checktype (L, index, LUA_TTABLE);

    h = lua_gettop (L);

    lua_pushvalue (L, index);

    for (r = 0, subarray = NULL;
         lua_type (L, -1) == LUA_TTABLE || (subarray = testarray(L, -1));
         lua_rawgeti (L, -1, 1)) {
        if (subarray) {
            /* If we've hit a subarray we can happily skip parsing the
             * rest of the table structure. */

            r += subarray->rank;
            break;
        } else {
            r += 1;
        }
    }

    lua_settop (L, h);

    if (array->rank == 0) {
        array->rank = r;
        array->size = calloc (r, sizeof(int));
    } else if (r != array->rank) {
        lua_pushstring (L, "Initialization from table of incompatible rank.");
        lua_error (L);
    }

    lua_pushvalue (L, index);

    for (i = 0, l = 1 ; i < r ; i += 1) {
        int j, l_0;

        subarray = testarray(L, -1);

        if (subarray) {
            /* If we've hit a subarray we can happily skip parsing the
             * rest of the table structure. */

            l *= subarray->length / sizeof_element (subarray->type);

            for (j = i ; j < r ; j += 1) {
                if (array->size[j] == 0) {
                    array->size[j] = subarray->size[j - i];
                } else if (subarray->size[j - i] != array->size[j]) {
                    lua_pushstring (L, "Initialization from table of incompatible size.");
                    lua_error (L);
                }
            }

            break;
        } else {
            l_0 = lua_rawlen (L, -1);
            l *= l_0;

            if (array->size[i] == 0) {
                array->size[i] = l_0;
            } else if (l_0 != array->size[i]) {
                lua_pushstring (L, "Initialization from table of incompatible size.");
                lua_error (L);
            }

            lua_rawgeti (L, -1, 1);
        }
    }

    lua_settop (L, h);

    array->free = FREE_BOTH;
    array->length = l * sizeof_element (array->type);
    array->values.any = malloc (array->length);

    dump (L, index, array, 0, 0);

    return construct (L, array);
}

static array_Array *fromstring (lua_State *L, int index, array_Array *array)
{
    int i, l;

    for (i = 0, l = 1 ; i < array->rank ; l *= array->size[i], i += 1);

    l *= sizeof_element (array->type);
    array->free = FREE_SIZE;
    array->length = lua_rawlen (L, index);
    array->values.any = (void *)lua_tostring (L, index);

    if (l != array->length) {
        lua_pushfstring (L, "Invalid array data length (should be %d bytes but is %d bytes).", l, lua_rawlen (L, index));
        lua_error (L);
    }

    return construct (L, array);
}

static array_Array *fromuserdata (lua_State *L, int index, array_Array *array)
{
    array->free = FREE_SIZE;
    array->length = lua_rawlen(L, index);
    array->values.any = (void *)lua_touserdata (L, index);

    return construct (L, array);
}

#define CAST1(A, B, FACTOR, N)                                          \
    {                                                                   \
        int i;                                                          \
                                                                        \
        if (FACTOR != 1) {                                              \
            for (i = 0 ; i < N ; i += 1) {                              \
                A[i] = FACTOR * B[i];                                   \
            }                                                           \
        } else {                                                        \
            for (i = 0 ; i < N ; i += 1) {                              \
                A[i] = B[i];                                            \
            }                                                           \
        }                                                               \
    }

#define CAST2(A, B, FACTORA, N)                                         \
    switch (B->type) {                                                  \
    case ARRAY_TDOUBLE:                                                 \
        CAST1(A, B->values.doubles, FACTORA, N);                        \
        break;                                                          \
    case ARRAY_TFLOAT:                                                  \
        CAST1(A, B->values.floats, FACTORA, N);                         \
        break;                                                          \
    case ARRAY_TUINT:                                                   \
        CAST1(A, B->values.uints, FACTORA, N);                          \
        break;                                                          \
    case ARRAY_TNUINT:                                                  \
        CAST1(A, B->values.uints, FACTORA / (double)UINT_MAX, N);       \
        break;                                                          \
    case ARRAY_TINT:                                                    \
        CAST1(A, B->values.ints, FACTORA, N);                           \
        break;                                                          \
    case ARRAY_TNINT:                                                   \
        CAST1(A, B->values.ints, FACTORA / (double)INT_MAX, N);         \
        break;                                                          \
    case ARRAY_TUSHORT:                                                 \
        CAST1(A, B->values.ushorts, FACTORA, N);                        \
        break;                                                          \
    case ARRAY_TNUSHORT:                                                \
        CAST1(A, B->values.ushorts, FACTORA / (double)USHRT_MAX, N);    \
        break;                                                          \
    case ARRAY_TSHORT:                                                  \
        CAST1(A, B->values.shorts, FACTORA, N);                         \
        break;                                                          \
    case ARRAY_TNSHORT:                                                 \
        CAST1(A, B->values.shorts, FACTORA / (double)SHRT_MAX, N);      \
        break;                                                          \
    case ARRAY_TUCHAR:                                                  \
        CAST1(A, B->values.uchars, FACTORA, N);                         \
        break;                                                          \
    case ARRAY_TNUCHAR:                                                 \
        CAST1(A, B->values.uchars, FACTORA / (double)UCHAR_MAX, N);     \
        break;                                                          \
    case ARRAY_TCHAR:                                                   \
        CAST1(A, B->values.chars, FACTORA, N);                          \
        break;                                                          \
    case ARRAY_TNCHAR:                                                  \
        CAST1(A, B->values.chars, FACTORA / (double)CHAR_MAX, N);       \
        break;                                                          \
    }

static array_Array *fromarray (lua_State *L, int index, array_Array *array)
{
    array_Array *B;
    int i, n;

    B = lua_touserdata (L, index);

    if (array->rank == 0) {
        array->rank = B->rank;
        array->size = malloc (B->rank * sizeof(int));
        memcpy(array->size, B->size, B->rank * sizeof(int));
    } else if (B->rank != array->rank) {
        lua_pushstring (L, "Initialization from table of incompatible rank.");
        lua_error (L);
    }

    for (i = 0, n = 1; i < B->rank ; n *= B->size[i], i += 1) {
        if (array->size[i] != B->size[i]) {
            lua_pushstring (L,
                            "Initialization from table of incompatible size.");
            lua_error (L);
        }
    }

    array->free = FREE_BOTH;
    array->length = n * sizeof_element (array->type);
    array->values.any = malloc (array->length);

    switch (array->type) {
    case ARRAY_TDOUBLE:
        CAST2(array->values.doubles, B, 1, n);
        break;
    case ARRAY_TFLOAT:
        CAST2(array->values.floats, B, 1, n);
        break;
    case ARRAY_TUINT:
        CAST2(array->values.uints, B, 1, n);
        break;
    case ARRAY_TNUINT:
        CAST2(array->values.uints, B, UINT_MAX, n);
        break;
    case ARRAY_TINT:
        CAST2(array->values.ints, B, 1, n);
        break;
    case ARRAY_TNINT:
        CAST2(array->values.ints, B, INT_MAX, n);
        break;
    case ARRAY_TUSHORT:
        CAST2(array->values.ushorts, B, 1, n);
        break;
    case ARRAY_TNUSHORT:
        CAST2(array->values.ushorts, B, USHRT_MAX, n);
        break;
    case ARRAY_TSHORT:
        CAST2(array->values.shorts, B, 1, n);
        break;
    case ARRAY_TNSHORT:
        CAST2(array->values.shorts, B, SHRT_MAX, n);
        break;
    case ARRAY_TUCHAR:
        CAST2(array->values.uchars, B, 1, n);
        break;
    case ARRAY_TNUCHAR:
        CAST2(array->values.uchars, B, UCHAR_MAX, n);
        break;
    case ARRAY_TCHAR:
        CAST2(array->values.chars, B, 1, n);
        break;
    case ARRAY_TNCHAR:
        CAST2(array->values.chars, B, CHAR_MAX, n);
        break;
    }

    return construct (L, array);
}

static array_Array *fromnothing (lua_State *L, array_Array *array)
{
    int i, l;

    for (i = 0, l = 1 ; i < array->rank ; l *= array->size[i], i += 1);

    array->free = FREE_BOTH;
    array->length = l * sizeof_element (array->type);
    array->values.any = malloc (array->length);

    return construct (L, array);
}

static int typeerror (lua_State *L, int narg, const char *tname) {
  const char *msg = lua_pushfstring(L, "%s expected, got %s",
                                    tname, luaL_typename(L, narg));
  return luaL_argerror(L, narg, msg);
}

static array_Array *testarray (lua_State *L, int index)
{
    index = absolute (L, index);

    if (!lua_type (L, index) == LUA_TUSERDATA || !lua_getmetatable (L, index)) {
        return NULL;
    } else {
        if (lua_topointer (L, -1) == signature) {
            lua_pop (L, 1);
            return lua_touserdata (L, index);
        } else if (lua_getfield (L, -1, "__array"),
                   lua_toboolean (L, -1)) {
            lua_pop (L, 2);
            return lua_touserdata (L, index);
        } else {
            lua_pop (L, 1);
            return NULL;
        }
    }
}

int array_isarray(lua_State *L, int index)
{
    return testarray(L, index) != NULL;
}

array_Array *array_testarray (lua_State *L, int index)
{
    index = absolute (L, index);

    if (lua_type (L, index) == LUA_TTABLE) {
        array_toarray (L, index, ARRAY_TDOUBLE, 0);
        lua_replace (L, index);
    }

    return testarray (L, index);
}

array_Array *array_checkarray (lua_State *L, int index)
{
    array_Array *array;

    index = absolute (L, index);

    if (!(array = array_testarray (L, index))) {
        typeerror(L, index, "array");
    }

    return array;
}

array_Array *array_checkcompatible (lua_State *L, int index, int what, ...)
{
    va_list ap;
    array_Array *array;
    array_Type type;
    int rank;

    index = absolute (L, index);

    va_start (ap, what);

    if (what & ARRAY_TYPE) {
        type = va_arg (ap, array_Type);
    } else {
        type = ARRAY_TDOUBLE;
    }

    if (lua_type (L, index) == LUA_TTABLE) {
        array_toarray (L, index, type, 0);
        lua_replace (L, index);
    }

    array = array_checkarray (L, index);

    if (what & ARRAY_TYPE) {
        if (array->type != type) {
            luaL_argerror (L, index, "expected array of different type");
        }
    }

    if (what & ARRAY_RANK) {
        rank = va_arg (ap, int);

        if (array->rank != rank) {
            luaL_argerror (L, index,
                           "expected array of different dimensionality");
        }
    }

    if (what & ARRAY_SIZE) {
        int i, l;

        assert (what & ARRAY_RANK);

        for (i = 0 ; i < rank ; i += 1) {
            l = va_arg (ap, int);

            if (l > 0 && array->size[i] != l) {
                luaL_argerror (L, index,
                               "expected array of different size");
            }
        }
    }

    va_end (ap);

    return array;
}

array_Array *array_testcompatible (lua_State *L, int index, int what, ...)
{
    va_list ap;
    array_Array *array;
    array_Type type;
    int rank;

    index = absolute (L, index);

    va_start (ap, what);

    if (what & ARRAY_TYPE){
        type = va_arg (ap, array_Type);
    } else {
        type = ARRAY_TDOUBLE;
    }

    if (lua_type (L, index) == LUA_TTABLE) {
        array_toarray (L, index, type, 0);
        lua_replace (L, index);
    }

    array = array_testarray (L, index);

    if (!array) {
        return NULL;
    }

    if (what & ARRAY_TYPE){
        if (array->type != type) {
            return NULL;
        }
    }

    if (what & ARRAY_RANK) {
        rank = va_arg (ap, int);

        if (array->rank != rank) {
            return NULL;
        }
    }

    if (what & ARRAY_SIZE) {
        int i;

        assert (what & ARRAY_RANK);

        for (i = 0 ; i < rank ; i += 1) {
            if (array->size[i] != va_arg (ap, int)) {
                return NULL;
            }
        }
    }

    va_end (ap);

    return array;
}

array_Array *array_pusharray (lua_State *L, array_Array *array)
{
    return construct (L, array);
}

void array_initializev (array_Array *array, array_Type type,
                        void *values, int rank, int *size)
{
    int j, l;

    array->type = type;
    array->rank = rank;
    array->size = malloc (rank * sizeof(int));

    for (j = 0, l = 1 ; j < rank ; j += 1) {
        array->size[j] = size[j];
        l *= array->size[j];
    }

    array->free = FREE_BOTH;
    array->length = l * sizeof_element (type);
    array->values.any = malloc (array->length);

    if (values) {
        copy_values (array, values, 0, 0, l);
    }
}

void array_initialize (array_Array *array, array_Type type, void *values,
                       int rank, ...)
{
    va_list ap;
    int j, size[rank];

    va_start (ap, rank);

    for (j = 0 ; j < rank ; j += 1) {
        size[j] = va_arg(ap, int);
    }

    va_end(ap);

    array_initializev (array, type, values, rank, size);
}

array_Array *array_createarrayv (lua_State *L, array_Type type,
                                 void *values, int rank, int *size)
{
    array_Array array;

    array_initializev (&array, type, values, rank, size);
    return construct (L, &array);
}

array_Array *array_createarray (lua_State *L, array_Type type,
                                void *values, int rank, ...)
{
    va_list ap;
    int j, size[rank];

    va_start (ap, rank);

    for (j = 0 ; j < rank ; j += 1) {
        size[j] = va_arg(ap, int);
    }

    va_end(ap);

    return array_createarrayv (L, type, values, rank, size);
}

array_Array *array_toarrayv (lua_State *L, int index, array_Type type,
                             int rank, int *size)
{
    array_Array array;
    int j;

    index = absolute (L, index);

    if (lua_type (L, index) != LUA_TNIL &&
        lua_type (L, index) != LUA_TSTRING &&
        lua_type (L, index) != LUA_TUSERDATA &&
        lua_type (L, index) != LUA_TLIGHTUSERDATA &&
        lua_type (L, index) != LUA_TTABLE) {
        lua_pushfstring (L,
                         "Initialization from incompatible value "
                         "(expected string or table, got %s).",
                         lua_typename (L, lua_type (L, index)));
        lua_error (L);
    }

    array.type = type;

    if (rank < 1) {
        if (lua_type(L, index) == LUA_TSTRING) {
            array.rank = 1;

            array.size = malloc (sizeof (int));
            array.size[0] = lua_rawlen(L, index) / sizeof_element (array.type);
        } else if (lua_type(L, index) == LUA_TTABLE || testarray(L, index)) {
            array.rank = 0;
            array.size = NULL;
        } else {
            lua_pushstring (L, "Array dimensions undefined.");
            lua_error (L);
        }
    } else {
        array.rank = rank;
        array.size = malloc (rank * sizeof (int));

        for (j = 0 ; j < rank ; j += 1) {
            array.size[j] = size[j];
        }
    }

    if (lua_type(L, index) == LUA_TSTRING) {
        array_Array *result;

        result = fromstring (L, index, &array);

        /* Make a reference to the string since we're pointing
         * into its memory. */

        lua_createtable(L, 1, 0);
        lua_pushvalue(L, index);
        lua_rawseti(L, -2, 1);
        lua_setuservalue(L, -2);

        return result;
    } else if (lua_type(L, index) == LUA_TTABLE) {
        return fromtable (L, index, &array);
    } else if (testarray(L, index)) {
        return fromarray(L, index, &array);
    } else if (lua_type(L, index) == LUA_TUSERDATA) {
        return fromuserdata (L, index, &array);

        /* Make a reference to the userdata since we're pointing
         * into its memory. */

        lua_createtable(L, 1, 0);
        lua_pushvalue(L, index);
        lua_rawseti(L, -2, 1);
        lua_setuservalue(L, -2);
    } else {
        return fromnothing (L, &array);
    }
}

array_Array *array_toarray (lua_State *L, int index, array_Type type, int rank, ...)
{
    va_list ap;
    int j, size[rank];

    va_start (ap, rank);

    for (j = 0 ; j < rank ; j += 1) {
        size[j] = va_arg(ap, int);
    }

    va_end(ap);

    return array_toarrayv (L, index, type, rank, size);
}

array_Array *array_reshapev (lua_State *L, int index, int rank, int *size)
{
    array_Array reshaped, *array;
    int j, l, m;

    array = array_testarray (L, index);
    reshaped.size = malloc (rank * sizeof (int));

    for (j = 0, l = 1 ; j < rank ; j += 1) {
        reshaped.size[j] = size[j];
        l *= size[j];
    }

    for (j = 0, m = 1 ; j < array->rank ; j += 1) {
        m *= array->size[j];
    }

    if (l != m) {
        lua_pushstring (L, "Incompatible reshape dimensions.");
        lua_error (L);
    }

    reshaped.type = array->type;
    reshaped.length = array->length;
    reshaped.rank = rank;
    reshaped.values.any = array->values.any;
    reshaped.free = FREE_SIZE;

    /* Make a reference to the original array since we're pointing
     * into its memory. */

    lua_createtable(L, 1, 0);
    lua_pushvalue(L, index);
    lua_rawseti(L, -2, 1);
    lua_setuservalue(L, -2);

    return construct (L, &reshaped);
}

array_Array *array_reshape (lua_State *L, int index, int rank, ...)
{
    va_list ap;
    int j, size[rank];

    va_start (ap, rank);

    for (j = 0 ; j < rank ; j += 1) {
        size[j] = va_arg(ap, int);
    }

    va_end(ap);

    return array_reshapev (L, index, rank, size);
}

array_Array *array_copy (lua_State *L, int index)
{
    array_Array *array, copy;
    int i, d;

    array = array_testarray(L, index);

    copy.type = array->type;
    copy.length = array->length;
    copy.rank = array->rank;
    copy.free = FREE_BOTH;

    copy.size = malloc (array->rank * sizeof (int));
    memcpy(copy.size, array->size,array->rank * sizeof (int));

    for (i = 0, d = 1;
         i < array->rank;
         d *= array->size[i], i += 1);

    copy.values.any = malloc (copy.length);
    copy_elements(&copy, array, 0, 0, d);

    return construct (L, &copy);
}

array_Array *array_set (lua_State *L, int index, lua_Number c)
{
    array_Array *array;
    int i, d;

    array = array_testarray(L, index);

    for (i = 0, d = 1;
         i < array->rank;
         d *= array->size[i], i += 1);

    for (i = 0 ; i < d ; i += 1) {
        write_element (array, i, c);
    }

    return array;
}

static void adjust(array_Array *source, array_Array *sink, void *defaults,
                   int offset_s, int stride_s, int offset_k, int stride_k,
                   int level, int use_defaults)
{
    int j;

    if (level <= sink->rank - 1) {
        stride_s /= source->size[level];
        stride_k /= sink->size[level];

        for (j = 0 ; j < sink->size[level] ; j += 1) {
            adjust (source, sink, defaults,
                    offset_s + j * stride_s, stride_s,
                    offset_k + j * stride_k, stride_k,
                    level + 1, use_defaults || j >= source->size[level]);
        }
    } else {
        assert(level <= source->rank);
        assert(offset_k + stride_k <= sink->length);
        assert(stride_k == stride_s);

        if (use_defaults) {
            if (!defaults) {
                zero_elements (sink, offset_k, stride_k);
            } else {
                copy_values(sink, defaults, offset_k, offset_k, stride_k);
            }
        } else {
            copy_elements(sink, source, offset_k, offset_s, stride_k);
        }
    }
}

array_Array *array_adjustv (lua_State *L, int index, void *defaults, int rank, int *size)
{
    array_Array sink, *source, *array;
    int j, l, m;

    index = absolute (L, index);
    source = array_testcompatible(L, index, ARRAY_RANK, rank);

    if (!source) {
        return NULL;
    }

    /* If no adjustment is required return the array as-is. */

    if (!memcmp(source->size, size, rank * sizeof(int))) {
        return source;
    }

    /* Initialize the adjusted array. */

    sink.type = source->type;
    sink.rank = rank;
    sink.size = malloc (rank * sizeof(int));

    for (j = 0, l = 1 ; j < rank ; j += 1) {
        sink.size[j] = size[j];
        l *= sink.size[j];
    }

    sink.free = FREE_BOTH;
    sink.length = l * sizeof_element(sink.type);
    sink.values.any = malloc (sink.length);

    for (j = 0, m = 1;
         j < source->rank;
         m *= source->size[j], j += 1);

    adjust (source, &sink, defaults, 0, m, 0, l, 0, 0);

    array = construct (L, &sink);
    lua_replace (L, index);

    return array;
}

array_Array *array_adjust (lua_State *L, int index, void *defaults, int rank, ...)
{
    va_list ap;
    int j, size[rank];

    va_start (ap, rank);

    for (j = 0 ; j < rank ; j += 1) {
        size[j] = va_arg(ap, int);
    }

    va_end(ap);

    return array_adjustv (L, index, defaults, rank, size);
}

static void cut (array_Array *source, array_Array *sink, int from, int to,
                 int l, int m, int d, int *counts, int *ranges)
{
    int i, j, k;

    if (d < source->rank - 1) {
       int s, t;

       s = l / source->size[d];
       t = m / sink->size[d];

       for (j = 0, k = 0 ; j < counts[0] ; j += 1) {
           int *range;

           range = &ranges[2 * j];

           for (i = 0 ; i < range[1] - range[0] + 1 ; i += 1, k += 1) {
               cut (source, sink,
                    from + (range[0] - 1 + i) * s,
                    to + k * t,
                    s, t, d + 1, &counts[1], &ranges[2 * counts[0]]);
           }
       }
    } else {
        for (j = 0, k = 0 ; j < counts[0] ; j += 1) {
           int *range;

           range = &ranges[2 * j];

           copy_elements (sink, source, to + k, from + (range[0] - 1),
                          (range[1] - range[0] + 1));
           k += (range[1] - range[0] + 1);
       }
    }
}

array_Array *array_slicev (lua_State *L, int index, int *slices)
{
    array_Array *array, slice;
    int i, n;

    array = array_testarray (L, index);

    for (i = 0, n = 0;
         i < array->rank;
         n += slices[i], i += 1);

    {
        int j, k, l, m, p, wrapped[array->rank + 2 * n];

        slice.size = malloc (array->rank * sizeof (int));
        memcpy (slice.size, array->size, array->rank * sizeof (int));
        memcpy (wrapped, slices, (array->rank + 2 * n) * sizeof (int));

        /* Wrap around negative indices and check slice ranges against
         * source array bounds.  Also calculate sizes for the sliced
         * array. */

        for (k = 0, p = array->rank, l = m = 1 ; k < array->rank ; k += 1) {
            slice.size[k] = 0;

            for (j = 0 ; j < wrapped[k] ; j += 1, p += 2) {
                int *b;

                b = &wrapped[p];

                for (i = 0 ; i < 2 ; i += 1) {
                    if (b[i] < 0) {
                        b[i] += array->size[k] + 1;
                    }

                    if (b[i] < 1 || b[i] > array->size[k]) {
                        lua_pushfstring (L,
                                         "Slice range %d along dimension %d "
                                         "is invalid (%d is out of bounds).",
                                         j + 1, k + 1, b[i]);

                        lua_error (L);
                    }
                }

                /* Check slice range ordering. */

                if (b[1] < b[0]) {
                    lua_pushfstring (L,
                                     "Invalid slice range specified (%d < %d).",
                                     b[1], b[0]);

                    lua_error (L);
                }

                slice.size[k] += b[1] - b[0] + 1;
            }

            l *= array->size[k];
            m *= slice.size[k];
        }

        slice.length = m * sizeof_element (array->type);
        slice.values.any = malloc (slice.length);
        slice.type = array->type;
        slice.rank = array->rank;
        slice.free = FREE_BOTH;

        cut (array, &slice, 0, 0, l, m, 0, &wrapped[0], &wrapped[array->rank]);
    }

    return construct (L, &slice);
}

array_Array *array_slice (lua_State *L, int index, ...)
{
    array_Array *array, *sliced;
    va_list ap;

    array = array_testarray (L, index);

    {
        int i, n, counts[array->rank];

        va_start (ap, index);

        for (i = 0, n = 1;
             i < array->rank;
             counts[i] = va_arg(ap, int), n += counts[i], i += 1);

        {
            int j, slices[array->rank + 2 * n];

            for (j = 0 ; j < n ; j += 1) {
                slices[2 * j] = va_arg(ap, int);
                slices[2 * j + 1] = va_arg(ap, int);
            }

            sliced = array_slicev (L, index, slices);
        }

        va_end(ap);
    }

    return sliced;
}

array_Array *array_transposev (lua_State *L, int index, int *indices)
{
    array_Array *array, transposed;
    int i, n;

    array = array_testarray (L, index);

    transposed.free = FREE_BOTH;
    transposed.length = array->length;
    transposed.type = array->type;
    transposed.rank = array->rank;
    transposed.values.any = malloc (transposed.length);
    assert(transposed.values.any);
    transposed.size = calloc (array->rank, sizeof (int));

    for (i = 0 ; i < array->rank ; i += 1) {
        int j;

        j = indices[i];

        if (j < 0 || j >= transposed.rank) {
            lua_pushfstring (L,
                             "Index %d is out of range for given array.",
                             j + 1);
            lua_error (L);
        }

        transposed.size[i] = array->size[j];
    }

    for (i = 0 ; i < transposed.rank ; i += 1) {
        if (transposed.size[i] == 0) {
            lua_pushstring (L,
                            "Specified indices do not form a permutation "
                            "of the array's indices.");
            lua_error (L);
        }
    }

    n = transposed.length / sizeof_element (transposed.type);

    for (i = 0 ; i < n ; i += 1) {
        int i_a, j, k, l, permuted[transposed.rank];

        /* The index i traverses the transposed matrix.  Break it up
         * into separate indices along each dimension and permute them
         * according to the map. */

        for (j = array->rank - 1, k = i;
             j >= 0;
             k /= transposed.size[j], j -= 1) {
            permuted[indices[j]] = k % transposed.size[j];
        }

        /* Calculate an index into the original array.  */

        for (j = array->rank - 1, i_a = 0, l = 1;
             j >= 0;
             i_a += permuted[j] * l, l *= array->size[j], j -= 1);

        assert(i_a >= 0 && i_a < n);
        copy_elements(&transposed, array, i, i_a, 1);
    }

    return construct (L, &transposed);
}

array_Array *array_transpose (lua_State *L, int index, ...)
{
    array_Array *array, *transposed;

    array = array_testarray (L, index);

    {
        va_list ap;
        int j, indices[array->rank];

        va_start (ap, index);

        for (j = 0 ; j < array->rank ; j += 1) {
            indices[j] = va_arg(ap, int);
        }

        va_end(ap);

        transposed = array_transposev (L, index, indices);
    }

    return transposed;
}
