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
#include "slider.h"

@implementation Slider

-(void) init
{
    self->joint = dJointCreateSlider (_WORLD, NULL);
    dJointSetSliderAxis (self->joint, 0, 0, 1);

    self->stops[0] = -dInfinity;
    self->stops[1] = dInfinity;

    self->axis[0] = 0;
    self->axis[1] = 0;
    self->axis[2] = 1;
    
    [super init];
}

-(void) update
{
    if (dJointGetBody (self->joint, 0) ||
	dJointGetBody (self->joint, 1)) {
	dJointGetSliderAxis (self->joint, self->axis);
    }
    
    [super update];

    /* Reset the axis to update the initial sider position. */
	
    dJointSetSliderAxis (self->joint,
			 self->axis[0],
			 self->axis[1],
			 self->axis[2]);
}

-(int) _get_axis
{
    dJointGetSliderAxis (self->joint, self->axis);
    array_createarray (_L, ARRAY_TDOUBLE, self->axis, 1, 3);

    return 1;
}

-(int) _get_motor
{
    int i;

    lua_newtable (_L);
        
    for(i = 0; i < 2; i += 1) {
	lua_pushnumber (_L, self->motor[i]);
	lua_rawseti (_L, -2, i + 1);
    }

    return 1;
}

-(int) _get_stops
{
    int i;

    lua_newtable (_L);

    lua_newtable (_L);
    for(i = 0; i < 2; i += 1) {
	lua_pushnumber (_L, self->stops[i]);
	lua_rawseti (_L, -2, i + 1);
    }
    lua_rawseti (_L, -2, 1);
	
    lua_newtable (_L);
    for(i = 0; i < 2; i += 1) {
	lua_pushnumber (_L, self->hardness[i]);
	lua_rawseti (_L, -2, i + 1);
    }
    lua_rawseti (_L, -2, 2);

    lua_pushnumber (_L, self->bounce);
    lua_rawseti (_L, -2, 3);

    return 1;
}

-(int) _get_tolerance
{
    lua_pushnumber (_L, self->tolerance);

    return 1;
}

-(int) _get_fudge
{
    lua_pushnumber (_L, self->fudge);

    return 1;
}

-(int) _get_position
{
    if (self->joint) {
        lua_pushnumber (_L, dJointGetSliderPosition (self->joint));
    } else {
	lua_pushnil (_L);
    }

    return 1;
}

-(int) _get_rate
{
    if (self->joint) {
        lua_pushnumber (_L, dJointGetSliderPositionRate (self->joint));
    } else {
	lua_pushnil (_L);
    }

    return 1;
}

-(void) _set_axis
{
    array_Array *array;
    int i;
    
    array = array_checkcompatible (_L, 3,
                                   ARRAY_TYPE | ARRAY_RANK | ARRAY_SIZE,
                                   ARRAY_TDOUBLE, 1, 3);
    
    dSafeNormalize3 (array->values.doubles);
    dJointSetSliderAxis (self->joint,
			 array->values.doubles[0],
			 array->values.doubles[1],
			 array->values.doubles[2]);

    for (i = 0 ; i < 3 ; i += 1) {
	self->axis[i] = array->values.doubles[i];
    }
}

-(void) _set_motor
{
    int i;

    if(lua_istable (_L, 3)) {
	for(i = 0 ; i < 2 ; i += 1) {
	    lua_rawgeti (_L, 3, i + 1);
	    self->motor[i] = lua_tonumber (_L, -1);
                
	    lua_pop (_L, 1);
	}

	dJointSetSliderParam (self->joint, dParamVel, self->motor[0]);
	dJointSetSliderParam (self->joint, dParamFMax, self->motor[1]);
    }
}

-(void) _set_stops
{
    int i;

    /* Resetting the self->stops makes sure that lo remains
       smaller than hi between calls. */

    dJointSetSliderParam (self->joint, dParamLoStop, -dInfinity);
    dJointSetSliderParam (self->joint, dParamHiStop, dInfinity);
	
    if(lua_istable (_L, 3)) {
        double erp, cfm;        
        
	lua_rawgeti (_L, 3, 1);
	for(i = 0 ; i < 2 ; i += 1) {
	    lua_rawgeti (_L, -1, i + 1);
		
	    self->stops[i] = lua_tonumber (_L, -1);
		
	    lua_pop (_L, 1);
	}
	lua_pop (_L, 1);

	lua_rawgeti (_L, 3, 2);
	for(i = 0 ; i < 2 ; i += 1) {
	    lua_rawgeti (_L, -1, i + 1);

	    self->hardness[i] = lua_tonumber (_L, -1);

	    lua_pop (_L, 1);
	}
	lua_pop (_L, 1);
	
	lua_rawgeti (_L, 3, 3);
	self->bounce = lua_tonumber (_L, -1);
	lua_pop (_L, 1);

	dJointSetSliderParam (self->joint, dParamLoStop,
			      self->stops[0]);
	dJointSetSliderParam (self->joint, dParamHiStop,
			      self->stops[1]);

        t_convert_spring(self->hardness[0],
                         self->hardness[1],
                         &erp, &cfm);
  
	dJointSetSliderParam (self->joint, dParamStopCFM,
			      self->hardness[0]);
	dJointSetSliderParam (self->joint, dParamStopERP,
			      self->hardness[1]);
        
	dJointSetSliderParam (self->joint, dParamBounce, self->bounce);
    }
}

-(void) _set_fudge
{
    self->fudge = lua_tonumber (_L, 3);

    dJointSetSliderParam (self->joint, dParamFudgeFactor, self->fudge);
}

-(void) _set_tolerance
{
    self->tolerance = lua_tonumber (_L, 3);

    dJointSetSliderParam (self->joint, dParamCFM, self->tolerance);
}

-(void) _set_position
{
    T_WARN_READONLY;
}

-(void) _set_rate
{
    T_WARN_READONLY;
}

@end
