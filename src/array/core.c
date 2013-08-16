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

static int create (lua_State *L)
{
    int n, j;

    for (n = 0 ; lua_type (L, n + 1) == LUA_TNUMBER ; n += 1);

    if (lua_gettop (L) == n) {
	lua_pushnil(L);
    }

    {
	int size[n];
	array_Type type;

	for (j = 0 ; j < n ; j += 1) {
	    size[j] = lua_tonumber (L, j + 1);
	}

	type = lua_tointeger (L, lua_upvalueindex(1));

	if (lua_toboolean (L, n + 2)) {
	    if (type == ARRAY_TFLOAT || type == ARRAY_TDOUBLE) {
		lua_pushstring (L,
				"Only integer arrays can be normalized.");
		lua_error (L);
	    }
		
	    type = -type;
	}
	
	array_toarrayv (L, n + 1, type, n, size);
    }		 
    
    return 1;
}

static int cast (lua_State *L)
{
    array_checkarray (L, 1);
    array_cast(L, lua_tointeger (L, lua_upvalueindex(1)));
    
    return 1;
}

static int reshape (lua_State *L)
{
    int n, j;

    for (n = 0 ; lua_type (L, n + 1) == LUA_TNUMBER ; n += 1);

    if (n < 1) {
	lua_pushstring (L, "Array dimensions undefined.");
	lua_error (L);
    }

    array_checkarray (L, n + 1);

    {
	int size[n];

	for (j = 0 ; j < n ; j += 1) {
	    size[j] = lua_tonumber (L, j + 1);
	}
	
	array_reshapev (L, n + 1, n, size);
    }		 
    
    return 1;
}

static int copy (lua_State *L)
{
    array_checkarray (L, 1);

    array_copy (L, 1);
    return 1;
}

static int dump (lua_State *L)
{
    array_Array *array;

    array = array_checkarray (L, 1);
    lua_pushlstring (L, array->values.any, array->length);
    
    return 1;
}

static int adjust (lua_State *L)
{
    array_Array *defaults;
    int n, j;

    for (n = 0 ; lua_type (L, n + 1) == LUA_TNUMBER ; n += 1);

    array_checkcompatible (L, n + 1, ARRAY_RANK, n);
    defaults = array_testarray (L, n + 2);

    {
	int size[n];

	for (j = 0 ; j < n ; j += 1) {
	    size[j] = lua_tonumber (L, j + 1);
	}
	
        if (defaults) {
            if (defaults->rank != n) {
                luaL_argerror (L, n + 2,
                               "default array has incompatible rank");
            }
        
            for (j = 0 ; j < n ; j += 1) {
                if (defaults->size[j] != size[j]) {
                    luaL_argerror (L, n + 2,
                                   "default array has incompatible "
                                   "dimensionality");
                }
            }
        }
        
	array_adjustv (L, n + 1, defaults ? defaults->values.any : NULL, n, size);
    }

    if (defaults) {
        lua_pop (L, 1);             /* Pop the defaults array. */
    }
    
    return 1;
}

