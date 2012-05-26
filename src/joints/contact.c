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

#include "techne.h"
#include "../array/array.h"
#include "contact.h"
#include "body.h"

@implementation Contact

-(Joint *) init
{
    self->joint = dJointCreateContact (_WORLD, NULL, &self->contact);

    self->internal = 0;

    self->contact.geom.pos[0] = 0;
    self->contact.geom.pos[1] = 0;
    self->contact.geom.pos[2] = 0;

    self->contact.fdir1[0] = 1;
    self->contact.fdir1[1] = 0;
    self->contact.fdir1[2] = 0;

    self->contact.geom.normal[0] = 0;
    self->contact.geom.normal[1] = 0;
    self->contact.geom.normal[2] = 1;

    self->contact.geom.depth = 0;

    self->contact.surface.mode = 0;

    self->contact.surface.bounce = 0;
    self->contact.surface.bounce_vel = 0.01;
    
    self->contact.surface.mu = 0;
    self->contact.surface.mu2 = 0;
    
    self->contact.surface.soft_cfm = 0;
    self->contact.surface.soft_erp = 0;

    self->contact.geom.g1 = 0;
    self->contact.geom.g2 = 0;

    self = [super init];

    return self;
}

-(Contact *) initWithJoint: (dJointID) j
		andContact: (dContact) c
	       andFeedback: (dJointFeedback) f
{
    self->joint = j;
    self->contact = c;
    self->feedback = f;

    self->internal = 1;

    self = [super init];

    return self;
}

-(void) free
{
    /* Prevent manual destruction of internally generated contact
     * joints. */
    
    if (self->internal) {
	self->joint = NULL;
    }
    
    [super free];
}

-(void) update
{
    if (!self->internal) {
	dGeomID p, q;
	dBodyID a, b;    

	[super update];

	a = dJointGetBody (self->joint, 0);
	b = dJointGetBody (self->joint, 1);
    
	p = a ? dBodyGetFirstGeom (a) : NULL;
	q = b ? dBodyGetFirstGeom (b) : NULL;

	self->contact.geom.g1 = p;
	self->contact.geom.g2 = q;
    }
}

-(void) reset
{
    dBodyID a, b;

    /* Internal joints are reset only if their parameters have been
     * changed.  Manually generated contacts need to be reset at every
     * frame anyway. */

    a = dJointGetBody (self->joint, 0);
    b = dJointGetBody (self->joint, 1);
    
    dJointDestroy (self->joint);
    self->joint = dJointCreateContact (_WORLD, NULL, &self->contact);
    dJointSetFeedback (self->joint, &self->feedback);
    dJointAttach (self->joint, a, b);
}

-(void) _set_bodies
{
    /* Disallow explicit setting of bodies for internally generated
     * joints. */
    
    if (!self->internal) {
	[super _set_bodies];
    }
}

-(int) _get_anchor
{
    array_createarray (_L, ARRAY_TDOUBLE, self->contact.geom.pos, 1, 3);

    return 1;
}

-(int) _get_axes
{
    dVector3 fdir2;
    
    lua_newtable (_L);

    array_createarray (_L, ARRAY_TDOUBLE, self->contact.fdir1, 1, 3);
    lua_rawseti (_L, -2, 1);

    dCROSS (fdir2, =, self->contact.fdir1, self->contact.geom.normal);
    array_createarray (_L, ARRAY_TDOUBLE, fdir2, 1, 3);
    lua_rawseti (_L, -2, 2);
    
    array_createarray (_L, ARRAY_TDOUBLE,
		       self->contact.geom.normal, 1, 3);
    lua_rawseti (_L, -2, 3);

    return 1;
}

-(int) _get_friction
{
    if (self->contact.surface.mode & dContactApprox1) {
	if (self->contact.surface.mode & dContactMu2) {
	    lua_newtable (_L);
		
	    lua_pushnumber (_L, self->contact.surface.mu);
	    lua_rawseti (_L, -2, 1);
		
	    lua_pushnumber (_L, self->contact.surface.mu2);
	    lua_rawseti (_L, -2, 2);
	} else {
	    lua_pushnumber (_L, self->contact.surface.mu);
	}
    } else {
	lua_pushnil (_L);
    }

    return 1;
}

-(int) _get_force
{
    if (self->contact.surface.mode & dContactApprox1) {
	lua_pushnil (_L);
    } else {
	if (self->contact.surface.mode & dContactMu2) {
	    lua_newtable (_L);
		
	    lua_pushnumber (_L, self->contact.surface.mu);
	    lua_rawseti (_L, -2, 1);
		
	    lua_pushnumber (_L, self->contact.surface.mu2);
	    lua_rawseti (_L, -2, 2);
	} else {
	    lua_pushnumber (_L, self->contact.surface.mu);
	}
    }

    return 1;
}

