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

#ifndef _DYNAMICS_H_
#define _DYNAMICS_H_

#include <lua.h>
#include "transform.h"

@interface Dynamics: Node {
}

-(void) iterate: (Transform *)root;

-(int) _get_stepsize;
-(int) _get_ceiling;
-(int) _get_timescale;
-(int) _get_iterations;
-(int) _get_collision;
-(int) _get_tolerance;
-(int) _get_popvelocity;
-(int) _get_surfacelayer;
-(int) _get_gravity;

-(void) _set_stepsize;
-(void) _set_ceiling;
-(void) _set_timescale;
-(void) _set_iterations;
-(void) _set_collision;
-(void) _set_tolerance;
-(void) _set_popvelocity;
-(void) _set_surfacelayer;
-(void) _set_gravity;
-(int) _get_interval;
-(void) _set_interval;

@end

#endif
