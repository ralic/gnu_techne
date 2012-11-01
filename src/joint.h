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

#ifndef _JOINT_H_
#define _JOINT_H_

#include <ode/ode.h>
#include "transform.h"

@interface Joint: Transform {
@public
    dJointID joint;
    dBodyID bodies[2];
    dJointFeedback feedback;    

    int inverted, attach, debug, explicit;
}

-(void) update;

-(int) _get_forces;
-(int) _get_torques;
-(int) _get_inverted;
-(int) _get_bodies;
-(int) _get_attach;
-(int) _get_pair;

-(void) _set_forces;
-(void) _set_torques;
-(void) _set_inverted;
-(void) _set_bodies;
-(void) _set_attach;
-(void) _set_pair;

@end

#endif
