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

#include "techne.h"
#include "dynamics.h"
#include "wheel.h"

#define sign(x) ((x) >= 0 ? 1.0 : -1.0)

static dColliderFn * getCollider (int num)
{
    return 0;
}

@implementation Wheel

-(void) init
{
    struct wheeldata *data;
    int i;

    if (!dWheelClass) {
	struct dGeomClass class = {
	    sizeof (struct wheeldata),
	    getCollider,
	    dInfiniteAABB,
	    0, 0
	};
      
	dWheelClass = dCreateGeomClass (&class);
    }

    self->geom = dCreateGeom (dWheelClass);
    dGeomSetData (self->geom, self);

    data = dGeomGetClassData (self->geom);

    data->radii[0] = 0.297 - 0.09;
    data->radii[1] = 0.09;

    data->elasticity[0] = 141e3;
    data->elasticity[1] = 1500;

    data->airborne = 1;
    
    self->damper = dJointCreateAMotor (_WORLD, NULL);

    self->F_x = 0;
    self->F_y = 0;
    self->M_z = 0;
    
    self->F_z0 = 1100;

    self->C_x = 1.6064;
    self->p_Dx1 = 1.2017;
    self->p_Dx2 = -0.0922;
    self->p_Ex1 = 0.0263;
    self->p_Ex2 = 0.27056;
    self->p_Ex3 = -0.0769;
    self->p_Ex4 = 1.1268;
    self->p_Kx1 = 25.94;
    self->p_Kx2 = -4.233;
    self->p_Kx3 = 0.3369;

    self->r_Bx1 = 13.476;
    self->r_Bx2 = 11.354;
    self->C_xalpha = 1.1231;

    self->C_y = 0.8327;
    self->p_Dy1 = 1.3;
    self->p_Dy2 = 0;
    self->p_Dy3 = 0;
    self->p_Ey1 = -1.2556;  
    self->p_Ey2 = -3.2068;
    self->p_Ey4 = -3.998;
    self->p_Ky1 = 22.841;
    self->p_Ky2 = 2.1578;
    self->p_Ky3 =  2.5058;
    self->p_Ky4 =  -0.08088;
    self->p_Ky5 =  -0.22882;
    self->C_gamma = 0.86765;
    self->p_Ky6 =  0.69677;
    self->p_Ky7 = -0.03077;
    self->E_gamma = -15.815;

    self->r_By1 = 7.7856;
    self->r_By2 = 8.1697;
    self->r_By3 = -0.05914;
    self->C_ykappa = 1.0533;

    self->C_t = 1.0917;
    self->q_Bz1 = 10.486;
    self->q_Bz2 = -0.001154;
    self->q_Bz5 = -0.68973;
    self->q_Bz6 = 1.0411;
    self->q_Bz9 = 27.445;
    self->q_Bz10 = -1.0792;
    self->q_Dz1 = 0.19796;
    self->q_Dz2 = 0.06563;
    self->q_Dz3 = 0.2199;
    self->q_Dz4 = 0.21866;
    self->q_Dz8 = 0.3682;
    self->q_Dz9 = 0.1218;
    self->q_Dz10 = 0.25439;
    self->q_Dz11 = -0.17873;
    self->q_Ez1 = -0.91586;
    self->q_Ez2 = 0.11625;
    self->q_Ez3 = -0.0024085;
    self->q_Ez5 = 1.4387;
    self->q_Hz3 = -0.003789;
    self->q_Hz4 = -0.01557;

    self->resistance = 0.015;

    for (i = 0 ; i < 10 ; i += 1) {
	data->lambda[i] = 1;
    }
    
    [super init];
}

-(void) release
{
    dVector3 a;
    
    [super release];

    dBodyVectorToWorld (self->body, 0, 1, 0, a);
    
    dBodySetFiniteRotationMode (self->body, 1);
    dBodySetGyroscopicMode (self->body, 1);
    
    dJointAttach (self->damper, self->body, 0);
    dJointSetAMotorMode (self->damper, dAMotorUser);
    dJointSetAMotorNumAxes (self->damper, 1);
    dJointSetAMotorAxis (self->damper, 0, 1, a[0], a[1], a[2]);
}

