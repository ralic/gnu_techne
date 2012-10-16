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

#include "array/array.h"
#include "techne.h"
#include "dynamics.h"
#include "body.h"

static int drawbodies = -1;

static void call_poststep_hook (dBodyID body)
{
    Body *object;

    object = (Body *)dBodyGetData (body);

    t_pushuserdata (_L, 1, object);
    t_callhook (_L, object->poststep, 1, 0);
}

@implementation Body

-(Body *) init
{
    /* Initialize the object. */
    
    [super init];

    if (drawbodies < 0) {
	/* Get the configuration. */
    
	lua_getglobal (_L, "options");

	lua_getfield (_L, -1, "drawbodies");
	drawbodies = lua_toboolean (_L, -1);
	lua_pop (_L, 2);
    }

    dMassSetZero(&self->mass);
    
    self->mass.mass = 1;
    
    self->mass.I[0] = 1;
    self->mass.I[1] = 0;
    self->mass.I[2] = 0;
    self->mass.I[3] = 0;

    self->mass.I[4] = 0;
    self->mass.I[5] = 1;
    self->mass.I[6] = 0;
    self->mass.I[7] = 0;

    self->mass.I[8] = 0;
    self->mass.I[9] = 0;
    self->mass.I[10] = 1;
    self->mass.I[11] = 0;

    self->debug = drawbodies;
    
    self->body = NULL;
    self->space = NULL;

    self->poststep = LUA_REFNIL;
    
    /* Create and attach the body. */
    
    [self release];
    
    return self;
}

-(void) free
{
    if (self->geom) {
	dGeomSetBody (self->geom, NULL);
	dGeomDestroy (self->geom);
    }
    
    if (self->body) {
	dBodyDestroy (self->body);
    }

    luaL_unref (_L, LUA_REGISTRYINDEX, self->poststep);

    [super free];
}

-(void) fasten
{
    assert (self->body);

    dBodyDestroy (self->body);
    self->body = NULL;
    
    if (self->geom) {
	dGeomSetBody (self->geom, NULL);
    }
}

-(void) release
{
    assert (!self->body);

    self->body = dBodyCreate (_WORLD);
    
    dBodySetData (self->body, self);
    dBodySetMass (self->body, &self->mass);
    dBodySetFiniteRotationMode (self->body, 1);
    dBodySetGyroscopicMode (self->body, 1);
    dBodyDisable (self->body);

    if (self->geom) {
	dGeomSetBody (self->geom, self->body);
    }
}

-(void) insertInto: (dSpaceID) new
{
    assert (!self->linked);
    assert (!self->space || !new);
    
    self->space = new;
}

-(void) toggle
{
    [super toggle];

    if (linked) {
	if (self->body) {
	    dBodyEnable (self->body);
	}

	if (self->geom) {
	    if (self->space) {
		dSpaceAdd (self->space, self->geom);
	    } else {
		dSpaceAdd (_SPACE, self->geom);
	    }
	}
    } else {
	if (self->body) {
	    dBodyDisable (self->body);
	}

	if (self->geom) {
	    if (self->space) {
		dSpaceRemove (self->space, self->geom);
	    } else {
		dSpaceRemove (_SPACE, self->geom);
	    }
	}
    }
}

-(void) transform
{
    dVector3 zero = {0, 0, 0};
    dReal *r, *R, *drdt, *dRdt;
    int i, j;
    
    if (self->body) {
	drdt = (dReal *)dBodyGetLinearVel (self->body);
	dRdt = (dReal *)dBodyGetAngularVel (self->body);
    } else {
	drdt = zero;
	dRdt = zero;
    }

    if(self->geom &&
       dGeomGetClass(self->geom) != dPlaneClass) {
	r = (dReal *)dGeomGetPosition (self->geom);
	R = (dReal *)dGeomGetRotation (self->geom);
    } else if (self->body) {
	r = (dReal *)dBodyGetPosition (self->body);
	R = (dReal *)dBodyGetRotation (self->body);
    } else {
	[super transform];
	return;
    }
    
    for(i = 0 ; i < 3 ; i += 1) {
	self->position[i] = r[i];
	self->velocity[i] = drdt[i];
	self->spin[i] = dRdt[i];
    }
    
    for(i = 0 ; i < 3 ; i += 1) {
	for(j = 0 ; j < 3 ; j += 1) {
	    self->orientation[i * 3 + j] = R[i * 4 + j];
	}
    }

    [super transformAsRoot];
}

