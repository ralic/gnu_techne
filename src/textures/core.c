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

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include "texture.h"

#define DEFINE_TEXTURE(texture, target)				\
    static int export_##texture (lua_State *L)			\
    {								\
	[[Texture alloc] initWithTarget: target];               \
								\
	/* ...and initialize it. */				\
								\
	if(lua_istable(L, 1)) {					\
	    lua_pushnil(L);					\
								\
	    while(lua_next(L, 1)) {				\
		lua_pushvalue(L, -2);				\
		lua_insert(L, -2);				\
		lua_settable(L, 2);				\
	    }							\
	}							\
								\
	return 1;						\
    }

DEFINE_TEXTURE(planar, GL_TEXTURE_2D)

int luaopen_textures_core (lua_State *L)
{
    const luaL_Reg textures[] = {
	{"planar", export_planar},
	{NULL, NULL}
    };

    lua_newtable (L);
    luaL_setfuncs (L, textures, 0);
    
    return 1;
}