-(void)evaluateWithStep: (double)h andFactors: (double[10])lambda
{
    double df_z, D_x, E_x, K_xkappa, B_x;
    double gamma_2, D_y, E_y, K_yalpha, B_y, K_ygamma, B_gamma;
    double D_y0, B_y0, K_yalpha0, F_y00;
    double beta_2, S_Hr, alpha_r, B_t, D_t, E_t, B_r, D_r, M_zt0, M_zr0;
    double B_xalpha, B_ykappa, G_ykappa;
    double lambda_r, lambda_t, M_zr, K_2;
    double sigma, V;
    int i;

    struct wheeldata *data;
    
    data = dGeomGetClassData (self->geom);
    V = dLENGTH (dBodyGetLinearVel (self->body));

    /* Combine track and wheel scaling factors. */

    for (i = 0 ; i < 10 ; i += 1) {
	lambda[i] *= data->lambda[i];
    }

    /* Make sure the load does not vanish completely. */
    
    if (self->F_z < 1e-3) {
	self->F_z = 1e-3;
    }
    
    /* if (self->tag == 1) */
    /* printf ("!! %f, %f, %f, %f, %f, %f\n", self->F_z, self->beta, self->beta_1, self->kappa, self->gamma, V); */

    /* printf ("%.2f, %.2f, %.2f, %.2f, %.2f, %.2f, %.2f\n", */
    /* 	    lambda[0], lambda[1], lambda[2], */
    /* 	    lambda[3], lambda[4], lambda[5], */
    /* 	    lambda[6]); */

    /* Pure longitudinal slip. */

    df_z = (self->F_z - self->F_z0) / self->F_z0;
    D_x = lambda[0] * (self->p_Dx1 + self->p_Dx2 * df_z) * self->F_z;
    E_x = (self->p_Ex1 + self->p_Ex2 * df_z + self->p_Ex3 * df_z * df_z) * (1 - self->p_Ex4 * sign(self->kappa));
    K_xkappa = lambda[2] * self->F_z * (self->p_Kx1 + self->p_Kx2 * df_z) * exp (self->p_Kx3 * df_z);
    B_x = K_xkappa / (self->C_x * D_x) * self->kappa;
    self->F_x0 = D_x * sin(self->C_x * atan(B_x - E_x * (B_x - atan (B_x))));

    /* Sideslip angle lag due to tire deformation. */

    K_yalpha0 = lambda[3] * self->p_Ky1 * self->F_z0 * sin (self->p_Ky2 * atan(self->F_z / (self->p_Ky3 * self->F_z0)));

    sigma = K_yalpha0 * (self->relaxation[0] +
			 self->relaxation[1] * V +
			 self->relaxation[2] * V * V);
    
    if (V == 0 || sigma < V * h || sigma <= 0 || h == 0) {
	self->beta_1 = self->beta;
    } else {
	self->beta_1 += (self->beta - self->beta_1) *
	    V / sigma * h;
    }

    /* Pure lateral slip. */
    
    gamma_2 = self->gamma * self->gamma;
    K_yalpha = lambda[3] * self->p_Ky1 * self->F_z0 * sin (self->p_Ky2 * atan(self->F_z / ((self->p_Ky3 + self->p_Ky4 * gamma_2) * self->F_z0))) / (1 + p_Ky5 * gamma_2);
    
    /* The paper by Sharp has a division by the last term but the
       Pacejka in section 10.6.1 of Tyre and Vehicle Dynamics has a
       multiplication.  The lisp code for Sharp's paper multiplies as
       well so we'll assume this is correct. */
    
    D_y = lambda[1] * self->F_z * self->p_Dy1 * exp (self->p_Dy2 * df_z) * (1 + self->p_Dy3 * gamma_2);
    E_y = self->p_Ey1 + self->p_Ey2 * gamma_2 + self->p_Ey4 * self->gamma * sign(self->beta_1);
    B_y = K_yalpha / (self->C_y * D_y);
    K_ygamma = lambda[4] * (self->p_Ky6 + self->p_Ky7 * df_z) * self->F_z;
    B_gamma = K_ygamma / (self->C_gamma * D_y) * self->gamma;

    self->F_y0 = D_y * sin (self->C_y * atan (B_y * self->beta_1 - E_y * (B_y * self->beta_1 - atan (B_y * self->beta_1))) + self->C_gamma * atan (B_gamma - self->E_gamma * (B_gamma - atan (B_gamma))));

    /* Pure lateral slip for zero camber. */

    D_y0 = lambda[1] * self->F_z * self->p_Dy1 * exp (self->p_Dy2 * df_z);
    B_y0 = K_yalpha0 / (self->C_y * D_y0) * self->beta_1;

    F_y00 = D_y0 * sin (self->C_y * atan (B_y0 - self->p_Ey1 * (B_y0 - atan (B_y0))));

    /* Aligning moment under pure sideslip and camber. */

    beta_2 = self->beta_1 * self->beta_1;
    S_Hr = (self->q_Hz3 + self->q_Hz4 * df_z) * self->gamma;
    alpha_r = self->beta_1 + S_Hr;
    B_t = (self->q_Bz1 + self->q_Bz2 * df_z) * (1 + self->q_Bz5 * fabs(self->gamma) + self->q_Bz6 * gamma_2) * lambda[3] / lambda[1];
    D_t = lambda[5] * self->F_z * (data->radii[1] / self->F_z0) * (self->q_Dz1 + self->q_Dz2 * df_z) * (1 + self->q_Dz3 * fabs(self->gamma) + self->q_Dz4 * gamma_2);
    E_t = (self->q_Ez1 + self->q_Ez2 * df_z + self->q_Ez3 * df_z * df_z) * (1 + self->q_Ez5 * self->gamma * (2 / M_PI) * atan(B_t * self->beta_1 * self->C_t));
    B_r = (self->q_Bz9 + self->q_Bz10 * B_y * self->C_y) * lambda[3] / lambda[1];
    D_r = lambda[1] * F_z * data->radii[1] * ((self->q_Dz8 + self->q_Dz9 * df_z) * self->gamma * lambda[6] + (self->q_Dz10 + self->q_Dz11 * df_z) * self->gamma * fabs(self->gamma)) / sqrt (1 + beta_2);

    /* These are not strictly necessary it seems
       but are included for completeness when plotting. */
    
    M_zt0 = -D_t * cos(self->C_t * atan(B_t * self->beta_1 - E_t * (B_t * self->beta_1 - atan(B_t * self->beta_1)))) / sqrt (1 + beta_2) * F_y00;
    M_zr0 = D_r * cos(atan(B_r * alpha_r));
    self->M_z0 = M_zt0 + M_zr0;

    /* Combinded slip forces. */

    B_xalpha = self->r_Bx1 * cos (atan (self->r_Bx2 * self->kappa));
    B_ykappa = self->r_By1 * cos (atan (self->r_By2 * (self->beta_1 - self->r_By3)));
    G_ykappa = cos (self->C_ykappa * atan (B_ykappa * self->kappa));
	
    self->F_x = cos (self->C_xalpha * atan (B_xalpha * self->beta_1)) * self->F_x0;
    self->F_y = G_ykappa * self->F_y0;

    /* Combinded slip moment. */

    K_2 = (K_xkappa * self->kappa / K_yalpha0);
    K_2 *= K_2;
    lambda_t = sqrt(beta_2 + K_2) * sign (self->beta_1);
    lambda_r = sqrt (alpha_r * alpha_r + K_2) * sign (alpha_r);
    M_zr = D_r * cos (atan (B_r * lambda_r));
    B_t *= lambda_t;
    self->M_z = -D_t * cos (self->C_t * atan (B_t - E_t * (B_t - atan (B_t)))) / sqrt (1 + beta_2) * G_ykappa * F_y00 + M_zr;
}

