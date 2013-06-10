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

#include "gl.h"

#include "techne.h"
#include "capsule.h"

@implementation Capsule

-(void) init
{
    self->geom = dCreateCapsule (NULL, 1, 1);
    dGeomSetData (self->geom, self);

    self->radius = 1;
    self->height = 1;
    
    [super init];
}

-(int) _get_radius
{
    lua_pushnumber (_L, self->radius);
    
    return 1;
}

-(int) _get_length
{
    lua_pushnumber (_L, self->height);

    return 1;
}

-(void) _set_radius
{
    self->radius = lua_tonumber (_L, 3);

    dGeomCapsuleSetParams (self->geom, self->radius, self->height);
}

-(void) _set_length
{
    self->height = lua_tonumber (_L, 3);

    dGeomCapsuleSetParams (self->geom, self->radius, self->height);
}

@end
