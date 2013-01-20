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

#include "gl.h"

#include "techne.h"
#include "splat.h"

static ShaderMold *handle;

@implementation Splat

-(void) init
{
#include "glsl/elevation_vertex.h"	
#include "glsl/elevation_fragment.h"	

    const char *private[3] = {"base", "offset", "scale"};
    int i;
    
    /* If this is the first instance create the program. */

    if (!handle) {
        ShaderMold *shader;
        
	shader = [ShaderMold alloc];
        
        [shader initWithHandle: &handle];
        [shader declare: 3 privateUniforms: private];
	[shader addSource: glsl_elevation_vertex for: VERTEX_STAGE];
	[shader addSource: glsl_elevation_fragment for: FRAGMENT_STAGE];
	[shader link];
    } else {
        t_pushuserdata(_L, 1, handle);
    }
    
    [super init];

    /* The base texture is always in unit 0 so load the sampler
     * uniform once and for all. */
    
    i = glGetUniformLocation(self->name, "base");
    glUseProgram(self->name);
    glUniform1i(i, 0);
}

-(void) draw
{
    glEnable (GL_CULL_FACE);
    glEnable (GL_DEPTH_TEST);
    
    glUseProgram(self->name);
    
    [super draw];

    glDisable (GL_DEPTH_TEST);
    glDisable (GL_CULL_FACE);
}

@end
