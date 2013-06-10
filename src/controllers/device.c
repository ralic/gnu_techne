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
#include <ctype.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/input.h>

#include "techne.h"
#include "device.h"

@implementation Device

-(void) init
{
    [super init];

    self->buttonpress = LUA_REFNIL;
    self->buttonrelease = LUA_REFNIL;
    self->absolute = LUA_REFNIL;
    self->relative = LUA_REFNIL;
}

-(void) free
{
    luaL_unref (_L, LUA_REGISTRYINDEX, self->buttonpress);
    luaL_unref (_L, LUA_REGISTRYINDEX, self->buttonrelease);
    luaL_unref (_L, LUA_REGISTRYINDEX, self->absolute);
    luaL_unref (_L, LUA_REGISTRYINDEX, self->relative);

    [super free];
}

-(int) _get_buttonpress
{
    lua_rawgeti(_L, LUA_REGISTRYINDEX, self->buttonpress);

    return 1;
}

-(int) _get_buttonrelease
{
    lua_rawgeti(_L, LUA_REGISTRYINDEX, self->buttonrelease);

    return 1;
}

-(int) _get_absolute
{
    lua_rawgeti(_L, LUA_REGISTRYINDEX, self->absolute);

    return 1;
}

-(int) _get_relative
{
    lua_rawgeti(_L, LUA_REGISTRYINDEX, self->relative);

    return 1;
}

-(void) _set_buttonpress
{
    luaL_unref (_L, LUA_REGISTRYINDEX, self->buttonpress);
    self->buttonpress = luaL_ref (_L, LUA_REGISTRYINDEX);
}

-(void) _set_buttonrelease
{
    luaL_unref (_L, LUA_REGISTRYINDEX, self->buttonrelease);
    self->buttonrelease = luaL_ref (_L, LUA_REGISTRYINDEX);
}

-(void) _set_absolute
{
    luaL_unref (_L, LUA_REGISTRYINDEX, self->absolute);
    self->absolute = luaL_ref (_L, LUA_REGISTRYINDEX);
}

-(void) _set_relative
{
    luaL_unref (_L, LUA_REGISTRYINDEX, self->relative);
    self->relative = luaL_ref (_L, LUA_REGISTRYINDEX);
}

@end