-(void) stepBy: (double) h at: (double) t
{
    struct wheeldata *data;
    dContact contact;
    dJointID joint;
    const dReal *r, *v, *omega;
    dVector3 p;
    dReal V_l, r_e;

    data = dGeomGetClassData (self->geom);

/*   if (self->tag == 1) { */
/*     printf ("%f, %f;\n", data->kappa, data->beta); */
/*   } */
	    
    /* Calculate slip ratios. */
    
    r = dBodyGetPosition (self->body);
    v = dBodyGetLinearVel (self->body);
    omega = dBodyGetAngularVel (self->body);

    V_l = dDOT (v, data->longitudinal);

    /* We need to displace the theoretical contact point
       by the depth along the normal to get the real */

    p[0] = data->contact.pos[0] + data->contact.normal[0] * data->contact.depth;
    p[1] = data->contact.pos[1] + data->contact.normal[1] * data->contact.depth;
    p[2] = data->contact.pos[2] + data->contact.normal[2] * data->contact.depth;

    r_e = (p[0] - r[0]) * data->radial[0] +
	(p[1] - r[1]) * data->radial[1] +
	(p[2] - r[2]) * data->radial[2];
    
    self->gamma = asin(dDOT(data->axial, data->contact.normal));

    /* The slip ratio kappa is the ratio of the difference between the
       spin velocity of the driven tire (V_l / r_e) and that of the
       free-rolling tire (omega) to the spin velocity of the
       free-rolling tire (according to SAE J670e). */
    
    self->kappa = (dDOT (omega, data->axial) * r_e - V_l) / fabs(V_l);

    /* Here beta is the tangent of the slip angle which is the input
       to the magic formula according to: Tyre and vehicle dynamics
       pp. 173 */
    
    self->beta = dDOT (v, data->lateral) / fabs(V_l);

    /* if (self->tag == 1) { */
    	/* printf ("%f, %f, %f, %f\n", self->kappa, self->beta, self->F_x, self->F_y); */
	/* printf ("%f\n", r_e); */
    /* } */

    /* Take care of singularities. */
    
    if (!isfinite (self->kappa)) {
	self->kappa = 0;
    }

    if (!isfinite (self->beta)) {
	self->beta = 0;
    }

    /* printf ("%f, %f, %f\n", V_l, self->kappa, self->beta); */

    if (!data->airborne) {
	double lambda[10] = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1};
	int airborne = 0;
	int i, h_0;

	/* Call the collision callback for the pair
	   of geoms and get the returned parameters. */
	
	h_0 = lua_gettop (_L);
		    
	lua_getglobal(_L, "dynamics");
	lua_getfield(_L, -1, "collision");
	lua_replace (_L, -2);
	
	if (!lua_isnil(_L, -1)) {
	    /* Get the userdata. */
	    
	    t_pushuserdata (_L, 2,
			     dGeomGetData(data->contact.g1),
			     dGeomGetData(data->contact.g2));

	    /* Call the hook. */
		    
	    lua_call (_L, 2, LUA_MULTRET);

	    if (lua_type (_L, h_0 + 1) == LUA_TNUMBER) {
		airborne = lua_tointeger (_L, h_0 + 1) == 0;
	    }

	    for (i = 0 ; i < 10 ; i += 1) {
		if (lua_type (_L, h_0 + 2 + i) == LUA_TNUMBER) {
		    lambda[i] = lua_tonumber (_L, h_0 + 2 + i);
		}
	    }
	}
		    
	lua_settop (_L, h_0);

	/* Recheck in case the user requested not contact. */
    
	if (!airborne) {
	    /* Create the contact joint. */
	
	    contact.geom = data->contact;

	    /* If the vehicle velocity is very low switch
	       to normal ode friction as it is more stable.*/
	
	    contact.surface.mode = dContactSoftERP | dContactSoftCFM;
	    if (fabs(V_l) <= 1e-1) {
		contact.surface.mode |= dContactApprox1;
		contact.surface.mu = 1;
	    } else {
		contact.surface.mu = 0;
	    }

	    {
		dReal k_s, k_d;
	    
		k_s = lambda[8] * data->elasticity[0];
		k_d = lambda[9] * data->elasticity[1];

		contact.surface.soft_cfm = 1.0 / (h * k_s + k_d);
		contact.surface.soft_erp = h * k_s / (h * k_s + k_d);
	    }
	
	    joint = dJointCreateContact (_WORLD, _GROUP, &contact);
	    dJointSetFeedback (joint, &feedback);
	    dJointAttach (joint,
			  dGeomGetBody (data->contact.g1),
			  dGeomGetBody (data->contact.g2));

	    /* if (self->tag == 1) { */
	    /*     fprintf (stderr, "%f, %f, %f\n", */
	    /* 	     t, contact.geom.pos[2], contact.geom.depth); */
	    /* } */

	    if (fabs(V_l) > 1e-1) {
		[self evaluateWithStep: h andFactors: lambda];

		dBodyAddForceAtPos (self->body,
				    data->longitudinal[0] * self->F_x,
				    data->longitudinal[1] * self->F_x,
				    data->longitudinal[2] * self->F_x,
				    p[0], p[1], p[2]);

		dBodyAddForceAtPos (self->body,
				    -data->lateral[0] * self->F_y,
				    -data->lateral[1] * self->F_y,
				    -data->lateral[2] * self->F_y,
				    p[0], p[1], p[2]);

		dBodyAddTorque (self->body,
				-data->contact.normal[0] * self->M_z,
				-data->contact.normal[1] * self->M_z,
				-data->contact.normal[2] * self->M_z);

		/* Set the rolling resistance torque. */
	
		dJointSetAMotorParam (self->damper, dParamFMax,
				      self->F_z * lambda[7] * self->resistance * r_e);
	    }
	} else {
	    dJointSetAMotorParam (self->damper, dParamFMax, 0);
	}
    }
    
    [super stepBy: h at: t];
}

