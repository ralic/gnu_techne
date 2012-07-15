/* Copyright (C) 2009 Papavasileiou Dimitris                             
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

#include <lua.h>
#include <lauxlib.h>

#include "prompt/prompt.h"
#include "techne.h"
#include "node.h"

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

static int describe (lua_State *L)
{
    int i, h;
    
    h = lua_gettop (L);
    
    for (i = 1 ; i <= h ; i += 1) {
	id node;

	node = t_check_node (L, i, [Node class]);
	[node describe];
    }

    return 0;
}

int luaopen_console_core (lua_State *L)
{
    luaL_Reg core[] = {
	{"trace", trace},
	{"describe", describe},
	{NULL, NULL},
    };
    
    lua_newtable (L);
    luaL_newlib (L, core);

    return 1;
}
