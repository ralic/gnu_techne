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

#ifndef _AIRPLANE_H_
#define _AIRPLANE_H_

#include <ode/ode.h>
#include "joint.h"

@interface Airplane: Joint {
    dBodyID body;
    dJointID amotor, lmotor;
    
    double controls[3];
    double area, span, chord;

    double alpha_0, beta_0;
    dJointFeedback feedback_1;    
    
    struct {
	double reference;

	int lengths[10];
	double *values[10];
    } derivatives[6];
}

-(void) getDerivative: (int)k;
-(void) setDerivative: (int)k;

-(int) _get_area;
-(int) _get_span;
-(int) _get_chord;
-(int) _get_ailerons;
-(int) _get_elevators;
-(int) _get_rudder;
-(int) _get_drag;
-(int) _get_sideforce;
-(int) _get_lift;
-(int) _get_roll;
-(int) _get_pitch;
-(int) _get_yaw;

-(void) _set_area;
-(void) _set_span;
-(void) _set_chord;
-(void) _set_ailerons;
-(void) _set_elevators;
-(void) _set_rudder;
-(void) _set_drag;
-(void) _set_sideforce;
-(void) _set_lift;
-(void) _set_roll;
-(void) _set_pitch;
-(void) _set_yaw;

@end

#endif