-(void) transform
{
    struct wheeldata *data;

    data = dGeomGetClassData (self->geom);

    if (!data->airborne) {
	self->F_z = dDOT(feedback.f1, data->contact.normal);
    } else {
	self->F_z = 0;
    }

    /* printf ("%f, %f, %f\n", feedback.f1[0], feedback.f1[1], feedback.f1[2]); */
    /* printf ("Load: %f\n", self->F_z); */
    [super transform];
}

-(int) _get_elasticity
{
    struct wheeldata *data;

    data = dGeomGetClassData (self->geom);
      
    lua_newtable (_L);

    lua_pushnumber (_L, data->elasticity[0]);
    lua_rawseti (_L, -2, 1);

    lua_pushnumber (_L, data->elasticity[1]);
    lua_rawseti (_L, -2, 2);

    return 1;
}

-(int) _get_radii
{
    struct wheeldata *data;

    data = dGeomGetClassData (self->geom);
      
    lua_newtable (_L);

    lua_pushnumber (_L, data->radii[0]);
    lua_rawseti (_L, -2, 1);

    lua_pushnumber (_L, data->radii[1]);
    lua_rawseti (_L, -2, 2);

    return 1;
}

-(int) _get_load
{
    lua_pushnumber (_L, self->F_z0);

    return 1;
}

