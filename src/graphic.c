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

#include "gl.h"

#include "graphic.h"
#include "techne.h"

void t_draw_subtree (Node *root, int frame)
{
    Node *child, *next;

    t_begin_cpu_interval (&root->core);

    if ([root isKindOf: [Graphic class]]) {
        Graphic *graphic = (Graphic *)root;

        t_begin_gpu_interval (&graphic->graphics);
	[graphic draw: frame];
        t_end_gpu_interval (&graphic->graphics);
    } else {
	for (child = root->down ; child ; child = next) {
	    next = child->right;	    
	    t_draw_subtree (child, frame);
	}
    }
    
    t_end_cpu_interval (&root->core);
}

@implementation Graphic

-(void) draw: (int)frame
{
    Node *child, *sister;
    
    t_pushuserdata (_L, 1, self);
    lua_pushinteger (_L, frame);
    t_callhook (_L, self->draw, 2, 0);
    
    for (child = self->down ; child ; child = sister) {
	sister = child->right;
	t_draw_subtree (child, frame);
    }
}

-(void) init
{
    [super init];
    
    self->draw = LUA_REFNIL;
}

-(void) free
{
    luaL_unref (_L, LUA_REGISTRYINDEX, self->draw);
    t_free_profiling_queries(&self->graphics);
    
    [super free];
}
 
-(int) _get_graphics
{
    t_pushgraphicsinterval(_L, &self->graphics);
    
    return 1;
}

-(void) _set_graphics
{
    T_WARN_READONLY;
}

-(int) _get_draw
{
    lua_rawgeti (_L, LUA_REGISTRYINDEX, self->draw);

    return 1;
}

-(void) _set_draw
{
    luaL_unref (_L, LUA_REGISTRYINDEX, self->draw);
    self->draw = luaL_ref (_L, LUA_REGISTRYINDEX);
}

@end

