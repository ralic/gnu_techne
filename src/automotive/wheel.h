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

#ifndef _WHEEL_H_
#define _WHEEL_H_

#include <lua.h>
#include <ode/ode.h>
#include "body.h"

struct wheeldata {
    int airborne;
    dContactGeom contact;
    dVector3 lateral, axial, longitudinal, radial;
    dReal radii[2], elasticity[2];
    
    /*
      Scaling factors:
      mu_x, mu_y, K_x, K_ya, K_ygamma, t, K_zgamma, mu_r, k_s, k_d
    */
    
    dReal lambda[10];
};

int dWheelClass;

@interface Wheel: Body {
  dJointFeedback feedback;
  dJointID damper;

  double F_x, F_y, M_z, F_x0, F_y0, M_z0, F_z, F_z0, kappa, beta, beta_1, gamma;

  double C_x, p_Dx1, p_Dx2, p_Ex1, p_Ex2,
         p_Ex3, p_Ex4, p_Kx1, p_Kx2, p_Kx3;
  double r_Bx1, r_Bx2, C_xalpha;

  double C_y, p_Dy1, p_Dy2, p_Dy3, p_Ey1, p_Ey2,
         p_Ey4, p_Ky1, p_Ky2, p_Ky3, p_Ky4, p_Ky5,
         C_gamma, p_Ky6, p_Ky7, E_gamma;

  double r_By1, r_By2, r_By3, C_ykappa;

  double C_t, q_Bz1, q_Bz2, q_Bz5, q_Bz6, q_Bz9, q_Bz10,
         q_Dz1, q_Dz2, q_Dz3, q_Dz4, q_Dz8, q_Dz9, q_Dz10,
         q_Dz11, q_Ez1, q_Ez2, q_Ez3, q_Ez5, q_Hz3, q_Hz4;

  double resistance, relaxation[3];
}

-(void)evaluateWithStep: (double)h andFactors: (double[9])lambda;

-(int) _get_elasticity;
-(int) _get_radii;
-(int) _get_load;
-(int) _get_longitudinal;
-(int) _get_lateral;
-(int) _get_moment;
-(int) _get_relaxation;
-(int) _get_resistance;
-(int) _get_state;
-(int) _get_scaling;

-(void) _set_elasticity;
-(void) _set_radii;
-(void) _set_longitudinal;
-(void) _set_lateral;
-(void) _set_moment;
-(void) _set_load;
-(void) _set_relaxation;
-(void) _set_resistance;
-(void) _set_state;
-(void) _set_scaling;

@end

#endif