-(int) _get_longitudinal
{
    /* Parameters for pure and combined
       longitudinal slip. */
  
    lua_newtable (_L);
      
    lua_pushnumber (_L, self->C_x);
    lua_rawseti (_L, 3, 1);

    lua_pushnumber (_L, self->p_Dx1);
    lua_rawseti (_L, 3, 2);

    lua_pushnumber (_L, self->p_Dx2);
    lua_rawseti (_L, 3, 3);

    lua_pushnumber (_L, self->p_Ex1);
    lua_rawseti (_L, 3, 4);

    lua_pushnumber (_L, self->p_Ex2);
    lua_rawseti (_L, 3, 5);

    lua_pushnumber (_L, self->p_Ex3);
    lua_rawseti (_L, 3, 6);

    lua_pushnumber (_L, self->p_Ex4);
    lua_rawseti (_L, 3, 7);

    lua_pushnumber (_L, self->p_Kx1);
    lua_rawseti (_L, 3, 8);

    lua_pushnumber (_L, self->p_Kx2);
    lua_rawseti (_L, 3, 9);

    lua_pushnumber (_L, self->p_Kx3);
    lua_rawseti (_L, 3, 10);

    lua_pushnumber (_L, self->r_Bx1);
    lua_rawseti (_L, 3, 11);

    lua_pushnumber (_L, self->r_Bx2);
    lua_rawseti (_L, 3, 12);

    lua_pushnumber (_L, self->C_xalpha);
    lua_rawseti (_L, 3, 13);

    return 1;
}

-(int) _get_lateral
{
    /* Parameters for pure and combined
       lateral slip. */
   		
    lua_newtable (_L);

    lua_pushnumber (_L, self->C_y);
    lua_rawseti (_L, 3, 1);

    lua_pushnumber (_L, self->p_Dy1);
    lua_rawseti (_L, 3, 2);

    lua_pushnumber (_L, self->p_Dy2);
    lua_rawseti (_L, 3, 3);

    lua_pushnumber (_L, self->p_Dy3);
    lua_rawseti (_L, 3, 4);

    lua_pushnumber (_L, self->p_Ey1);
    lua_rawseti (_L, 3, 5);

    lua_pushnumber (_L, self->p_Ey2);
    lua_rawseti (_L, 3, 6);

    lua_pushnumber (_L, self->p_Ey4);
    lua_rawseti (_L, 3, 7);

    lua_pushnumber (_L, self->p_Ky1);
    lua_rawseti (_L, 3, 8);

    lua_pushnumber (_L, self->p_Ky2);
    lua_rawseti (_L, 3, 9);

    lua_pushnumber (_L, self->p_Ky3);
    lua_rawseti (_L, 3, 10);

    lua_pushnumber (_L, self->p_Ky4);
    lua_rawseti (_L, 3, 11);

    lua_pushnumber (_L, self->p_Ky5);
    lua_rawseti (_L, 3, 12);

    lua_pushnumber (_L, self->C_gamma);
    lua_rawseti (_L, 3, 13);

    lua_pushnumber (_L, self->p_Ky6);
    lua_rawseti (_L, 3, 14);

    lua_pushnumber (_L, self->p_Ky7);
    lua_rawseti (_L, 3, 15);

    lua_pushnumber (_L, self->E_gamma);
    lua_rawseti (_L, 3, 16);

    lua_pushnumber (_L, self->r_By1);
    lua_rawseti (_L, 3, 17);

    lua_pushnumber (_L, self->r_By2);
    lua_rawseti (_L, 3, 18);

    lua_pushnumber (_L, self->r_By3);
    lua_rawseti (_L, 3, 19);

    lua_pushnumber (_L, self->C_ykappa);
    lua_rawseti (_L, 3, 20);

    return 1;
}