static int slice (lua_State *L)
{
    array_Array *array;
    int j, n;

    array = array_checkarray (L, 1);

    for (j = 0, n = 0 ; j < array->rank ; j += 1) {
        /* If no slices are specified along a dimension assume one
         * range with all elements. */
        
        if (lua_isnoneornil(L, 2 + j)) {
            n += 1;
        } else {
            luaL_checktype(L, 2 + j, LUA_TTABLE);    
            n += lua_rawlen(L, 2 + j);
        }
    }

    {
        int k, l, slices[array->rank + 2 * n];

        /* Iterate through each dimension and gather all slices. */
        
        for (j = 0, l = array->rank ; j < array->rank ; j += 1) {
            if (lua_isnoneornil(L, 2 + j)) {
                slices[j] = 1;
                slices[l] = 1;
                slices[l + 1] = array->size[j];
                l += 2;
            } else {
                slices[j] = lua_rawlen(L, 2 + j);

                for (k = 0 ; k < slices[j] ; k += 1, l += 2) {
                    lua_rawgeti(L, 2 + j, k + 1);

                    if (lua_isnumber(L, -1)) {
                        slices[l] = slices[l + 1] = lua_tointeger (L, -1);
                    } else if (lua_istable(L, -1)) {
                        lua_rawgeti(L, -1, 1);
                        slices[l] = lua_tointeger (L, -1);
                        lua_pop(L, 1);

                        lua_rawgeti(L, -1, 2);
                        slices[l + 1] = lua_tointeger (L, -1);
                        lua_pop(L, 1);
                    } else {
                        lua_pushstring (L,
                                        "Slice elements must be integers or"
                                        "integer pairs inside tables.");
                        lua_error (L);
                    }
                    
                    lua_pop(L, 1);
                }
            }
        }
       
        array_slicev (L, 1, slices);
    }           
    
    return 1;
}

static int transpose (lua_State *L)
{
    array_Array *array;
    int j;

    array = array_checkarray (L, 1);

    {
        int indices[array->rank];

        for (j = 0 ; j < array->rank ; j += 1) {
            indices[j] = luaL_checkinteger (L, j + 2) - 1;
        }
       
        array_transposev (L, 1, indices);
    }           
    
    return 1;
}


