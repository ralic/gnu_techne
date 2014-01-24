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

#include "../meteorology/meteorology.h"
#include "techne.h"
#include "array/array.h"
#include "dynamics.h"
#include "airplane.h"
#include "body.h"

static char *denominators[] = {"attack", "sideslip",
			       "attackrate", "sidesliprate",
			       "roll", "pitch", "yaw",
			       "ailerons", "elevators", "rudder"};

static double lookup (double x, double *values, int length)
{
    double *a, *b;
    int k;
    
    if (length > 0) {
	for(k = 0, a = values, b = a + 2;
	    k < length - 4 && b[0] <= x ;
	    k += 2, a = b, b += 2);
	
	return a[1] + (b[1] - a[1]) / (b[0] - a[0]) * (x - a[0]);
    } else {
	return 0;
    }
}

@implementation Airplane

-(Airplane *) init
{
    int i, j;
    
    self->controls[0] = 0;
    self->controls[1] = 0;
    self->controls[2] = 0;

    self->area = 1;
    self->span = 1;
    self->chord = 1;

    self->alpha_0 = 0;
    self->beta_0 = 0;

    for(i = 0 ; i < 6 ; i += 1) {
	self->derivatives[i].reference = 0;
	
	for(j = 0 ; j < 10 ; j += 1) {
	    self->derivatives[i].lengths[j] = 0;
	    self->derivatives[i].values[j] = NULL;
	}
    }

    /* Create the motors. */
    
    self->joint = dJointCreateAMotor (_WORLD, NULL);
    dJointSetAMotorNumAxes (self->joint, 3);
 
    self->lmotor = dJointCreateLMotor (_WORLD, NULL);
    dJointSetLMotorNumAxes (self->lmotor, 3);
    dJointSetFeedback (self->lmotor, &self->feedback_1);
    dJointSetData (self->lmotor, self);
   
    [super init];

    return self;
}

-(void) free
{  
    dJointDestroy (self->joint);
    dJointDestroy (self->lmotor);
  
    [super free];
}

-(void) update
{
    const dReal *R;

    [super update];

    if (self->inverted) {
        t_print_error("Airplane joint should not be connected inverted.\n");
        abort();
    }

    /* Attach additional motors to the parent body. */

    dJointAttach (self->lmotor, self->bodies[0], self->bodies[1]);

    if (dJointGetBody(self->joint, 0)) {
        R = dBodyGetRotation(self->bodies[0]);
        
        /* Set the motor's reference frames. */
	
        dJointSetAMotorAxis (self->joint, 0, 1, R[0], R[4], R[8]);
        dJointSetAMotorAxis (self->joint, 1, 1, R[1], R[5], R[9]);
        dJointSetAMotorAxis (self->joint, 2, 1, R[2], R[6], R[10]);
	
        dJointSetLMotorAxis (self->lmotor, 0, 1, R[0], R[4], R[8]);
        dJointSetLMotorAxis (self->lmotor, 1, 1, R[1], R[5], R[9]);
        dJointSetLMotorAxis (self->lmotor, 2, 1, R[2], R[6], R[10]);
    }
}
  
-(int) _get_forces
{
    lua_newtable (_L);

    /* The force applied on the first body. */
	
    array_createarray (_L, ARRAY_TDOUBLE,
                       self->feedback_1.f1, 1, 3);

    lua_rawseti (_L, -2, 1);

    /* The force applied on the second body. */
	
    array_createarray (_L, ARRAY_TDOUBLE,
                       self->feedback_1.f2, 1, 3);

    lua_rawseti (_L, -2, 2);

    return 1;
}
  
-(void) getDerivative: (int)k
{
    int i, j, n;
    
    for(i = 0 ; i < 10 && self->derivatives[k].lengths[i] == 0 ; i += 1);
    
    if (i < 10) {
	lua_newtable(_L);

	lua_pushstring(_L, "reference");
	lua_pushnumber(_L, self->derivatives[k].reference);
	lua_settable(_L, -3);
	    
	for(i = 0 ; i < 10 ; i += 1) {
	    n = self->derivatives[k].lengths[i];
		
	    if(n > 0) { 
		lua_pushstring(_L, denominators[i]);
		lua_newtable(_L);

		for (j = 0 ; j < n ; j += 1) {
		    lua_pushnumber(_L, self->derivatives[k].values[i][j]);
		    lua_rawseti(_L, -2, j + 1);
		}

		lua_settable(_L, -3);
	    }
	}
    } else {
	lua_pushnil (_L);
    }
}

