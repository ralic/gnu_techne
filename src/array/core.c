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
	
	array_castv (L, n + 1, n, size);
    }		 
    
    return 1;
}

static int slice (lua_State *L)
{
    array_Array *array;
    int j;

    array = array_checkarray (L, 1);

    {
	int slices[array->rank * 2];

	for (j = 0 ; j < array->rank * 2 ; j += 1) {
	    slices[j] = luaL_checknumber (L, j + 2);
	}
	
	array_slicev (L, 1, slices);
    }		 
    
    return 1;
}

static int copy (lua_State *L)
{
    array_checkarray (L, 1);

    array_copy (L, 1);
    return 1;
}

static int set (lua_State *L)
{
    double c;
    
    array_checkarray (L, 1);
    c = luaL_checknumber (L, 2);

    array_set (L, 1, c);
    lua_pop (L, 1);
    
    return 1;
}

int luaopen_array_core (lua_State *L)
{
    const luaL_Reg api[] = {
	{"add", array_add},
	{"multiply", array_multiply},
	{"subtract", array_subtract},
	{"divide", array_divide},
	{"scale", array_scale},
	{"raise", array_raise},
    
	{"copy", copy},
	{"set", set},
	{"cast", cast},
	{"slice", slice},
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

    /* Create arrays from other arrays. */
    
    return 1;
}
