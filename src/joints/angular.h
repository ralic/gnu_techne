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

#ifndef _ANGULAR_H_
#define _ANGULAR_H_

#include <lua.h>
#include <ode/ode.h>
#include "joint.h"

@interface Angular: Joint {
    int degrees, relative[3];
    dVector3 axes[3];
    double motor[3][2], stops[3][2], hardness[3][2];
    double tolerance[3], bounce[3];
}

-(void) setup;

-(int) _get_axes;
-(int) _get_relative;
-(int) _get_motor;
-(int) _get_stops;
-(int) _get_tolerance;

-(void) _set_axes;
-(void) _set_relative;
-(void) _set_motor;
-(void) _set_stops;
-(void) _set_tolerance;

-(int) _get_rates;
-(int) _get_positions;

-(void) _set_positions;
-(void) _set_rates;

@end

#endif
