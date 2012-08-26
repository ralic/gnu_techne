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

#include <stdlib.h>

#include <lua.h>
#include <lauxlib.h>

#include <GL/gl.h>
#include <GL/glx.h>

#include "techne.h"
#include "flat.h"
#include "shader.h"

static Shader *mold;
static int reference;

@implementation Flat
-(Flat *)init
{
#include "glsl/flat_vertex.h"	
#include "glsl/flat_fragment.h"	
    
    /* If this is the first instance create the program. */

    if (!mold) {
	mold = [[Shader alloc] init];

	[mold addSource: glsl_flat_vertex for: VERTEX_STAGE];
	[mold addSource: glsl_flat_fragment for: FRAGMENT_STAGE];
	[mold link];

	reference = luaL_ref (_L, LUA_REGISTRYINDEX);
    }
    
    [self initFrom: mold];
    
    return self;
}

@end