-(int) _get_moment
{
    /* Parameters for the aligning moment. */

    lua_newtable (_L);

    lua_pushnumber (_L, self->C_t);
    lua_rawseti (_L, 3, 1);

    lua_pushnumber (_L, self->q_Bz1);
    lua_rawseti (_L, 3, 2);

    lua_pushnumber (_L, self->q_Bz2);
    lua_rawseti (_L, 3, 3);

    lua_pushnumber (_L, self->q_Bz5);
    lua_rawseti (_L, 3, 4);

    lua_pushnumber (_L, self->q_Bz6);
    lua_rawseti (_L, 3, 5);

    lua_pushnumber (_L, self->q_Bz9);
    lua_rawseti (_L, 3, 6);

    lua_pushnumber (_L, self->q_Bz10);
    lua_rawseti (_L, 3, 7);

    lua_pushnumber (_L, self->q_Dz1);
    lua_rawseti (_L, 3, 8);

    lua_pushnumber (_L, self->q_Dz2);
    lua_rawseti (_L, 3, 9);

    lua_pushnumber (_L, self->q_Dz3);
    lua_rawseti (_L, 3, 10);

    lua_pushnumber (_L, self->q_Dz4);
    lua_rawseti (_L, 3, 11);

    lua_pushnumber (_L, self->q_Dz8);
    lua_rawseti (_L, 3, 12);

    lua_pushnumber (_L, self->q_Dz9);
    lua_rawseti (_L, 3, 13);

    lua_pushnumber (_L, self->q_Dz10);
    lua_rawseti (_L, 3, 14);

    lua_pushnumber (_L, self->q_Dz11);
    lua_rawseti (_L, 3, 15);

    lua_pushnumber (_L, self->q_Ez1);
    lua_rawseti (_L, 3, 16);

    lua_pushnumber (_L, self->q_Ez2);
    lua_rawseti (_L, 3, 17);

    lua_pushnumber (_L, self->q_Ez3);
    lua_rawseti (_L, 3, 18);

    lua_pushnumber (_L, self->q_Ez5);
    lua_rawseti (_L, 3, 19);

    lua_pushnumber (_L, self->q_Hz3);
    lua_rawseti (_L, 3, 20);

    lua_pushnumber (_L, self->q_Hz4);
    lua_rawseti (_L, 3, 21);

    return 1;
}

-(int) _get_relaxation
{
    int i;
    
    lua_newtable (_L);

    for (i = 0 ; i < 3 ; i += 1) {
	lua_pushnumber (_L, self->relaxation[i]);
	lua_rawseti (_L, 3, i + 1);
    }

    return 1;
}

-(int) _get_resistance
{
    lua_pushnumber (_L, self->resistance);

    return 1;
}

-(int) _get_state
{
    lua_newtable (_L);

    lua_pushnumber (_L, self->gamma);
    lua_rawseti (_L, -2, 1);

    lua_pushnumber (_L, self->kappa);
    lua_rawseti (_L, -2, 2);

    lua_pushnumber (_L, self->beta_1);
    lua_rawseti (_L, -2, 3);
 
    lua_pushnumber (_L, self->F_z);
    lua_rawseti (_L, -2, 4);
 
    lua_pushnumber (_L, self->F_x);
    lua_rawseti (_L, -2, 5);
 
    lua_pushnumber (_L, self->F_y);
    lua_rawseti (_L, -2, 6);
 
    lua_pushnumber (_L, self->M_z);
    lua_rawseti (_L, -2, 7);
 
    lua_pushnumber (_L, self->F_x0);
    lua_rawseti (_L, -2, 8);
 
    lua_pushnumber (_L, self->F_y0);
    lua_rawseti (_L, -2, 9);
 
    lua_pushnumber (_L, self->M_z0);
    lua_rawseti (_L, -2, 10);

    return 1;
}

-(int) _get_scaling
{
    int i;
    
    struct wheeldata *data;

    data = dGeomGetClassData (self->geom);

    lua_newtable(_L);
	
    for (i = 0 ; i < 10 ; i += 1) {
	lua_pushnumber (_L, data->lambda[i]);
	lua_rawseti (_L, -2, i + 1);
    }

    return 1;
}

-(void) _set_elasticity
{
    struct wheeldata *data;

    data = dGeomGetClassData (self->geom);

    lua_pushinteger (_L, 1);
    lua_gettable (_L, 3);
    data->elasticity[0] = lua_tonumber (_L, -1);

    lua_pushinteger (_L, 2);
    lua_gettable (_L, 3);
    data->elasticity[1] = lua_tonumber (_L, -1);

    lua_pop (_L, 2);
}

-(void) _set_radii
{
    struct wheeldata *data;

    data = dGeomGetClassData (self->geom);

    lua_pushinteger (_L, 1);
    lua_gettable (_L, 3);
    data->radii[0] = lua_tonumber (_L, -1);

    lua_pushinteger (_L, 2);
    lua_gettable (_L, 3);
    data->radii[1] = lua_tonumber (_L, -1);

    lua_pop (_L, 2);
}

