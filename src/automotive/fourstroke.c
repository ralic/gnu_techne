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

#include "gl.h"

#include "../array/array.h"
#include "techne.h"
#include "dynamics.h"
#include "body.h"
#include "fourstroke.h"

@implementation Fourstroke

-(void) init
{
    self->joint = dJointCreateHinge (_WORLD, NULL);
    
    self->anchor[0] = 0;
    self->anchor[1] = 0;
    self->anchor[2] = 0;

    self->axis[0] = 0;
    self->axis[1] = 0;
    self->axis[2] = 1;
    
    [super init];

    self->benchspeed = -1;
    self->throttle = 0;
    self->displacement = 0.000250;
    self->cylinders = 4;
    self->bypass = 0.01;
    self->idle = 1;
    self->spark = 1;
}

-(void) update
{
    [super update];
    
    /* Axis and anchor should be set after the
       joint has been attached. */
	
    dJointSetHingeAxis (self->joint,
			self->axis[0],
			self->axis[1],
			self->axis[2]);

    dJointSetHingeAnchor (self->joint,
			  self->anchor[0],
			  self->anchor[1],
			  self->anchor[2]);
}

-(void) cycle
{
    const double H_0 = 45e6, R_a = 286.9, phi_0 = 5 * M_PI / 180;

    double Q_atr, Q_atr0, Q_ac, Q_ac0, gamma_t, A;
    double omega/* , theta */, M, M_ind, M_loss;
    double p_me0g, p_me0f, p_m0, p_m1, p_mm;
    double T_m = 273 + 30, T_0 = 273 + 20, p_0 = 1e5;
    double phi = (85 * (self->throttle + self->bypass)) * M_PI / 180;
    double eta_t, eta_v, eta_v0, z = self->cylinders;
    int i;

/*   printf ("%f, %f\n", eta_v, eta_t); */
    
    if (self->benchspeed >= 0) {
	omega = self->benchspeed;

	self->benchspeed = -1;
    } else {
	/* theta = dJointGetHingeAngle (self->joint); */
	omega = dJointGetHingeAngleRate (self->joint);
    }

    if (omega < 1e-3) {
	omega = 1e-3;
    }

    /* Calculate a few constant values. */

    A = M_PI_4 * self->intake[0] * self->intake[0] *
	(1 - cos (phi + phi_0) / cos(phi_0));
    Q_atr0 = self->intake[1] * z * A * p_0 / sqrt (R_a * T_0) * 0.68473;
    Q_ac0 = 0.5 * z * self->displacement * omega / (2 * M_PI) / (R_a * T_m);

    eta_v0 = self->volumetric[0] +
	self->volumetric[1] * omega +
	self->volumetric[2] * omega * omega;

    eta_t = self->thermal[0] +
	self->thermal[1] * omega +
	self->thermal[2] * omega * omega;

    /* Solve for the manifold pressure that equalises 
       the air mass flows into and out of the manifold.
       The function is monotonically decreasing, therefore:
     
       f(0) * f(p_0) < 1 and f(0) > f(p_0)

       The bisection method is used which is guaranteed to
       converge with a tolerance of 100 Pa after 10 iterations. */
  
    for (i = 0, p_m0 = 0, p_m1 = p_0 ; i < 10 ; i += 1) {
	p_mm = 0.5 * (p_m0 + p_m1);

	eta_v = eta_v0 + self->volumetric[3] * p_mm;
	gamma_t = 1.8929 * p_mm / p_0;

	if (gamma_t > 1) {
	    Q_atr = Q_atr0 * 2.4495 * sqrt(pow(gamma_t, 1.4286) -
					   pow (gamma_t, 1.7143) / 1.2);
	} else {
	    Q_atr = Q_atr0;
	}

	Q_ac = Q_ac0 * eta_v * p_mm;

	if (Q_atr > Q_ac) {
	    p_m0 = p_mm;
	} else {
	    p_m1 = p_mm;
	}
    }

    /* Calculate the induced torque. */

    M_ind = self->spark * eta_t * H_0 * Q_ac / 14.7 / omega;

    /* Losses due to friction and gas-exchange according to
       Introduction to Modeling and Control of IC Engine Systems
       pp. 68. */

    p_me0g = self->exchange[0] * (1 - self->exchange[1] * p_mm / p_0);
    p_me0f = self->friction[0] +
	self->friction[1] * omega +
	self->friction[2] * omega * omega;

    if (omega > 0) {
	M_loss = (p_me0g + p_me0f) * z * self->displacement / (4 * M_PI);
    } else {
	M_loss = 0;
    }

    M = M_ind - M_loss;

    /* Apply produced torque to the crankshaft. */
    
    if (fabs(M) > 0) {
	dJointSetHingeParam (self->joint, dParamVel, M / 0);
	dJointSetHingeParam (self->joint, dParamFMax, fabs(M));
    } else {
	dJointSetHingeParam (self->joint, dParamVel, 0);
	dJointSetHingeParam (self->joint, dParamFMax, 0);
    }

    self->state[2] = p_mm;
    self->state[3] = eta_v;
    self->state[4] = eta_t;
    self->state[5] = M_ind;
    self->state[6] = p_me0g * z * self->displacement / (4 * M_PI);
    self->state[7] = p_me0f * z * self->displacement / (4 * M_PI);
    self->state[8] = M;

/*   printf ("%f, %f, %f, %f;\n", */
/* 	  omega / 2 / M_PI * 60, */
/* 	  q * M_ind * omega / 745.699872, */
/* 	  q * M_loss * omega / 745.699872, */
/* 	  q * M * omega / 745.699872); */
}