-(void) setDerivative: (int)k
{
    int i, j, n;

    self->derivatives[k].reference = 0;
    
    for(i = 0 ; i < 10 ; i += 1) {
	if (self->derivatives[k].lengths[i] > 0) {
	    free (self->derivatives[k].values[i]);

	    self->derivatives[k].lengths[i] = 0;
	    self->derivatives[k].values[i] = NULL;
	}
    }
    
    if (lua_istable (_L, -1)) {
	lua_pushstring(_L, "reference");
	lua_gettable(_L, -2);
    
	self->derivatives[k].reference = lua_tonumber(_L, -1);

	lua_pop(_L, 1);
    
	for(i = 0 ; i < 10 ; i += 1) {
	    lua_pushstring(_L, denominators[i]);
	    lua_gettable(_L, -2);
		    
	    n = lua_rawlen(_L, -1);

	    self->derivatives[k].lengths[i] = n;
	    self->derivatives[k].values[i] =
		(double *)calloc(n, sizeof(double));

	    for (j = 0 ; j < n ; j += 1) {
		lua_rawgeti(_L, -1, j + 1);
		self->derivatives[k].values[i][j] = lua_tonumber(_L, -1);

		lua_pop(_L, 1);
	    }

	    lua_pop(_L, 1);
	}
    }
}

-(int) _get_area
{
    lua_pushnumber(_L, self->area);

    return 1;
}

-(int) _get_span
{
    lua_pushnumber(_L, self->span);

    return 1;
}

-(int) _get_chord
{
    lua_pushnumber(_L, self->chord);

    return 1;
}

-(int) _get_ailerons
{
    lua_pushnumber(_L, self->controls[0]);

    return 1;
}

-(int) _get_elevators
{
    lua_pushnumber(_L, self->controls[1]);

    return 1;
}

-(int) _get_rudder
{
    lua_pushnumber(_L, self->controls[2]);

    return 1;
}

-(int) _get_drag
{
    [self getDerivative:  0];

    return 1;
}

-(int) _get_sideforce
{
    [self getDerivative:  1];

    return 1;
}

-(int) _get_lift
{
    [self getDerivative:  2];

    return 1;
}

-(int) _get_roll
{
    [self getDerivative:  3];

    return 1;
}

-(int) _get_pitch
{
    [self getDerivative:  4];

    return 1;
}

-(int) _get_yaw
{
    [self getDerivative:  5];

    return 1;
}

-(void) _set_area
{
    self->area = lua_tonumber(_L, 3);
}

-(void) _set_span
{
    self->span = lua_tonumber(_L, 3);
}

-(void) _set_chord
{
    self->chord = lua_tonumber(_L, 3);
}

-(void) _set_ailerons
{
    self->controls[0] = lua_tonumber (_L, 3);
}

-(void) _set_elevators
{
    self->controls[1] = lua_tonumber (_L, 3);
}

-(void) _set_rudder
{
    self->controls[2] = lua_tonumber (_L, 3);
}

-(void) _set_drag
{
    [self setDerivative:  0];
}

-(void) _set_sideforce
{
    [self setDerivative:  1];
}

-(void) _set_lift
{
    [self setDerivative:  2];
}

-(void) _set_roll
{
    [self setDerivative:  3];
}

-(void) _set_pitch
{
    [self setDerivative:  4];
}

-(void) _set_yaw
{
    [self setDerivative:  5];
}

