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

#include <lualib.h>
#include <lauxlib.h>

static int clamp(lua_State *L)
{
    lua_Number n, a, b;
    
    n = luaL_checknumber (L, 1);
    a = luaL_checknumber (L, 2);
    b = luaL_checknumber (L, 3);

    if (n < a) {
	lua_pop (L, 1);
    } else if (n < b) {
	lua_pop (L, 2);
    }

    return 1;
}

static int sign(lua_State *L)
{
    lua_Number a;
    
    a = luaL_checknumber (L, 1);

    lua_pushnumber (L, a > 0 ? 1 : -1);
    
    return 1;
}

int luaopen_moremath (lua_State *L)
{
    luaL_Reg api[] = {
	{"clamp", clamp},
	{"sign", sign},
	{NULL, NULL}
    };

    luaL_requiref(L, "math", luaopen_math, 0);
    luaL_setfuncs(L, api, 0);

    return 1;
}

