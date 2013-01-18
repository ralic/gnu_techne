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

#ifndef _FOURSTROKE_H_
#define _FOURSTROKE_H_

#include <lua.h>
#include <ode/ode.h>
#include "joint.h"

@interface Fourstroke: Joint {
    double state[9];
    double anchor[3], axis[3], tolerance;
    double benchspeed, throttle, bypass;
    double displacement, intake[2];
    double volumetric[4], thermal[3];
    double exchange[2], friction[3];
    int cylinders, idle, spark;
}

-(void) cycle;

-(int) _get_anchor;
-(int) _get_axis;
-(int) _get_tolerance;
-(int) _get_spark;
-(int) _get_throttle;
-(int) _get_displacement;
-(int) _get_cylinders;
-(int) _get_intake;
-(int) _get_volumetric;
-(int) _get_thermal;
-(int) _get_friction;
-(int) _get_exchange;

-(void) _set_axis;
-(void) _set_anchor;
-(void) _set_tolerance;
-(void) _set_spark;
-(void) _set_throttle;
-(void) _set_displacement;
-(void) _set_cylinders;
-(void) _set_intake;
-(void) _set_volumetric;
-(void) _set_thermal;
-(void) _set_friction;
-(void) _set_exchange;

-(int) _get_rate;
-(int) _get_angle;

-(void) _set_angle;
-(void) _set_rate;

@end

#endif
