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
#include "angular.h"

@implementation Angular

-(void) init
{
    self->joint = dJointCreateAMotor (_WORLD, NULL);
    
    self->axes[0][0] = 1;
    self->axes[0][1] = 0;
    self->axes[0][2] = 0;

    self->axes[1][0] = 0;
    self->axes[1][1] = 1;
    self->axes[1][2] = 0;

    self->axes[2][0] = 0;
    self->axes[2][1] = 0;
    self->axes[2][2] = 1;
    
    self->stops[0][0] = -dInfinity;
    self->stops[0][1] = dInfinity;

    self->stops[1][0] = -dInfinity;
    self->stops[1][1] = dInfinity;

    self->stops[2][0] = -dInfinity;
    self->stops[2][1] = dInfinity;

    self->degrees = 1;

    self->relative[0] = 0;
    self->relative[1] = 0;
    self->relative[2] = 0;

    [super init];
}

-(void) setup
{
    int i;

    for (i = 0 ; i < self->degrees ; i += 1) {	
	dJointSetAMotorAxis (self->joint, i,
			     self->relative[i],
			     self->axes[i][0],
			     self->axes[i][1],
			     self->axes[i][2]);
    }
}

-(void) update
{
    /* Reset the axes after reattaching. */

    [super update];
    [self setup];
}

-(int) _get_axes
{
    int j;
    
    dJointGetAMotorAxis (self->joint, 0, self->axes[0]);
    dJointGetAMotorAxis (self->joint, 1, self->axes[1]);
    dJointGetAMotorAxis (self->joint, 2, self->axes[2]);

    lua_newtable (_L);
        
    for(j = 0 ; j < self->degrees ; j += 1) {
	array_createarray (_L, ARRAY_TDOUBLE, self->axes[j], 1, 3);

	lua_rawseti (_L, -2, j + 1);
    }

    return 1;
}

-(int) _get_relative
{
    int j;
    
    lua_newtable (_L);
        
    for(j = 0 ; j < self->degrees ; j += 1) {
	lua_pushnumber (_L, self->relative[j]);
	lua_rawseti (_L, -2, j + 1);
    }

    return 1;
}

-(int) _get_motor
{
    int i, j;
    
    lua_newtable (_L);
        
    for(j = 0 ; j < self->degrees ; j += 1) {
	lua_newtable (_L);
        
	for(i = 0; i < 2; i += 1) {
	    lua_pushnumber (_L, self->motor[j][i]);
	    lua_rawseti (_L, -2, i + 1);
	}

	lua_rawseti (_L, -2, j + 1);
    }

    return 1;
}

-(int) _get_stops
{
    int i, j;
    
    lua_newtable (_L);
        
    for(j = 0 ; j < self->degrees ; j += 1) {
	lua_newtable (_L);

	lua_newtable (_L);
	for(i = 0; i < 2; i += 1) {
	    lua_pushnumber (_L, self->stops[j][i]);
	    lua_rawseti (_L, -2, i + 1);
	}
	lua_rawseti (_L, -2, 1);
	
	lua_newtable (_L);
	for(i = 0; i < 2; i += 1) {
	    lua_pushnumber (_L, self->hardness[j][i]);
	    lua_rawseti (_L, -2, i + 1);
	}
	lua_rawseti (_L, -2, 2);

	lua_pushnumber (_L, bounce[j]);
	lua_rawseti (_L, -2, 3);

	lua_rawseti (_L, -2, j + 1);
    }

    return 1;
}

-(int) _get_tolerance
{
    int j;
    
    lua_newtable (_L);
        
    for(j = 0 ; j < self->degrees ; j += 1) {
	lua_pushnumber (_L, self->tolerance[j]);

	lua_rawseti (_L, -2, j + 1);
    }

    return 1;
}

-(int) _get_angles
{
    int j;
    
    lua_newtable (_L);

    for(j = 0 ; j < self->degrees ; j += 1) {
	lua_pushnumber (_L, dJointGetAMotorAngle (self->joint, j));
	lua_rawseti (_L, -2, j + 1);
    }

    return 1;
}

-(int) _get_rates
{
    int j;
    
    lua_newtable (_L);

    for(j = 0 ; j < self->degrees ; j += 1) {
	lua_pushnumber (_L, dJointGetAMotorAngleRate (self->joint, j));
	lua_rawseti (_L, -2, j + 1);
    }

    return 1;
}

