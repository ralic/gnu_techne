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

#include <ode/ode.h>
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include "joint.h"
#include "body.h"
#include "array/array.h"
#include "techne.h"

@implementation Joint

-(void) init
{
    /* Initialize the object. */
    
    [super init];

    self->explicit = 0;
    self->inverted = 0;
    self->objects[0] = NULL;
    self->objects[1] = NULL;
    self->attach = LUA_REFNIL;
    self->references[0] = LUA_REFNIL;
    self->references[1] = LUA_REFNIL;

    if (self->joint) {
	dJointSetFeedback (self->joint, &self->feedback);
	dJointSetData (self->joint, self);
    }
}

-(void) free
{
    if (self->joint) {
	dJointDestroy (self->joint);
    }

    luaL_unref (_L, LUA_REGISTRYINDEX, self->attach);
    luaL_unref (_L, LUA_REGISTRYINDEX, self->references[0]);
    luaL_unref (_L, LUA_REGISTRYINDEX, self->references[1]);

    [super free];
}

-(void) update
{
    if (self->linked) {
	Node *a, *b;

	a = self->up;
	
	if (self->explicit < 2) {
            /* Attach to the parent body if only one body has been
             * referenced explicitly. */

	    if (a && [a isKindOf: [Body class]]) {
		self->objects[0] = (Body *)a;
	    } else {
		self->objects[0] = NULL;
	    }
	}

	if (self->explicit < 1) {
            /* Attach to a child body if no body has been referenced
             * explicitly. */

	    for (b = self->down;
		 b && ![b isKindOf: [Body class]];
		 b = b->right);
    
	    if (b) {
		self->objects[1] = (Body *)b;
	    } else {
		self->objects[1] = NULL;
	    }
	}
        
        self->bodies[0] = self->objects[0] ? self->objects[0]->body : NULL;
        self->bodies[1] = self->objects[1] ? self->objects[1]->body : NULL;
        
        if (self->inverted) {
            if (self->joint) {
		dJointAttach (self->joint, self->bodies[1], self->bodies[0]);
            }

            t_pushuserdata (_L, 3, self, self->objects[1], self->objects[0]);
            t_callhook (_L, self->attach, 3, 0);
        } else {
            if (self->joint) {
                dJointAttach (self->joint, self->bodies[0], self->bodies[1]);
            }
            
            t_pushuserdata (_L, 3, self, self->objects[0], self->objects[1]);
            t_callhook (_L, self->attach, 3, 0);
	}
    } else {
	if (self->joint) {
	    dJointAttach (self->joint, NULL, NULL);
	}
    }	
}

-(void) toggle
{
    [super toggle];
    [self update];
}

-(void) meetParent: (Node *)parent
{
    [super meetParent: parent];

    if ([parent isKindOf: [Body class]]) {
	[self update];
    }
}

-(void) adopt: (Node *)child
{
    [super adopt: child];
    
    if ([child isKindOf: [Body class]]) {
	[self update];
    }
}

-(void) renounce: (Node *)child
{
    [super renounce: child];
    
    if ([child isKindOf: [Body class]]) {
	[self update];
    }
}

-(void) transform
{
    [super transformAsRoot];
}

-(int) _get_forces
{
    lua_newtable (_L);

    /* The force applied on the first body. */
	
    if (self->inverted) {
	array_createarray (_L, ARRAY_TDOUBLE,
			   self->feedback.f2, 1, 3);
    } else {
	array_createarray (_L, ARRAY_TDOUBLE,
			   self->feedback.f1, 1, 3);
    }

    lua_rawseti (_L, -2, 1);

    /* The force applied on the second body. */
	
    if (self->inverted) {
	array_createarray (_L, ARRAY_TDOUBLE,
			   self->feedback.f1, 3);
    } else {
	array_createarray (_L, ARRAY_TDOUBLE,
			   self->feedback.f2, 1, 3);
    }

    lua_rawseti (_L, -2, 2);

    return 1;
}

-(int) _get_torques
{
    lua_newtable (_L);

    /* The torque applied on the first body. */
	
    if (self->inverted) {
	array_createarray (_L, ARRAY_TDOUBLE,
			   self->feedback.t2, 1, 3);
    } else {
	array_createarray (_L, ARRAY_TDOUBLE,
			   self->feedback.t1, 1, 3);
    }

    lua_rawseti (_L, -2, 1);

    /* The torque applied on the second body. */

    if (self->inverted) {
	array_createarray (_L, ARRAY_TDOUBLE,
			   self->feedback.t1, 1, 3);
    } else {
	array_createarray (_L, ARRAY_TDOUBLE,
			   self->feedback.t2, 1, 3);
    }

    lua_rawseti (_L, -2, 2);	

    return 1;
}

-(int) _get_inverted
{
    lua_pushboolean (_L, self->inverted);

    return 1;
}

-(int) _get_pair
{
    if (self->bodies[0] ||  self->bodies[1]) {
	lua_createtable (_L, 2, 0);

	if (self->inverted) {
	    t_pushuserdata (_L, 2, self->objects[1], self->objects[0]);
	} else {
	    t_pushuserdata (_L, 2, self->objects[0], self->objects[1]);
	}

	lua_rawseti (_L, 3, 2);
	lua_rawseti (_L, 3, 1);
    } else {
	lua_pushnil(_L);
    }
    
    return 1;
}

-(int) _get_bodies
{
    if (self->explicit == 1) {
	lua_rawgeti(_L, LUA_REGISTRYINDEX, self->references[1]);
    } else if (self->explicit == 2) {
	lua_createtable (_L, 2, 0);
	lua_rawgeti(_L, LUA_REGISTRYINDEX, self->references[0]);
	lua_rawseti (_L, -2, 1);
	lua_rawgeti(_L, LUA_REGISTRYINDEX, self->references[1]);
	lua_rawseti (_L, -2, 2);
    } else {
	lua_pushnil (_L);
    }
    
    return 1;
}

-(int) _get_attach
{
    lua_rawgeti (_L, LUA_REGISTRYINDEX, self->attach);

    return 1;
}

-(void) _set_inverted
{
    self->inverted = lua_toboolean (_L, 3);

    [self update];
}

-(void) _set_bodies
{
    int i;

    luaL_unref (_L, LUA_REGISTRYINDEX, self->references[0]);
    luaL_unref (_L, LUA_REGISTRYINDEX, self->references[1]);

    if (lua_isnil (_L, 3)) {
	self->objects[0] = NULL;
	self->objects[1] = NULL;
        
	self->explicit = 0;
    } else if(lua_istable(_L, 3)) {
	for (i = 0 ; i < 2 ; i += 1) {
	    lua_rawgeti (_L, 3, i + 1);
            
            self->objects[i] = t_testnode (_L, -1, [Body class]);
            self->references[i] = luaL_ref(_L, LUA_REGISTRYINDEX);
	}

	self->explicit = 2;
    } else {
	self->objects[0] = NULL;
	self->objects[1] = t_checknode (_L, 3, [Body class]);	    
        self->references[1] = luaL_ref(_L, LUA_REGISTRYINDEX);
        self->references[0] = LUA_REFNIL;
        
	self->explicit = 1;
    }

    [self update];
}

-(void) _set_attach
{
    luaL_unref (_L, LUA_REGISTRYINDEX, self->attach);
    self->attach = luaL_ref (_L, LUA_REGISTRYINDEX);
}

-(void) _set_forces
{
    T_WARN_READONLY;
}

-(void) _set_torques
{
    T_WARN_READONLY;
}

-(void) _set_pair
{
    T_WARN_READONLY;
}

@end