-(int) _get_position
{
    dReal *r;
    int i;
	
    if(self->geom &&
       dGeomGetClass(self->geom) != dPlaneClass) {
	r = (dReal *)dGeomGetPosition (self->geom);
    } else if (self->body) {
	r = (dReal *)dBodyGetPosition (self->body);
    } else {
	assert (0);
    }
    
    for(i = 0 ; i < 3 ; i += 1) {
	self->position[i] = r[i];
    }

    [super _get_position];

    return 1;
}

-(int) _get_orientation
{
    dReal *R;
    int i, j;
	
    if(self->geom &&
       dGeomGetClass(self->geom) != dPlaneClass) {
	R = (dReal *)dGeomGetRotation (self->geom);
    } else if (self->body) {
	R = (dReal *)dBodyGetRotation (self->body);
    } else {
	assert (0);
    }
    
    for(i = 0 ; i < 3 ; i += 1) {
	for(j = 0 ; j < 3 ; j += 1) {
	    self->orientation[i * 3 + j] = R[i * 4 + j];
	}
    }
    
    [super _get_orientation];
    
    return 1;
}

-(int) _get_velocity
{
    dVector3 zero = {0, 0, 0};
    dReal *v;
	
    if (self->body) {
	v = (dReal *)dBodyGetLinearVel (self->body);
    } else {
	v = zero;
    }

    assert (sizeof(dReal) == sizeof(double));
    array_createarray (_L, ARRAY_TDOUBLE, v, 1, 3);

    return 1;
}

-(int) _get_spin
{
    dVector3 zero = {0, 0, 0};
    dReal *omega;
	
    if (self->body) {
	omega = (dReal *)dBodyGetAngularVel (self->body);
    } else {
	omega = zero;
    }
    
    assert (sizeof(dReal) == sizeof(double));
    array_createarray (_L, ARRAY_TDOUBLE, omega, 1, 3);
    
    return 1;
}

-(int) _get_mass
{
    double *I, *c;
    int i, j;

    dBodyGetMass (self->body, &self->mass);

    lua_newtable (_L);

    /* Mass. */
	
    lua_pushnumber (_L, self->mass.mass);
    lua_rawseti (_L, -2, 1);
    
    /* Center of mass. */
	
    array_createarray (_L, ARRAY_TDOUBLE, NULL, 1, 3);
    c = ((array_Array *)lua_touserdata (_L, -1))->values.doubles;

    for(i = 0 ; i < 3 ; i += 1) {
	c[i] = self->mass.c[i];
    }

    lua_rawseti (_L, -2, 2);

    /* Inertia. */

    array_createarray (_L, ARRAY_TDOUBLE, NULL, 2, 3, 3);
    I = ((array_Array *)lua_touserdata (_L, -1))->values.doubles;

    for(i = 0 ; i < 3 ; i += 1) {
	for(j = 0 ; j < 3 ; j += 1) {
	    I[i * 3 + j] = self->mass.I[i * 4 + j];
	}
    }
    
    lua_rawseti (_L, -2, 3);

    return 1;
}

