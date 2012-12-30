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

#include "gl.h"

#include "techne.h"
#include "algebra.h"
#include "widget.h"

static int drawlayout = -1;

static void recurse (Node *root)
{
    Node *child, *next;

    if ([root isKindOf: [Widget class]]) {
	[(Widget *)root arrange];
    } else {
	for (child = root->down ; child ; child = next) {
	    next = child->right;	    
	    recurse (child);
	}
    }
}

@implementation Widget

-(void)init
{
    [super init];

    if (drawlayout < 0) {
	/* Get the configuration. */
    
	lua_getglobal (_L, "options");

	lua_getfield (_L, -1, "drawlayout");
	drawlayout = lua_toboolean (_L, -1);
	lua_pop (_L, 2);
    }

    self->align[0] = 0;
    self->align[1] = 0;
    
    self->offset[0] = 0;
    self->offset[1] = 0;
    
    self->padding[0] = 0;
    self->padding[1] = 0;
    self->padding[2] = 0;
    self->padding[3] = 0;

    self->debug = drawlayout;
}

-(void) measure
{
}

-(void) arrange
{
    Node *child, *sister;    
    
    for (child = self->down ; child ; child = sister) {
	sister = child->right;
	recurse (child);
    }
}

-(int) _get_align
{
    int i;
    
    lua_newtable (_L);
        
    for(i = 0; i < 2; i += 1) {
	lua_pushnumber (_L, self->align[i]);
	lua_rawseti (_L, -2, i + 1);
    }

    return 1;
}

-(int) _get_padding
{
    int i;
    
    lua_newtable (_L);
        
    for(i = 0; i < 4; i += 1) {
	lua_pushnumber (_L, self->padding[i]);
	lua_rawseti (_L, -2, i + 1);
    }

    return 1;
}

-(int) _get_offset
{
    int i;
    
    lua_newtable (_L);
        
    for(i = 0; i < 2; i += 1) {
	lua_pushnumber (_L, self->offset[i]);
	lua_rawseti (_L, -2, i + 1);
    }

    return 1;
}

-(int) _get_content
{
    int i;
    
    lua_newtable (_L);
        
    for(i = 0; i < 2; i += 1) {
	lua_pushnumber (_L, self->content[i]);
	lua_rawseti (_L, -2, i + 1);
    }

    return 1;
}

-(void) _set_align
{
    int i;
    
    if(lua_istable (_L, 3)) {
	for(i = 0 ; i < 2 ; i += 1) {
	    lua_pushinteger (_L, i + 1);
	    lua_gettable (_L, 3);
	    self->align[i] = lua_tonumber (_L, -1);
                
	    lua_pop (_L, 1);
	}
    }
}

-(void) _set_padding
{
    int i;
    
    if(lua_istable (_L, 3)) {
	for(i = 0 ; i < 4 ; i += 1) {
	    lua_pushinteger (_L, i + 1);
	    lua_gettable (_L, 3);
	    self->padding[i] = lua_tonumber (_L, -1);
                
	    lua_pop (_L, 1);
	}
    }
}

-(void) _set_offset
{
    int i;
    
    if(lua_istable (_L, 3)) {
	for(i = 0 ; i < 2 ; i += 1) {
	    lua_pushinteger (_L, i + 1);
	    lua_gettable (_L, 3);
	    self->offset[i] = lua_tonumber (_L, -1);
                
	    lua_pop (_L, 1);
	}
    }
}

-(void) _set_content
{
    T_WARN_READONLY;
}

-(void) draw
{
    double *d, *m, *p, *a, *A;
    float M[16];
    
    d = self->offset;
    m = self->content;
    p = self->padding;
    a = self->allocation;
    A = self->align;

    t_load_identity_4(M);
    
    M[12] = d[0] + p[0] - 0.5 * (a[0] - m[0]) +
        0.5 * (A[0] + 1) * (a[0] - m[0] - p[0] - p[1]);
    M[13] = d[1] + p[2] - 0.5 * (a[1] - m[1]) +
        0.5 * (A[1] + 1) * (a[1] - m[1] - p[2] - p[3]);

    t_push_modelview(M, T_MULTIPLY);

    [super draw];

    t_pop_modelview();
}

@end