-(void) stepBy: (double) h at: (double) t
{
    [self cycle];
    [super stepBy: h at: t];
}

-(int) _get_anchor
{
    dJointGetHingeAnchor (self->joint, self->anchor);
    array_createarray (_L, ARRAY_TDOUBLE, self->anchor, 1, 3);

    return 1;
}

-(int) _get_axis
{
    dJointGetHingeAxis (self->joint, self->axis);
    array_createarray (_L, ARRAY_TDOUBLE, self->axis, 1, 3);

    return 1;
}

-(int) _get_tolerance
{
    lua_pushnumber (_L, self->tolerance);

    return 1;
}

-(int) _get_spark {
    lua_pushnumber(_L, self->spark);

    return 1;
}

-(int) _get_throttle
{
    lua_pushnumber(_L, self->throttle);

    return 1;
}

-(int) _get_displacement
{
    lua_pushnumber (_L, self->displacement);

    return 1;
}

-(int) _get_cylinders
{
    lua_pushinteger (_L, self->cylinders);

    return 1;
}

-(int) _get_intake
{
    int i;
    
    lua_newtable (_L);
        
    for(i = 0; i < 2; i += 1) {
	lua_pushnumber (_L, self->intake[i]);
	lua_rawseti (_L, -2, i + 1);
    }      

    return 1;
}

-(int) _get_volumetric
{
    int i;
    
    lua_newtable (_L);
        
    for(i = 0; i < 4; i += 1) {
	lua_pushnumber (_L, self->volumetric[i]);
	lua_rawseti (_L, -2, i + 1);
    }      

    return 1;
}

-(int) _get_thermal
{
    int i;
    
    lua_newtable (_L);
        
    for(i = 0; i < 3; i += 1) {
	lua_pushnumber (_L, self->thermal[i]);
	lua_rawseti (_L, -2, i + 1);
    }

    return 1;
}

-(int) _get_friction
{
    int i;
    
    lua_newtable (_L);
        
    for(i = 0; i < 3; i += 1) {
	lua_pushnumber (_L, self->friction[i]);
	lua_rawseti (_L, -2, i + 1);
    }

    return 1;
}

-(int) _get_exchange
{
    int i;
    
    lua_newtable (_L);
        
    for(i = 0; i < 2; i += 1) {
	lua_pushnumber (_L, self->exchange[i]);
	lua_rawseti (_L, -2, i + 1);
    }

    return 1;
}

