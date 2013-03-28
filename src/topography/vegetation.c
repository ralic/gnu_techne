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
#include "vegetation.h"
#include "swatch.h"
#include "barren.h"
#include "shader.h"

@implementation Vegetation

-(void)init
{
    [super init];

    self->swatches = 0;
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
    const char *private[6] = {"base", "offset", "scale", "power",
                              "references", "weights"};
    char *header;
    ShaderMold *shader;
    int i;
        
#include "glsl/color.h"
#include "glsl/rand.h"
#include "glsl/vegetation_vertex.h"
#include "glsl/vegetation_tesselation_control_header.h"
#include "glsl/vegetation_tesselation_evaluation_header.h"
#include "glsl/vegetation_geometry_header.h"
#include "glsl/vegetation_fragment.h"

    /* Update the swatch nodes. */
    
    asprintf (&header, "const int N = %d;\n", self->swatches);

    [self unload];
    
    shader = [ShaderMold alloc];
        
    [shader initWithHandle: NULL];
    [shader declare: 6 privateUniforms: private];
    [shader add: 4 sourceStrings: (const char *[4]){header, glsl_rand, glsl_color, glsl_vegetation_vertex} for: T_VERTEX_STAGE];

    /* Assemble the tessellation source. */
    /* Start with the global headers. */
    
    [shader addSourceFragement: glsl_vegetation_tesselation_control_header
                           for: T_TESSELATION_CONTROL_STAGE];
        
    [shader addSourceFragement: glsl_vegetation_tesselation_evaluation_header
                           for: T_TESSELATION_EVALUATION_STAGE];
        
    [shader addSourceFragement: glsl_vegetation_geometry_header
                           for: T_GEOMETRY_STAGE];

    /* Add per-swatch headers. */
    
    for (child = self->down;
         child;
         child = child->right) {
        if ([child isKindOf: [Swatch class]]) {
            Swatch *swatch = (Swatch *)child;
            char *s;
            
            if (swatch->sources[T_TESSELATION_CONTROL_STAGE]) {
                [shader add: 2 sourceStrings: (const char *[2]){
                        glsl_vegetation_tesselation_control_header,
                            swatch->sources[T_TESSELATION_CONTROL_STAGE]}
                                       for: T_TESSELATION_CONTROL_STAGE];
                
                asprintf(&s, "void %s_control();\n", [child name]);
                
                [shader addSourceFragement: s for: T_TESSELATION_CONTROL_STAGE];
            }

            if (swatch->sources[T_TESSELATION_EVALUATION_STAGE]) {
                [shader add: 2 sourceStrings: (const char *[2]){
                        glsl_vegetation_tesselation_evaluation_header,
                            swatch->sources[T_TESSELATION_EVALUATION_STAGE]}
                                       for: T_TESSELATION_EVALUATION_STAGE];
                
                asprintf(&s, "void %s_evaluation();\n", [child name]);
                
                [shader addSourceFragement: s for: T_TESSELATION_EVALUATION_STAGE];
            }

            if (swatch->sources[T_GEOMETRY_STAGE]) {
                [shader add:2 sourceStrings: (const char *[2]){
                        glsl_vegetation_geometry_header,
                            swatch->sources[T_GEOMETRY_STAGE]}
                                       for: T_GEOMETRY_STAGE];
                
                asprintf(&s, "void %s_geometry();\n", [child name]);
                
                [shader addSourceFragement: s for: T_GEOMETRY_STAGE];
            }
        }
    }

    /* Start the main functions. */
    
    [shader addSourceFragement: "void main() { color_tc = color[0]; index_tc = index[0]; switch(index[0]) {\n" 
                           for: T_TESSELATION_CONTROL_STAGE];
        
    [shader addSourceFragement: "void main() { color_te = color_tc; index_te = index_tc; switch(index_tc) {\n"
                           for: T_TESSELATION_EVALUATION_STAGE];
        
    [shader addSourceFragement: "void main() { color_g = color_te[0]; switch(index_te[0]) {\n"
                           for: T_GEOMETRY_STAGE];

    /* Add switching code. */
    
    for (child = self->down, i = 0;
         child;
         child = child->right) {
        if ([child isKindOf: [Swatch class]]) {
            char *s;

            if ([child isMemberOf: [Barren class]]) {
                asprintf(&s,
                         "case %d: if(gl_InvocationID == 0) gl_TessLevelOuter[0] = gl_TessLevelOuter[1] = 0;break;\n",
                         i);
                
                [shader addSourceFragement: s
                                       for: T_TESSELATION_CONTROL_STAGE];
            } else {
                asprintf(&s,
                         "case %d: %s_control();break;\n",
                         i, [child name]);
                
                [shader addSourceFragement: s
                                       for: T_TESSELATION_CONTROL_STAGE];
                
                asprintf(&s,
                         "case %d: %s_evaluation();break;\n",
                         i, [child name]);
                [shader addSourceFragement: s for: T_TESSELATION_EVALUATION_STAGE];
                
                asprintf(&s,
                         "case %d: %s_geometry();break;\n",
                         i, [child name]);
                [shader addSourceFragement: s for: T_GEOMETRY_STAGE];
            }
                
            i += 1;
        }
    }

    [shader addSourceFragement: "}}" 
                           for: T_TESSELATION_CONTROL_STAGE];
        
    [shader addSourceFragement: "}}"
                           for: T_TESSELATION_EVALUATION_STAGE];

    [shader addSourceFragement: "}}" 
                           for: T_GEOMETRY_STAGE];

    [shader finishAssemblingSourceFor: T_TESSELATION_CONTROL_STAGE];
    [shader finishAssemblingSourceFor: T_TESSELATION_EVALUATION_STAGE];
    [shader finishAssemblingSourceFor: T_GEOMETRY_STAGE];

    /* Add the fragment source. */
    
    [shader addSourceString: glsl_vegetation_fragment for: T_FRAGMENT_STAGE];
    [shader link];

    [self load];

    /* Get uniform locations. */
        
    i = glGetUniformLocation (self->name, "base");
    self->locations.power = glGetUniformLocation (self->name, "power");

    /* Vegetation-related uniforms will remain constant as the
     * program is only used by this node so all uniform values can
     * be loaded beforehand. */

    glUseProgram(self->name);
        
    glUniform1i(i, 0);
    glUniform1f(self->locations.power, self->separation);

    /* Request all the children to update their uniforms. */
    
    for (child = self->down, i = 0 ; child ; child = child->right) {
        if ([child isKindOf: [Swatch class]]) {
            [(Swatch *)child updateWithProgram: self->name andIndex: i];
            i += 1;
        }
    }
}

-(void) adopt: (Node *)child
{
    [super adopt: child];

    if ([child isKindOf: [Swatch class]]) {
        self->swatches += 1;
        
        [self update];
    }
}

-(void) renounce: (Node *)child
{
    [super renounce: child];

    if ([child isKindOf: [Swatch class]]) {
        self->swatches -= 1;
        
        [self update];
    }
}

-(void) draw: (int)frame
{
    /* glEnable (GL_CULL_FACE); */
    glEnable (GL_DEPTH_TEST);
    
    glUseProgram(self->name);
    
    [super draw: frame];

    glDisable (GL_DEPTH_TEST);
    /* glDisable (GL_CULL_FACE); */
}

@end
