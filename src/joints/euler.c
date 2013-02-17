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

#include "techne.h"
#include "dynamics.h"
#include "euler.h"

@implementation Euler

-(void) setup
{
    if (dJointGetBody (self->joint, 0) ||
	dJointGetBody (self->joint, 1)) {
	dJointSetAMotorAxis (self->joint, 0,
			     dJointGetBody (self->joint, 0) ? 1 : 0,
			     self->axes[0][0],
			     self->axes[0][1],
			     self->axes[0][2]);

	dJointSetAMotorAxis (self->joint, 2,
			     dJointGetBody (self->joint, 1) ? 2 : 0,
			     self->axes[2][0],
			     self->axes[2][1],
			     self->axes[2][2]);
	
	dJointSetAMotorMode (self->joint, dAMotorEuler);
    }
}

@end