-(void) _set_axes
{
    array_Array *arrays[3];
    int i, j;
    
    if(!lua_isnil (_L, 3)) {
	lua_len (_L, 3);
	self->degrees = lua_tointeger (_L, -1);
	lua_pop (_L, 1);

	dJointSetAMotorNumAxes (self->joint, self->degrees);
	    
	for(j = 0 ; j < self->degrees ; j += 1) {
	    lua_pushinteger (_L, j + 1);
	    lua_gettable (_L, 3);
	    arrays[j] = array_checkcompatible (_L, -1,
                                               ARRAY_TYPE | ARRAY_RANK |
                                               ARRAY_SIZE,
                                               ARRAY_TDOUBLE, 1, 3);
	    dSafeNormalize3 (arrays[j]->values.doubles);
	    lua_pop (_L, 1);

	    for (i = 0 ; i < 3 ; i += 1) {
		self->axes[j][i] = arrays[j]->values.doubles[i];
	    }	    
	}

	/* Setup joint axes. */
		
	[self setup];
    }
}

-(void) _set_relative
{
    int j;

    if(lua_istable (_L, 3)) {
	for(j = 0 ; j < 3 ; j += 1) {
	    if(lua_istable (_L, 3)) {
		lua_rawgeti (_L, 3, j + 1);
		self->relative[j] = lua_tonumber (_L, -1);
		lua_pop(_L, 1);
	    } else if(lua_isnumber (_L, 3)) {
		self->relative[j] = lua_tonumber (_L, 3);
	    }

	    lua_pop (_L, 1);
	}

	/* Setup joint axes. */
		
	[self setup];
    }
}

-(void) _set_motor
{
    int i, j;
    
    if(lua_istable (_L, 3)) {
	for(j = 0 ; j < 3 ; j += 1) {
	    lua_rawgeti (_L, 3, j + 1);
		
	    if(lua_istable (_L, -1)) {
		for(i = 0 ; i < 2 ; i += 1) {
		    lua_rawgeti (_L, -1, i + 1);
		    self->motor[j][i] = lua_tonumber (_L, -1);
                
		    lua_pop (_L, 1);
		}

		dJointSetAMotorParam (self->joint,
				      dParamVel + dParamGroup * j,
				      self->motor[j][0]);
		    
		dJointSetAMotorParam (self->joint,
				      dParamFMax + dParamGroup * j,
				      self->motor[j][1]);
	    }

	    lua_pop (_L, 1);
	}
    }
}

-(void) _set_stops
{
    int i, j;
    
    /* Resetting the self->stops makes sure that lo remains
       smaller than hi between calls. */

    for(j = 0 ; j < 3 ; j += 1) {
	dJointSetAMotorParam (self->joint,
			      dParamLoStop + dParamGroup * j,
			      -dInfinity);
		    
	dJointSetAMotorParam (self->joint,
			      dParamHiStop + dParamGroup * j,
			      dInfinity);
    }
	
    if(lua_istable (_L, 3)) {
        self->degrees = lua_rawlen(_L, 3);
        
	for(j = 0 ; j < self->degrees ; j += 1) {
	    lua_rawgeti (_L, 3, j + 1);
		
	    if(lua_istable (_L, -1)) {
		lua_rawgeti (_L, -1, 1);
		for(i = 0 ; i < 2 ; i += 1) {
		    lua_rawgeti (_L, -1, i + 1);
		
		    self->stops[j][i] = lua_tonumber (_L, -1);
		
		    lua_pop (_L, 1);
		}
		lua_pop (_L, 1);

		lua_rawgeti (_L, -1, 2);
		for(i = 0 ; i < 2 ; i += 1) {
		    lua_rawgeti (_L, -1, i + 1);

		    self->hardness[j][i] = lua_tonumber (_L, -1);
			
		    lua_pop (_L, 1);
		}
		lua_pop (_L, 1);
	
		lua_rawgeti (_L, -1, 3);
		bounce[j] = lua_tonumber (_L, -1);
		lua_pop (_L, 1);
		    
		dJointSetAMotorParam (self->joint,
				      dParamLoStop + dParamGroup * j,
				      self->stops[j][0]);

		dJointSetAMotorParam (self->joint,
				      dParamHiStop + dParamGroup * j,
				      self->stops[j][1]);

		dJointSetAMotorParam (self->joint,
				      dParamStopCFM + dParamGroup * j,
				      self->hardness[j][0]);

		dJointSetAMotorParam (self->joint,
				      dParamStopERP + dParamGroup * j,
				      self->hardness[j][1]);

		dJointSetAMotorParam (self->joint,
				      dParamBounce + dParamGroup * j,
				      bounce[j]);
	    }

	    lua_pop (_L, 1);
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

	    dJointSetAMotorParam (self->joint,
				  dParamCFM + dParamGroup * j,
				  self->tolerance[j]);
	}
    }
}

-(void) _set_angles
{
    T_WARN_READONLY;
}

-(void) _set_rates
{
    T_WARN_READONLY;
}

@end