int luaopen_array_core (lua_State *L)
{
    const luaL_Reg api[] = {    
	{"copy", copy},
	{"dump", dump},
	{"reshape", reshape},
	{"adjust", adjust},        
        {"slice", slice},
        {"transpose", transpose},
	    
	{NULL, NULL}
    };


#if LUA_VERSION_NUM == 501
    luaL_register (L, "array", api);
#else
    luaL_newlib (L, api);
#endif

    /* Create an array object out of a Lua table or string. */
    
    lua_pushinteger (L, ARRAY_TDOUBLE);
    lua_pushcclosure (L, create, 1);
    lua_setfield (L, -2, "doubles");
    
    lua_pushinteger (L, ARRAY_TFLOAT);
    lua_pushcclosure (L, create, 1);
    lua_setfield (L, -2, "floats");
    
    lua_pushinteger (L, ARRAY_TULONG);
    lua_pushcclosure (L, create, 1);
    lua_setfield (L, -2, "ulongs");
    
    lua_pushinteger (L, ARRAY_TLONG);
    lua_pushcclosure (L, create, 1);
    lua_setfield (L, -2, "longs");
    
    lua_pushinteger (L, ARRAY_TUINT);
    lua_pushcclosure (L, create, 1);
    lua_setfield (L, -2, "uints");
    
    lua_pushinteger (L, ARRAY_TINT);
    lua_pushcclosure (L, create, 1);
    lua_setfield (L, -2, "ints");
    
    lua_pushinteger (L, ARRAY_TUSHORT);
    lua_pushcclosure (L, create, 1);
    lua_setfield (L, -2, "ushorts");
    
    lua_pushinteger (L, ARRAY_TSHORT);
    lua_pushcclosure (L, create, 1);
    lua_setfield (L, -2, "shorts");
    
    lua_pushinteger (L, ARRAY_TUCHAR);
    lua_pushcclosure (L, create, 1);
    lua_setfield (L, -2, "uchars");
    
    lua_pushinteger (L, ARRAY_TCHAR);
    lua_pushcclosure (L, create, 1);
    lua_setfield (L, -2, "chars");    
    
    lua_pushinteger (L, ARRAY_TNULONG);
    lua_pushcclosure (L, create, 1);
    lua_setfield (L, -2, "nulongs");
    
    lua_pushinteger (L, ARRAY_TNLONG);
    lua_pushcclosure (L, create, 1);
    lua_setfield (L, -2, "nlongs");
    
    lua_pushinteger (L, ARRAY_TNUINT);
    lua_pushcclosure (L, create, 1);
    lua_setfield (L, -2, "nuints");
    
    lua_pushinteger (L, ARRAY_TNINT);
    lua_pushcclosure (L, create, 1);
    lua_setfield (L, -2, "nints");
    
    lua_pushinteger (L, ARRAY_TNUSHORT);
    lua_pushcclosure (L, create, 1);
    lua_setfield (L, -2, "nushorts");
    
    lua_pushinteger (L, ARRAY_TNSHORT);
    lua_pushcclosure (L, create, 1);
    lua_setfield (L, -2, "nshorts");
    
    lua_pushinteger (L, ARRAY_TNUCHAR);
    lua_pushcclosure (L, create, 1);
    lua_setfield (L, -2, "nuchars");
    
    lua_pushinteger (L, ARRAY_TNCHAR);
    lua_pushcclosure (L, create, 1);
    lua_setfield (L, -2, "nchars");    

    /* Cast an array object to a different type. */
    
    lua_pushinteger (L, ARRAY_TDOUBLE);
    lua_pushcclosure (L, cast, 1);
    lua_setfield (L, -2, "todoubles");
    
    lua_pushinteger (L, ARRAY_TFLOAT);
    lua_pushcclosure (L, cast, 1);
    lua_setfield (L, -2, "tofloats");
    
    lua_pushinteger (L, ARRAY_TULONG);
    lua_pushcclosure (L, cast, 1);
    lua_setfield (L, -2, "toulongs");
    
    lua_pushinteger (L, ARRAY_TLONG);
    lua_pushcclosure (L, cast, 1);
    lua_setfield (L, -2, "tolongs");
    
    lua_pushinteger (L, ARRAY_TUINT);
    lua_pushcclosure (L, cast, 1);
    lua_setfield (L, -2, "touints");
    
    lua_pushinteger (L, ARRAY_TINT);
    lua_pushcclosure (L, cast, 1);
    lua_setfield (L, -2, "toints");
    
    lua_pushinteger (L, ARRAY_TUSHORT);
    lua_pushcclosure (L, cast, 1);
    lua_setfield (L, -2, "toushorts");
    
    lua_pushinteger (L, ARRAY_TSHORT);
    lua_pushcclosure (L, cast, 1);
    lua_setfield (L, -2, "toshorts");
    
    lua_pushinteger (L, ARRAY_TUCHAR);
    lua_pushcclosure (L, cast, 1);
    lua_setfield (L, -2, "touchars");
    
    lua_pushinteger (L, ARRAY_TCHAR);
    lua_pushcclosure (L, cast, 1);
    lua_setfield (L, -2, "tochars");    
    
    lua_pushinteger (L, ARRAY_TNULONG);
    lua_pushcclosure (L, cast, 1);
    lua_setfield (L, -2, "tonulongs");
    
    lua_pushinteger (L, ARRAY_TNLONG);
    lua_pushcclosure (L, cast, 1);
    lua_setfield (L, -2, "tonlongs");
    
    lua_pushinteger (L, ARRAY_TNUINT);
    lua_pushcclosure (L, cast, 1);
    lua_setfield (L, -2, "tonuints");
    
    lua_pushinteger (L, ARRAY_TNINT);
    lua_pushcclosure (L, cast, 1);
    lua_setfield (L, -2, "tonints");
    
    lua_pushinteger (L, ARRAY_TNUSHORT);
    lua_pushcclosure (L, cast, 1);
    lua_setfield (L, -2, "tonushorts");
    
    lua_pushinteger (L, ARRAY_TNSHORT);
    lua_pushcclosure (L, cast, 1);
    lua_setfield (L, -2, "tonshorts");
    
    lua_pushinteger (L, ARRAY_TNUCHAR);
    lua_pushcclosure (L, cast, 1);
    lua_setfield (L, -2, "tonuchars");
    
    lua_pushinteger (L, ARRAY_TNCHAR);
    lua_pushcclosure (L, cast, 1);
    lua_setfield (L, -2, "tonchars");    
    
    return 1;
}
