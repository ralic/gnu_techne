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

#include "prompt/prompt.h"
#include "techne.h"

static int trace (lua_State *L)
{
    lua_Debug ar;
    int i, h;

    lua_getstack (L, 1, &ar);
    lua_getinfo (L, "Sl", &ar);
    
    h = lua_gettop (L);
    
    for (i = 1 ; i <= h ; i += 1) {
	t_print_message("%s%s:%d: %s%s\n",
			t_ansi_color (31, 1),
			ar.short_src, ar.currentline,
			luap_describe(L, i),
			t_ansi_color (0, 0));
    }

    return 0;
}

static int replacemetatable(lua_State *L)
{
    luaL_checktype (L, 1, LUA_TUSERDATA);
    luaL_checktype (L, 2, LUA_TTABLE);

    if (lua_getmetatable (L, 1)) {
	/* If the value already has a metatable, copy any fields that
	 * are not set in the supplied one. */
	
	lua_pushnil (L);
	
	while (lua_next (L, -2)) {
	    /* Check to see whether the field is overriden and if not
	     * copy it to the new metatable. */
	    
	    lua_pushvalue (L, -2);
	    lua_gettable (L, 2);

	    if (lua_isnil (L, -1)) {
		
		lua_pop (L, 1);
		lua_pushvalue (L, -2);
		lua_insert (L, -2);		
		lua_settable (L, 2);
	    } else {
		lua_pop (L, 2);
	    }
	}

	lua_pop (L, 1);
    }

    lua_setmetatable (L, 1);

    return 1;
}

int luaopen_morebase (lua_State *L)
{
    luaL_Reg api[] = {
	{"trace", trace},
	{"replacemetatable", replacemetatable},
	{NULL, NULL}
    };

    luaL_requiref(L, "base", luaopen_base, 0);
    luaL_setfuncs(L, api, 0);

    return 1;
}

