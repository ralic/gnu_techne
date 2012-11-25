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
#include <lualib.h>
#include <lauxlib.h>

#include <ode/ode.h>
#include "gl.h"

#include "array/array.h"
#include "techne.h"
#include "dynamics.h"
#include "spherical.h"

@implementation Spherical

-(void) init
{
    self->joint = dJointCreateBall (_WORLD, NULL);

    self->anchor[0] = 0;
    self->anchor[1] = 0;
    self->anchor[2] = 0;

    [super init];
}

-(void) update
{
    dBodyID a, b;

    a = dJointGetBody (self->joint, 0);
    b = dJointGetBody (self->joint, 1);

    /* If the joint is already connected update the anchor. */
    
    if (a || b) {
	dJointGetBallAnchor (self->joint, self->anchor);
    }

    [super update];

    a = dJointGetBody (self->joint, 0);
    b = dJointGetBody (self->joint, 1);

    if (a || b) {
    	dJointSetBallAnchor (self->joint,
			     self->anchor[0],
			     self->anchor[1],
			     self->anchor[2]);
    }
}

-(int) _get_anchor
{
    dJointGetBallAnchor (self->joint, self->anchor);
    array_createarray (_L, ARRAY_TDOUBLE, self->anchor, 1, 3);

    return 1;
}

-(int) _get_tolerance
{
    int j;
    
    lua_newtable (_L);
        
    for(j = 0 ; j < 3 ; j += 1) {
	lua_pushnumber (_L, self->tolerance[j]);

	lua_rawseti (_L, -2, j + 1);
    }

    return 1;
}

-(void) _set_anchor
{
    array_Array *array;
    int i;
    
    if(!lua_isnil (_L, 3)) {
	array = array_checkcompatible (_L, 3,
                                       ARRAY_TYPE | ARRAY_RANK | ARRAY_SIZE,
                                       ARRAY_TDOUBLE, 1, 3);

	dJointSetBallAnchor (self->joint,
			     array->values.doubles[0],
			     array->values.doubles[1],
			     array->values.doubles[2]);

	for (i = 0 ; i < 3 ; i += 1) {
	    self->anchor[i] = array->values.doubles[i];
	}
    }
}

-(void) _set_tolerance
{
    int j;
    
    if(lua_istable (_L, 3)) {
	for(j = 0 ; j < 3 ; j += 1) {
	    lua_rawgeti (_L, 3, j + 1);
	    self->tolerance[j] = lua_tonumber (_L, -1);
                
	    lua_pop (_L, 1);

	    dJointSetBallParam (self->joint,
				dParamCFM + dParamGroup * j,
				self->tolerance[j]);
	}
    }
}

@end
