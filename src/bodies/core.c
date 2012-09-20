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
#include <lauxlib.h>

#include "techne.h"
#include "environment.h"
#include "point.h"
#include "box.h"
#include "ball.h"
#include "plane.h"
#include "capsule.h"
#include "cylinder.h"
#include "polyhedron.h"
#include "system.h"

int luaopen_bodies_core (lua_State *L)
{
    Class classes[] = {[Environment class], [Point class], [Box class],
		       [Plane class], [Capsule class], [Cylinder class],
		       [Polyhedron class], [Ball class], [System class], NULL};

    t_exportnodes (L, classes);
    
    return 1;
}
