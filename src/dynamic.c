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

#include "dynamic.h"
#include "techne.h"

void t_step_subtree (Node *root, double h, double t)
{
    Node *child, *next;

    t_begin_cpu_interval (&root->core);

    if ([root isKindOf: [Dynamic class]]) {
	[(id)root stepBy: h at: t];
    } else {
	for (child = root->down ; child ; child = next) {
	    next = child->right;	    
	    t_step_subtree (child, h, t);
	}
    }
    
    t_end_cpu_interval (&root->core);
}

@implementation Dynamic

-(void) init
{
    [super init];

    self->step = LUA_REFNIL;
}

-(void) free
{
    luaL_unref (_L, LUA_REGISTRYINDEX, self->step);
    
    [super free];
}


-(void) stepBy: (double) h at: (double) t
{
    Node *child, *sister;

    t_pushuserdata (_L, 1, self);
    lua_pushnumber (_L, h);
    lua_pushnumber (_L, t);
    t_callhook (_L, self->step, 3, 0);
    
    for (child = self->down ; child ; child = sister) {
	sister = child->right;
	t_step_subtree (child, h, t);
    }
}

-(int) _get_step
{
    lua_rawgeti (_L, LUA_REGISTRYINDEX, self->step);

    return 1;
}

-(void) _set_step
{
    luaL_unref (_L, LUA_REGISTRYINDEX, self->step);
    self->step = luaL_ref (_L, LUA_REGISTRYINDEX);
}

@end

