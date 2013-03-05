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
#include <string.h>

#include <lua.h>
#include <lauxlib.h>

#include "gl.h"

#include "array/array.h"
#include "techne.h"
#include "palette.h"
#include "swatch.h"
#include "shader.h"

@implementation Palette

-(void)init
{
    [super init];
    
    self->separation = 1;
}

-(int) _get_separation
{
    lua_pushnumber (_L, self->separation);

    return 1;
}

-(void) _set_separation
{
    self->separation = lua_tonumber (_L, -1);

    if (self->name) {
        glUseProgram(self->name);
        glUniform1f(self->locations.power, self->separation);
    }
}

-(void) update
{
    Node *child;
    const char *private[8] = {"base", "colors", "offset", "scale",
                              "power", "references", "weights"};
    char *header;
    ShaderMold *shader;
    int n;
        
#include "glsl/color.h"
#include "glsl/vegetation_vertex.h"
#include "glsl/vegetation_geometry.h"
#include "glsl/vegetation_fragment.h"

    /* Update the swatch nodes. */

    for (child = self->down, n = 0 ; child ; child = child->right) {
        if ([child isKindOf: [Swatch class]]) {
            ((Swatch *)child)->swatch = n;
            n += 1;
        }
    }

    asprintf (&header, "const int N = %d;\n", n);

    [self unload];
    
    shader = [ShaderMold alloc];
        
    [shader initWithHandle: NULL];
    [shader declare: 7 privateUniforms: private];
    [shader add: 3 sourceStrings: (const GLchar *[3]){header, glsl_color, glsl_vegetation_vertex} for: T_VERTEX_STAGE];
    [shader add: 2 sourceStrings: (const GLchar *[2]){header, glsl_vegetation_geometry} for: T_GEOMETRY_STAGE];
    [shader addSourceString: glsl_vegetation_fragment for: T_FRAGMENT_STAGE];
    [shader link];

    [self load];

    /* Get uniform locations. */
        
    self->locations.base = glGetUniformLocation (self->name, "base");
    self->locations.colors = glGetUniformLocation (self->name, "colors");
    self->locations.power = glGetUniformLocation (self->name, "power");
    self->locations.references = glGetUniformLocation (self->name, "references");
    self->locations.weights = glGetUniformLocation (self->name, "weights");
        
    /* _TRACE ("%d, %d, %d, %d\n", self->locations.base, self->locations.detail, self->locations.power, self->locations.matrices); */

    /* Vegetation-related uniforms will remain constant as the
     * program is only used by this node so all uniform values can
     * be loaded beforehand. */

    glUseProgram(self->name);
        
    glUniform1i(self->locations.base, 0);
    glUniform1f(self->locations.power, self->separation);
}

-(void) adopt: (Node *)child
{
    [super adopt: child];

    if ([child isKindOf: [Swatch class]]) {
        [self update];
    }
}

-(void) renounce: (Node *)child
{
    [super renounce: child];

    if ([child isKindOf: [Swatch class]]) {
        [self update];
    }
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