-(int) _get_lethargy
{
    if (self->body && dBodyGetAutoDisableFlag (self->body)) {
	lua_newtable (_L);

	lua_pushnumber (_L, dBodyGetAutoDisableLinearThreshold(self->body));
	lua_rawseti (_L, -2, 1);

	lua_pushnumber (_L, dBodyGetAutoDisableAngularThreshold(self->body));
	lua_rawseti (_L, -2, 2);

	lua_pushnumber (_L, dBodyGetAutoDisableSteps(self->body));
	lua_rawseti (_L, -2, 3);

	lua_pushnumber (_L, dBodyGetAutoDisableTime(self->body));
	lua_rawseti (_L, -2, 4);
    } else {
	lua_pushboolean (_L, 0);
    }

    return 1;
}

-(int) _get_damping
{
    if (self->body) {
	lua_newtable (_L);

	lua_pushnumber (_L, dBodyGetLinearDampingThreshold(self->body));
	lua_rawseti (_L, -2, 1);

	lua_pushnumber (_L, dBodyGetLinearDamping(self->body));
	lua_rawseti (_L, -2, 2);

	lua_pushnumber (_L, dBodyGetAngularDampingThreshold(self->body));
	lua_rawseti (_L, -2, 3);

	lua_pushnumber (_L, dBodyGetAngularDamping(self->body));
	lua_rawseti (_L, -2, 4);
    } else {
	lua_pushboolean (_L, 0);
    }

    return 1;
}

-(int) _get_kinematic
{
    if (self->body) {
	lua_pushboolean (_L, dBodyIsKinematic (self->body));
    } else {
	lua_pushboolean (_L, 0);
    }

    return 1;
}

-(int) _get_poststep
{
    lua_rawgeti (_L, LUA_REGISTRYINDEX, self->poststep);

    return 1;
}

-(void) _set_velocity
{
    array_Array *array;
    
    array = array_checkcompatible (_L, 3,
                                   ARRAY_TYPE | ARRAY_RANK | ARRAY_SIZE,
                                   ARRAY_TDOUBLE, 1, 3);
    memcpy (self->velocity, array->values.any, 3 * sizeof(double));

    if (self->body) {
	dBodySetLinearVel (self->body,
			   self->velocity[0],
			   self->velocity[1],
			   self->velocity[2]);
    }
}

-(void) _set_spin
{
    array_Array *array;
    
    array = array_checkcompatible (_L, 3,
                                   ARRAY_TYPE | ARRAY_RANK | ARRAY_SIZE,
                                   ARRAY_TDOUBLE, 1, 3);
    memcpy (self->spin, array->values.any, 3 * sizeof(double));
	
    if (self->body) {
	dBodySetAngularVel (self->body,
			    self->spin[0],
			    self->spin[1],
			    self->spin[2]);
    }
}

-(void) _set_mass
{
    array_Array *array;
    double *I, *c, m;

    dMassSetZero(&self->mass);
	
    if(lua_istable (_L, 3)) {
	/* Mass. */
	
	lua_rawgeti (_L, 3, 1);
	m = lua_tonumber (_L, -1);

	/* Center of mass. */
	
	lua_rawgeti (_L, 3, 2);	
	array = array_checkcompatible (_L, -1,
                                       ARRAY_TYPE | ARRAY_RANK | ARRAY_SIZE,
                                       ARRAY_TDOUBLE, 1, 3);
	c = array->values.doubles;

	/* Inertia. */
	
	lua_rawgeti (_L, 3, 3);
	array = array_checkcompatible (_L, -1,
                                       ARRAY_TYPE | ARRAY_RANK | ARRAY_SIZE,
                                       ARRAY_TDOUBLE, 2, 3, 3);
	I = array->values.doubles;

	dMassSetParameters (&self->mass, m,
			    c[0], c[1], c[2],
			    I[0], I[4], I[8],
			    I[1], I[2], I[5]);
	
	lua_settop (_L, 3);
    }

    if (self->body) {
	/* Is this needed? */
	/* dMassTranslate (&self->mass, */
	/* 		    -self->mass.c[0], */
	/* 		    -self->mass.c[1], */
	/* 		    -self->mass.c[2]); */

	dBodySetMass(self->body, &self->mass);
    }
}

