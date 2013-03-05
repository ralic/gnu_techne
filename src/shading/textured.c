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

#include "gl.h"

#include "techne.h"
#include "texture.h"
#include "textured.h"
#include "shader.h"

static ShaderMold *handle;

@implementation Textured
-(void)init
{
#include "glsl/textured_vertex.h"	
#include "glsl/textured_fragment.h"	
    
    [super init];
    
    /* If this is the first instance create the program. */

    if (!handle) {
        ShaderMold *shader;
        
	shader = [ShaderMold alloc];
        
        [shader initWithHandle: &handle];
	[shader addSourceString: glsl_textured_vertex for: T_VERTEX_STAGE];
	[shader addSourceString: glsl_textured_fragment for: T_FRAGMENT_STAGE];
	[shader link];
    } else {
        t_pushuserdata(_L, 1, handle);
    }

    [self load];
}

-(void) draw: (int)frame
{
    glEnable (GL_CULL_FACE);
    glEnable (GL_DEPTH_TEST);
    
    glUseProgram(self->name);
    
    [super draw: frame];

    glDisable (GL_DEPTH_TEST);
    glDisable (GL_CULL_FACE);
}

@end