-(int) _get_angle
{
    if (self->joint) {
        lua_pushnumber (_L, dJointGetHingeAngle (self->joint));
    } else {
	lua_pushnil (_L);
    }

    return 1;
}

-(int) _get_rate
{
    if (self->joint) {
        lua_pushnumber (_L, dJointGetHingeAngleRate (self->joint));
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
    dJointSetHingeAxis (self->joint,
			array->values.doubles[0],
			array->values.doubles[1],
			array->values.doubles[2]);

    for (i = 0 ; i < 3 ; i += 1) {
	self->axis[i] = array->values.doubles[i];
    }
}

-(void) _set_anchor
{
    array_Array *array;
    int i;
    
    array = array_checkcompatible (_L, 3,
                                   ARRAY_TYPE | ARRAY_RANK | ARRAY_SIZE,
                                   ARRAY_TDOUBLE, 1, 3);

    dJointSetHingeAnchor (self->joint,
			  array->values.doubles[0],
			  array->values.doubles[1],
			  array->values.doubles[2]);

    for (i = 0 ; i < 3 ; i += 1) {
	self->anchor[i] = array->values.doubles[i];
    }
}

-(void) _set_tolerance
{
    self->tolerance = lua_tonumber (_L, 3);

    dJointSetHingeParam (self->joint, dParamCFM, self->tolerance);
}

-(void) _set_spark
{
    self->spark = lua_toboolean (_L, 3);
}

-(void) _set_throttle
{
    self->throttle = lua_tonumber(_L, 3);

    if (self->throttle < 0) {
	self->throttle = 0;
    }

    if (self->throttle > 1) {
	self->throttle = 1;
    }
}

-(void) _set_displacement
{
    self->displacement = lua_tonumber (_L, 3);
}

-(void) _set_cylinders
{
    self->cylinders = lua_tointeger (_L, 3);
}

-(void) _set_angle
{
    T_WARN_READONLY;
}

-(void) _set_rate
{
    if (!lua_isnil (_L, 3)) {
	self->benchspeed = lua_tonumber (_L, 3);
	    
	[self cycle];
    }
}

-(void) _set_intake
{
    int i;
    
    if(!lua_isnil (_L, 3)) {
	for(i = 0 ; i < 2 ; i += 1) {
	    lua_pushinteger (_L, i + 1);
	    lua_gettable (_L, 3);
	    self->intake[i] = lua_tonumber (_L, -1);
                
	    lua_pop (_L, 1);
	}
    }
}

-(void) _set_volumetric
{
    int i;
    
    if(!lua_isnil (_L, 3)) {
	for(i = 0 ; i < 4 ; i += 1) {
	    lua_pushinteger (_L, i + 1);
	    lua_gettable (_L, 3);
	    self->volumetric[i] = lua_tonumber (_L, -1);
                
	    lua_pop (_L, 1);
	}
    }
}

-(void) _set_thermal
{
    int i;
    
    if(!lua_isnil (_L, 3)) {
	for(i = 0 ; i < 3 ; i += 1) {
	    lua_pushinteger (_L, i + 1);
	    lua_gettable (_L, 3);
	    self->thermal[i] = lua_tonumber (_L, -1);
                
	    lua_pop (_L, 1);
	}
    }
}

-(void) _set_friction
{
    int i;
    
    if(!lua_isnil (_L, 3)) {
	for(i = 0 ; i < 3 ; i += 1) {
	    lua_pushinteger (_L, i + 1);
	    lua_gettable (_L, 3);
	    self->friction[i] = lua_tonumber (_L, -1);
                
	    lua_pop (_L, 1);
	}
    }
}

-(void) _set_exchange
{
    int i;
    
    if(!lua_isnil (_L, 3)) {
	for(i = 0 ; i < 2 ; i += 1) {
	    lua_pushinteger (_L, i + 1);
	    lua_gettable (_L, 3);
	    self->exchange[i] = lua_tonumber (_L, -1);
                
	    lua_pop (_L, 1);
	}
    }
}

@end
