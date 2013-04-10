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

#include "techne.h"
#include "elevation.h"
#include "swatch.h"

@implementation Swatch

-(void) init
{
    int i;

    [super init];
    
    for (i = 0 ; i < 3 ; i += 1) {
        self->values[i] = 1;
        self->weights[i] = 1;
    }
}

-(void) free
{
    luaL_unref (_L, LUA_REGISTRYINDEX, self->reference);
        
    [super free];
}

-(int) _get_reference
{
    int j;
    
    lua_createtable (_L, 3, 0);

    for (j = 0; j < 3 ; j += 1) {
        if (self->weights[j] > 0) {
            lua_pushnumber(_L, self->values[j]);
        } else {
            lua_pushnil(_L);
        }
                
        lua_rawseti(_L, -2, j + 1);
    }

    return 1;
}

-(void) _set_reference
{
    int i;

    if (lua_istable (_L, 3)) {
        for (i = 0 ; i < 3 ; i += 1) {
            lua_pushinteger (_L, i + 1);
            lua_gettable (_L, 3);

            if (lua_isnumber(_L, -1)) {
                self->values[i] = lua_tonumber (_L, -1);
                self->weights[i] = 1;
            } else {
                self->values[i] = 0;
                self->weights[i] = 0;
            }			    
			
            lua_pop(_L, 1);
        }
    }
}

-(int) _get_resolution
{
    int j;
    
    lua_createtable (_L, 2, 0);

    for (j = 0; j < 2 ; j += 1) {
        lua_pushnumber(_L, self->resolutions[j]);                
        lua_rawseti(_L, -2, j + 1);
    }

    return 1;
}

-(void) _set_resolution
{
    int i;

    if (!lua_isnil (_L, 3)) {
        for (i = 0 ; i < 2 ; i += 1) {
            lua_pushinteger (_L, i + 1);
            lua_gettable (_L, 3);

            self->resolutions[i] = lua_tonumber (_L, -1);
			
            lua_pop(_L, 1);
        }
    }
}

-(int) _get_detail
{
    lua_rawgeti(_L, LUA_REGISTRYINDEX, self->reference);
    
    return 1;
}

-(void) _set_detail
{
    self->detail = t_testtexture (_L, -1, GL_TEXTURE_2D);
    luaL_ref(_L, LUA_REGISTRYINDEX);
}

-(void) meetParent: (Shader *)parent
{
    if (![parent isKindOf: [Elevation class]]) {
	t_print_warning("%s node has no Elevation parent.\n",
			[self name]);
	
	return;
    }
}

-(void) addSourceToVegetationShader: (ShaderMold *)shader for: (t_Enumerated)stage
{
}

-(void) configureVegetationShader: (Shader *)shader
{
}

@end
