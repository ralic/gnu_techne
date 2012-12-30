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

#include <lualib.h>
#include <lauxlib.h>

#include "event.h"
#include "techne.h"

static void recurse (Node *root)
{
    Node *child, *next;

    t_begin_interval (root);

    if ([root isKindOf: [Event class]]) {
	[(Event *)root input];
    } else {
	for (child = root->down ; child ; child = next) {
	    next = child->right;	    
	    recurse (child);
	}
    }
    
    t_end_interval (root);
}

@implementation Event

-(void) input
{
    Node *child, *sister;
    
    t_pushuserdata (_L, 1, self);
    t_callhook (_L, self->input, 1, 0);
    
    for (child = self->down ; child ; child = sister) {
	sister = child->right;
	recurse (child);
    }
}

-(void) init
{
    [super init];

    self->input = LUA_REFNIL;
}

-(void) free
{
    luaL_unref (_L, LUA_REGISTRYINDEX, self->input);
    
    [super free];
}

-(int) _get_input
{
    lua_rawgeti (_L, LUA_REGISTRYINDEX, self->input);

    return 1;
}

-(void) _set_input
{
    luaL_unref (_L, LUA_REGISTRYINDEX, self->input);
    self->input = luaL_ref (_L, LUA_REGISTRYINDEX);
}

@end