-(void) _set_lethargy
{
    if (self->body) {
	if (lua_istable (_L, 3)) {
	    dBodySetAutoDisableFlag (self->body, 1);

	    lua_pushinteger (_L, 1);
	    lua_gettable (_L, 3);
	    dBodySetAutoDisableLinearThreshold (self->body,
						lua_tonumber (_L, -1));
	    lua_pop (_L, 1);

	    lua_pushinteger (_L, 2);
	    lua_gettable (_L, 3);
	    dBodySetAutoDisableAngularThreshold (self->body,
						 lua_tonumber (_L, -1));
	    lua_pop (_L, 1);

	    lua_pushinteger (_L, 3);
	    lua_gettable (_L, 3);
	    dBodySetAutoDisableSteps (self->body, lua_tonumber (_L, -1));
	    lua_pop (_L, 1);

	    lua_pushinteger (_L, 4);
	    lua_gettable (_L, 3);
	    dBodySetAutoDisableTime (self->body, lua_tonumber (_L, -1));
	    lua_pop (_L, 1);
	} else if (lua_isboolean (_L, 3)) {
	    dBodySetAutoDisableFlag (self->body, lua_toboolean (_L, 3));
	} else if (lua_isnil (_L, 3)) {
	    dBodySetAutoDisableFlag (self->body, 0);
	}   
    }
}

-(void) _set_damping
{
    if (self->body && lua_istable (_L, 3)) {
	lua_pushinteger (_L, 1);
	lua_gettable (_L, 3);
	dBodySetLinearDampingThreshold(self->body, lua_tonumber (_L, -1));

	lua_pushinteger (_L, 2);
	lua_gettable (_L, 3);
	dBodySetLinearDamping(self->body, lua_tonumber (_L, -1));

	lua_pushinteger (_L, 3);
	lua_gettable (_L, 3);
	dBodySetAngularDampingThreshold(self->body, lua_tonumber (_L, -1));

	lua_pushinteger (_L, 4);
	lua_gettable (_L, 3);
	dBodySetAngularDamping(self->body, lua_tonumber (_L, -1));

	lua_pop (_L, 4);
    }
}

-(void) _set_kinematic
{
    if (self->body) {
	if (lua_toboolean (_L, 3)) {
	    dBodySetKinematic (self->body);
	} else {
	    dBodySetDynamic (self->body);
	}
    }
}

-(void) _set_poststep
{
    luaL_unref (_L, LUA_REGISTRYINDEX, self->poststep);
    self->poststep = luaL_ref (_L, LUA_REGISTRYINDEX);

    if (self->poststep == LUA_REFNIL) {
	dBodySetMovedCallback (self->body, NULL);
    } else {
	dBodySetMovedCallback (self->body, call_poststep_hook);
    }
}

-(void) _set_position
{
    [super _set_position];

    if (self->body) {
	dBodySetPosition (self->body,
			  self->position[0],
			  self->position[1],
			  self->position[2]);
    }

    if (self->geom &&
	dGeomGetClass(self->geom) != dPlaneClass) {
	dGeomSetPosition (self->geom,
			  self->position[0],
			  self->position[1],
			  self->position[2]);
    }
}

-(void) _set_orientation
{
    dMatrix3 R;
    int i, j;
	    
    [super _set_orientation];

    dRSetIdentity(R);

    for(i = 0 ; i < 3 ; i += 1) {
	for(j = 0 ; j < 3 ; j += 1) {
	    R[i * 4 + j] = self->orientation[i * 3 + j];
	}
    }
	    

    if (self->body) {
	dBodySetRotation (self->body, R);
    }
	    
    if (self->geom &&
	dGeomGetClass(self->geom) != dPlaneClass) {
	dGeomSetRotation (self->geom, R);
    }
}

@end