-(void) _set_longitudinal
{
    /* Parameters for pure and combined
       longitudinal slip. */

    lua_pushinteger (_L, 1);
    lua_gettable (_L, 3);
    self->C_x = lua_tonumber (_L, -1);

    lua_pushinteger (_L, 2);
    lua_gettable (_L, 3);
    self->p_Dx1 = lua_tonumber (_L, -1);

    lua_pushinteger (_L, 3);
    lua_gettable (_L, 3);
    self->p_Dx2 = lua_tonumber (_L, -1);

    lua_pushinteger (_L, 4);
    lua_gettable (_L, 3);
    self->p_Ex1 = lua_tonumber (_L, -1);

    lua_pushinteger (_L, 5);
    lua_gettable (_L, 3);
    self->p_Ex2 = lua_tonumber (_L, -1);

    lua_pushinteger (_L, 6);
    lua_gettable (_L, 3);
    self->p_Ex3 = lua_tonumber (_L, -1);

    lua_pushinteger (_L, 7);
    lua_gettable (_L, 3);
    self->p_Ex4 = lua_tonumber (_L, -1);

    lua_pushinteger (_L, 8);
    lua_gettable (_L, 3);
    self->p_Kx1 = lua_tonumber (_L, -1);

    lua_pushinteger (_L, 9);
    lua_gettable (_L, 3);
    self->p_Kx2 = lua_tonumber (_L, -1);

    lua_pushinteger (_L, 10);
    lua_gettable (_L, 3);
    self->p_Kx3 = lua_tonumber (_L, -1);

    lua_pushinteger (_L, 11);
    lua_gettable (_L, 3);
    self->r_Bx1 = lua_tonumber (_L, -1);

    lua_pushinteger (_L, 12);
    lua_gettable (_L, 3);
    self->r_Bx2 = lua_tonumber (_L, -1);

    lua_pushinteger (_L, 13);
    lua_gettable (_L, 3);
    self->C_xalpha = lua_tonumber (_L, -1);

    lua_pop (_L, 13);
}

-(void) _set_lateral
{
    /* Parameters for pure and combined
       lateral slip. */

    lua_pushinteger (_L, 1);
    lua_gettable (_L, 3);
    self->C_y = lua_tonumber (_L, -1);

    lua_pushinteger (_L, 2);
    lua_gettable (_L, 3);
    self->p_Dy1 = lua_tonumber (_L, -1);

    lua_pushinteger (_L, 3);
    lua_gettable (_L, 3);
    self->p_Dy2 = lua_tonumber (_L, -1);

    lua_pushinteger (_L, 4);
    lua_gettable (_L, 3);
    self->p_Dy3 = lua_tonumber (_L, -1);

    lua_pushinteger (_L, 5);
    lua_gettable (_L, 3);
    self->p_Ey1 = lua_tonumber (_L, -1);  

    lua_pushinteger (_L, 6);
    lua_gettable (_L, 3);
    self->p_Ey2 = lua_tonumber (_L, -1);

    lua_pushinteger (_L, 7);
    lua_gettable (_L, 3);
    self->p_Ey4 = lua_tonumber (_L, -1);

    lua_pushinteger (_L, 8);
    lua_gettable (_L, 3);
    self->p_Ky1 = lua_tonumber (_L, -1);

    lua_pushinteger (_L, 9);
    lua_gettable (_L, 3);
    self->p_Ky2 = lua_tonumber (_L, -1);

    lua_pushinteger (_L, 10);
    lua_gettable (_L, 3);
    self->p_Ky3 = lua_tonumber (_L, -1);

    lua_pushinteger (_L, 11);
    lua_gettable (_L, 3);
    self->p_Ky4 = lua_tonumber (_L, -1);

    lua_pushinteger (_L, 12);
    lua_gettable (_L, 3);
    self->p_Ky5 = lua_tonumber (_L, -1);

    lua_pushinteger (_L, 13);
    lua_gettable (_L, 3);
    self->C_gamma = lua_tonumber (_L, -1);

    lua_pushinteger (_L, 14);
    lua_gettable (_L, 3);
    self->p_Ky6 = lua_tonumber (_L, -1);

    lua_pushinteger (_L, 15);
    lua_gettable (_L, 3);
    self->p_Ky7 = lua_tonumber (_L, -1);

    lua_pushinteger (_L, 16);
    lua_gettable (_L, 3);
    self->E_gamma = lua_tonumber (_L, -1);

    lua_pushinteger (_L, 17);
    lua_gettable (_L, 3);
    self->r_By1 = lua_tonumber (_L, -1);

    lua_pushinteger (_L, 18);
    lua_gettable (_L, 3);
    self->r_By2 = lua_tonumber (_L, -1);

    lua_pushinteger (_L, 19);
    lua_gettable (_L, 3);
    self->r_By3 = lua_tonumber (_L, -1);

    lua_pushinteger (_L, 20);
    lua_gettable (_L, 3);
    self->C_ykappa = lua_tonumber (_L, -1);

    lua_pop (_L, 20);
}