-(void) stepBy: (double) h at: (double) t
{
    const dReal *r_H, *v_H, *omega_H;
    dVector3 uvw, pqr;
    
    double rho;
    double alpha, beta, alphadot, betadot, oneoverV, Vsquared;
    double b, cbar, S, bover2V, cbarover2V, qbarS;
    double delta_a, delta_e, delta_r;
    double C[6], F[3], Tau[3];

    int i;
    
    /* Parameters. */

    delta_a = self->controls[0];
    delta_e = self->controls[1];
    delta_r = self->controls[2];

    b = self->span;
    cbar = self->chord;
    S = self->area;

    /* State. */
    
    r_H = dBodyGetPosition(self->bodies[0]);
    v_H = dBodyGetLinearVel(self->bodies[0]);
    omega_H = dBodyGetAngularVel(self->bodies[0]);

    rho = get_density_at (r_H[2]);
    
    /* Now to calculate all needed scalar quantities. */
    
    Vsquared = dDOT(v_H, v_H);

    dBodyVectorFromWorld (self->bodies[0], v_H[0], v_H[1], v_H[2], uvw);
    dBodyVectorFromWorld (self->bodies[0], omega_H[0], omega_H[1], omega_H[2], pqr);

    /* Perform an additional rotation around the x-axis since
       we want our local frame to have an upwards pointing
       z-axis.  The local frame is usually defined to have a
       downwards pointing z-axis in most aerodynamics textbooks. */

    uvw[1] *= -1; uvw[2] *= -1;
    pqr[1] *= -1; pqr[2] *= -1;

    /* Compute derived parameters. */
    
    alpha = atan2 (uvw[2], uvw[0]);
    beta  = atan2 (uvw[1], uvw[0]);

    alphadot = (alpha - alpha_0) / h;
    betadot = (beta - beta_0) / h;
    
    oneoverV = 1 / sqrt(Vsquared);
    
    qbarS = 0.5 * rho * Vsquared * S;
    bover2V = 0.5 * b * oneoverV;
    cbarover2V = 0.5 * cbar * oneoverV;

    /* And now for our ultimate goal: calculating
       the aerodynamic force and torque coefficients.
       
       0 - Drag, 1 - Sideforce, 2 - Lift,
       3 - Roll, 4 - Pitch,     5 - Yaw
    */

    for(i = 0 ; i < 6 ; i += 1) {
	C[i] = (self->derivatives[i].reference + 
		lookup (alpha,
			self->derivatives[i].values[0],
			self->derivatives[i].lengths[0]) +
		lookup (beta,
			self->derivatives[i].values[1],
			self->derivatives[i].lengths[1]) +
		(lookup (pqr[0],
			 self->derivatives[i].values[4],
			 self->derivatives[i].lengths[4]) +
		 lookup (pqr[2],
			 self->derivatives[i].values[6],
			 self->derivatives[i].lengths[6]) +
		 lookup (betadot,
			 self->derivatives[i].values[3],
			 self->derivatives[i].lengths[3])) * bover2V +
		(lookup (pqr[1],
			 self->derivatives[i].values[5],
			 self->derivatives[i].lengths[5]) +
		 lookup (alphadot,
			 self->derivatives[i].values[2],
			 self->derivatives[i].lengths[2])) * cbarover2V +
		lookup (delta_a,
			self->derivatives[i].values[7],
			self->derivatives[i].lengths[7]) +
		lookup (delta_e,
			self->derivatives[i].values[8],
			self->derivatives[i].lengths[8]) +
		lookup (delta_r,
			self->derivatives[i].values[9],
			self->derivatives[i].lengths[9]));
    }

    self->alpha_0 = alpha;
    self->beta_0 = beta;

    /* Transform the forces from the wind
       frame to the body-axis frame. */

    F[0] = (C[2] * sin(alpha) -
		      C[0] * cos(alpha) -
		      C[1] * sin(beta)) * qbarS;
    F[1] = C[1] * cos(beta) * qbarS;
    F[2] = -(C[2] * cos(alpha) + C[0] * sin(alpha)) * qbarS;
    
    /* Transform the moments to the local frame. */

    Tau[0] = C[3] * b * qbarS;
    Tau[1] = C[4] * cbar * qbarS;
    Tau[2] = C[5] * b * qbarS;

    /* Rotate the results around the x axis to align
       the body-axis frame's z axis with the local
       frame's z axis. */

    F[1] *= -1; F[2] *= -1;
    Tau[1] *= -1; Tau[2] *= -1;

    /* _TRACEV (3, "f", F); */
    /* _TRACEV (3, "f", Tau); */
    /* _TRACEF (sqrt(Vsquared)); */

    /* dBodyAddTorque(self->bodies[0], self->torque[0], self->torque[1], self->torque[2]); */

    for(i = 0 ; i < 3 ; i += 1) {
    	dJointSetAMotorParam (self->joint, dParamVel + dParamGroup * i,
    			      Tau[i] > 0 ? dInfinity : -dInfinity);
		    
    	dJointSetAMotorParam (self->joint, dParamFMax + dParamGroup * i,
    			      fabs(Tau[i]));

    	dJointSetLMotorParam (self->lmotor, dParamVel + dParamGroup * i,
    			      F[i] > 0 ? dInfinity : -dInfinity);
		    
    	dJointSetLMotorParam (self->lmotor, dParamFMax + dParamGroup * i,
    			      fabs(F[i]));
    }
    
    [super stepBy: h at: t];
}

/* -(void)prepare */
/* { */
/*     printf ("%f: %f, %f, %f\n", */
/* 	    self->position[2], */
/* 	    get_tempreture_at (self->position[2]), */
/* 	    get_pressure_at (self->position[2]), */
/* 	    get_density_at (self->position[2])); */

/*      printf ("%f, %f, %f\n", */
/* 	    self->position[0], */
/* 	    self->position[1], */
/* 	    self->position[2]); */

/*     [super prepare]; */
/* } */

@end
