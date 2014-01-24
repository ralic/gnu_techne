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
#include "dynamics.h"
#include "clamp.h"
#include "planar.h"
#include "hinge.h"
#include "doublehinge.h"
#include "doubleball.h"
#include "spherical.h"
#include "slider.h"
#include "contact.h"
#include "linear.h"
#include "angular.h"
#include "euler.h"
#include "universal.h"
#include "gearing.h"

int luaopen_joints_core (lua_State *L)
{
    Class classes[] = {[Clamp class], [Planar class], [Hinge class],
		       [Doublehinge class], [Doubleball class],
		       [Linear class], [Angular class], [Euler class],
		       [Universal class], [Gearing class], [Contact class],
		       [Slider class], [Spherical class], [Joint class],
                       NULL};

    t_exportnodes (L, classes);
    
    return 1;
}
