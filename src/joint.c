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

static int drawjoints = -1;

@implementation Joint

-(Joint *) init
{
    /* Initialize the object. */
    
    self = [super init];
    
    if (drawjoints < 0) {
	/* Get the configuration. */
    
	lua_getglobal (_L, "options");

	lua_getfield (_L, -1, "drawjoints");
	drawjoints = lua_toboolean (_L, -1);
	lua_pop (_L, 2);
    }

    self->debug = drawjoints;

    self->explicit = 0;
    self->inverted = 0;
    self->attach = LUA_REFNIL;

    if (self->joint) {
	dJointSetFeedback (self->joint, &self->feedback);
	dJointSetData (self->joint, self);
    }

    return self;
}

-(void) free
{
    if (self->joint) {
	dJointDestroy (self->joint);
    }

    luaL_unref (_L, LUA_REGISTRYINDEX, self->attach);
    
    [super free];
}

-(void) update
{
    if (self->linked) {
	Node *a, *b;

	a = self->up;
	
	/* Resolve the parent body if a body hasn't been referenced
	 * explicitly. */

	if (self->explicit < 2) {
	    if (a && [a isKindOf: [Body class]]) {
		self->bodies[0] = ((Body *)a)->body;
	    } else {
		self->bodies[0] = NULL;
	    }
	} else {
	    b = dBodyGetData (self->bodies[0]);
	}

	/* Resolve the child body. Look for a suitable child body if a
	 * body hasn't been referenced explicitly. */

	if (self->explicit < 1) {
	    for (b = self->down;
		 b && ![b isKindOf: [Body class]];
		 b = b->right);
    
	    if (b) {
		self->bodies[1] = ((Body *)b)->body;
	    } else {
		self->bodies[1] = NULL;
	    }
	} else {
	    b = dBodyGetData (self->bodies[1]);
	}

	if (self->joint) {
	    if (self->inverted) {
		dJointAttach (self->joint,
			      self->bodies[1], self->bodies[0]);
		t_push_userdata (_L, 3, self,
				 self->bodies[1],
				 self->bodies[0]);
		t_call_hook (_L, self->attach, 3, 0);
	    } else {
		dJointAttach (self->joint,
			      self->bodies[0], self->bodies[1]);
		t_push_userdata (_L, 3, self,
				 self->bodies[0],
				 self->bodies[1]);
		t_call_hook (_L, self->attach, 3, 0);
	    }
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

-(int) _get_bodies
{
    if (self->explicit == 1) {
	t_push_userdata (_L, 1, dBodyGetData(self->bodies[1]));
    } else if (self->explicit == 2) {
	lua_newtable (_L);
	t_push_userdata (_L, 2, dBodyGetData(self->bodies[1]), dBodyGetData(self->bodies[0]));
	lua_rawseti (_L, -2, 2);
	lua_rawseti (_L, -2, 1);
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
    Body *object;
    int i;
    
    if (lua_isnil (_L, 3)) {
	self->bodies[0] = NULL;
	self->bodies[1] = NULL;
	self->explicit = 0;
    } else if(lua_istable(_L, 3)) {
	for (i = 0 ; i < 2 ; i += 1) {
	    lua_rawgeti (_L, 3, i + 1);
	    object = t_check_node (_L, -1, [Body class]);
	    self->bodies[i] = object->body;
	}

	lua_pop (_L, 2);
	self->explicit = 2;
    } else {
	object = t_check_node (_L, 3, [Body class]);
	    
	self->bodies[1] = object->body;
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
    /* Do nothing. */
}

-(void) _set_torques
{
    /* Do nothing. */
}

@end