-(int) _get_elasticity
{
    if (self->contact.surface.mode & dContactSoftCFM) {
	lua_newtable (_L);
		
	lua_pushnumber (_L, self->contact.surface.soft_cfm);
	lua_rawseti (_L, -2, 1);
		
	lua_pushnumber (_L, self->contact.surface.soft_erp);
	lua_rawseti (_L, -2, 2);
    } else {
	lua_pushnil (_L);
    }

    return 1;
}

-(int) _get_depth
{
    lua_pushnumber (_L, self->contact.geom.depth);

    return 1;
}

-(int) _get_restitution
{
    if (self->contact.surface.mode & dContactBounce) {	    
	lua_pushnumber (_L, self->contact.surface.bounce);
    } else {
	lua_pushnil (_L);
    }	    

    return 1;
}

-(void) _set_anchor
{
    array_Array *array;
    
    array = array_testcompatible (_L, 3, ARRAY_TDOUBLE, 1, 3);

    if (array) {
	memcpy (self->contact.geom.pos, array->values.any,
		3 * sizeof(double));
    }

    if (self->internal) {
	[self reset];
    };
}

-(void) _set_axes
{
    array_Array *array;
    
    if(!lua_isnil (_L, 3)) {
	lua_rawgeti (_L, 3, 1);
	array = array_testcompatible (_L, -1, ARRAY_TDOUBLE, 1, 3);

	if (array) {
	    memcpy (self->contact.fdir1, array->values.any,
		    3 * sizeof(double));
	}

	dSafeNormalize3 (self->contact.fdir1);

	lua_rawgeti (_L, 3, 3);
	array = array_testcompatible (_L, -1, ARRAY_TDOUBLE, 1, 3);

	if (array) {
	    memcpy (self->contact.geom.normal, array->values.any,
		    3 * sizeof(double));
	}

	dSafeNormalize3 (self->contact.geom.normal);

	lua_pop (_L, 2);
    }

    if (self->internal) {
	[self reset];
    };
}

-(void) _set_friction
{
    if(lua_istable (_L, 3)) {
	self->contact.surface.mode |= dContactApprox1 | dContactMu2 |  dContactFDir1;

	lua_rawgeti (_L, 3, 1);
	self->contact.surface.mu = lua_tonumber (_L, -1);

	lua_rawgeti (_L, 3, 2);
	self->contact.surface.mu2 = lua_tonumber (_L, -1);

	lua_pop (_L, 2);
    } else if(lua_tonumber (_L, 3)) {
	self->contact.surface.mode |= dContactApprox1 |  dContactFDir1;
	self->contact.surface.mode &= ~dContactMu2;

	self->contact.surface.mu = lua_tonumber (_L, 3);
    } else {
	self->contact.surface.mode &= ~(dContactApprox1 | dContactMu2 |  dContactFDir1);
    }

    if (self->internal) {
	[self reset];
    };
}

-(void) _set_force
{
    if(lua_istable (_L, 3)) {
	self->contact.surface.mode |= dContactMu2 |  dContactFDir1;
	self->contact.surface.mode &= ~dContactApprox1;

	lua_rawgeti (_L, 3, 1);
	self->contact.surface.mu = lua_tonumber (_L, -1);

	lua_rawgeti (_L, 3, 2);
	self->contact.surface.mu2 = lua_tonumber (_L, -1);

	lua_pop (_L, 2);
    } else if(lua_tonumber (_L, 3)) {
	self->contact.surface.mode |= dContactFDir1;
	self->contact.surface.mode &= ~dContactApprox1;
	self->contact.surface.mode &= ~dContactMu2;

	self->contact.surface.mu = lua_tonumber (_L, 3);
    } else {
	self->contact.surface.mode &= ~(dContactApprox1 | dContactMu2 |  dContactFDir1);
    }

    if (self->internal) {
	[self reset];
    };
}

-(void) _set_elasticity
{
    if(lua_istable (_L, 3)) {
	self->contact.surface.mode |= dContactSoftERP;
	self->contact.surface.mode |= dContactSoftCFM;

	lua_rawgeti (_L, 3, 1);
	self->contact.surface.soft_cfm = lua_tonumber (_L, -1);

	lua_rawgeti (_L, 3, 2);
	self->contact.surface.soft_erp = lua_tonumber (_L, -1);

	lua_pop (_L, 2);
    } else {
	self->contact.surface.mode &= ~dContactSoftERP;
	self->contact.surface.mode &= ~dContactSoftCFM;
    }	    

    if (self->internal) {
	[self reset];
    };
}

-(void) _set_depth
{
    self->contact.geom.depth = lua_tonumber (_L, 3);

    if (self->internal) {
	[self reset];
    };
}

-(void) _set_restitution
{
    if (lua_isnumber (_L, 3)) {
	self->contact.surface.mode |= dContactBounce;
	self->contact.surface.bounce = lua_tonumber (_L, 3);
    } else {
	self->contact.surface.mode &= ~dContactBounce;
    }	    

    if (self->internal) {
	[self reset];
    };
}

-(void) stepBy: (double) h at: (double) t
{
    if (!self->internal) {
	[self reset];
    }
    
    [super stepBy: h at: t];
}

@end
