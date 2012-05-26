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

#include "array/array.h"
#include "techne.h"
#include "shape.h"

@implementation Shape

-(Shape *)init
{
    [super init];

    self->reference = LUA_REFNIL;
    self->vertices = NULL;

    self->width = 1;

    self->color[0] = 1;
    self->color[1] = 1;
    self->color[2] = 1;
    self->color[3] = 1;

    return self;
}

-(int) _get_vertices
{
    lua_rawgeti (_L, LUA_REGISTRYINDEX, self->reference);

    return 1;
}

-(int) _get_width
{
    lua_pushnumber (_L, self->width);

    return 1;
}

-(int) _get_opacity
{
    lua_pushnumber (_L, self->color[3]);

    return 1;
}

-(int) _get_color
{
    array_createarray (_L, ARRAY_TDOUBLE, self->color, 1, 3);

    return 1;
}

-(void) _set_vertices
{
    luaL_unref (_L, LUA_REGISTRYINDEX, self->reference);

    self->vertices = array_checkarray (_L, 3);
    self->reference = luaL_ref (_L, LUA_REGISTRYINDEX);    
}

-(void) _set_width
{
    self->width = lua_tonumber (_L, 3);
}

-(void) _set_opacity
{
    self->color[3] = lua_tonumber (_L, 3);
}

-(void) _set_color
{
    array_Array *array;
    int i;
    
    array = array_checkcompatible (_L, 3, ARRAY_TDOUBLE, 1, 3);

    for(i = 0 ; i < 3 ; i += 1) {
	self->color[i] = array->values.doubles[i];
    }
}

-(void) free
{
    luaL_unref (_L, LUA_REGISTRYINDEX, self->reference);

    [super free];
}

@end
