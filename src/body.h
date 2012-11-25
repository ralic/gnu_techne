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

#ifndef _BODY_H_
#define _BODY_H_

#include <lua.h>
#include <ode/ode.h>
#include "dynamic.h"

@interface Body: Dynamic {
@public    
    dBodyID body;
    dGeomID geom;
    dSpaceID space;

    dReal velocity[3], spin[3];
    dMass mass;

    int clamped, encapsulated;
    int poststep;
}

-(void) fasten;
-(void) release;
-(void) insertInto: (dSpaceID) new;

-(int) _get_velocity;
-(int) _get_spin;
-(int) _get_mass;
-(int) _get_lethargy;
-(int) _get_damping;
-(int) _get_kinematic;
-(int) _get_poststep;
			       
-(void) _set_velocity;
-(void) _set_spin;
-(void) _set_mass;
-(void) _set_lethargy;
-(void) _set_damping;
-(void) _set_kinematic;
-(void) _set_poststep;

@end

#endif