-(void) _set_moment
{
    /* Parameters for the aligning moment. */

    lua_pushinteger (_L, 1);
    lua_gettable (_L, 3);
    self->C_t = lua_tonumber (_L, -1);

    lua_pushinteger (_L, 2);
    lua_gettable (_L, 3);
    self->q_Bz1 = lua_tonumber (_L, -1);

    lua_pushinteger (_L, 3);
    lua_gettable (_L, 3);
    self->q_Bz2 = lua_tonumber (_L, -1);

    lua_pushinteger (_L, 4);
    lua_gettable (_L, 3);
    self->q_Bz5 = lua_tonumber (_L, -1);

    lua_pushinteger (_L, 5);
    lua_gettable (_L, 3);
    self->q_Bz6 = lua_tonumber (_L, -1);

    lua_pushinteger (_L, 6);
    lua_gettable (_L, 3);
    self->q_Bz9 = lua_tonumber (_L, -1);

    lua_pushinteger (_L, 7);
    lua_gettable (_L, 3);
    self->q_Bz10 = lua_tonumber (_L, -1);

    lua_pushinteger (_L, 8);
    lua_gettable (_L, 3);
    self->q_Dz1 = lua_tonumber (_L, -1);

    lua_pushinteger (_L, 9);
    lua_gettable (_L, 3);
    self->q_Dz2 = lua_tonumber (_L, -1);

    lua_pushinteger (_L, 10);
    lua_gettable (_L, 3);
    self->q_Dz3 = lua_tonumber (_L, -1);

    lua_pushinteger (_L, 11);
    lua_gettable (_L, 3);
    self->q_Dz4 = lua_tonumber (_L, -1);

    lua_pushinteger (_L, 12);
    lua_gettable (_L, 3);
    self->q_Dz8 = lua_tonumber (_L, -1);

    lua_pushinteger (_L, 13);
    lua_gettable (_L, 3);
    self->q_Dz9 = lua_tonumber (_L, -1);

    lua_pushinteger (_L, 14);
    lua_gettable (_L, 3);
    self->q_Dz10 = lua_tonumber (_L, -1);

    lua_pushinteger (_L, 15);
    lua_gettable (_L, 3);
    self->q_Dz11 = lua_tonumber (_L, -1);

    lua_pushinteger (_L, 16);
    lua_gettable (_L, 3);
    self->q_Ez1 = lua_tonumber (_L, -1);

    lua_pushinteger (_L, 17);
    lua_gettable (_L, 3);
    self->q_Ez2 = lua_tonumber (_L, -1);

    lua_pushinteger (_L, 18);
    lua_gettable (_L, 3);
    self->q_Ez3 = lua_tonumber (_L, -1);

    lua_pushinteger (_L, 19);
    lua_gettable (_L, 3);
    self->q_Ez5 = lua_tonumber (_L, -1);

    lua_pushinteger (_L, 20);
    lua_gettable (_L, 3);
    self->q_Hz3 = lua_tonumber (_L, -1);

    lua_pushinteger (_L, 21);
    lua_gettable (_L, 3);
    self->q_Hz4 = lua_tonumber (_L, -1);

    lua_pop (_L, 20);
}

-(void) _set_load
{
    self->F_z0 = lua_tonumber (_L, 3);
}

-(void) _set_relaxation
{
    int i;
    
    for (i = 0 ; i < 3 ; i += 1) {
	lua_pushinteger (_L, i + 1);
	lua_gettable (_L, 3);
	self->relaxation[i] = lua_tonumber (_L, -1);
	lua_pop (_L, 1);
    }
}

-(void) _set_resistance
{
    self->resistance = lua_tonumber (_L, 3);
}

-(void) _set_state
{
    double one[10] = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1};
	
    lua_pushinteger(_L, 1);
    lua_gettable (_L, 3);
    self->gamma = lua_tonumber (_L, -1);
	
    lua_pushinteger(_L, 2);
    lua_gettable (_L, 3);
    self->kappa = lua_tonumber (_L, -1);
	
    lua_pushinteger(_L, 3);
    lua_gettable (_L, 3);
    self->beta = lua_tonumber (_L, -1);
	
    lua_pushinteger(_L, 4);
    lua_gettable (_L, 3);
    self->F_z = lua_tonumber (_L, -1);

    [self evaluateWithStep: 0 andFactors: one];
}

-(void) _set_scaling
{
    int i;
    
    struct wheeldata *data;

    data = dGeomGetClassData (self->geom);

    for (i = 0 ; i < 10 ; i += 1) {
	lua_pushinteger (_L, i + 1);
	lua_gettable (_L, 3);
	data->lambda[i] = lua_tonumber (_L, -1);
	lua_pop (_L, 1);
    }
}

@end
