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
#include "glsl/vegetation_fragment.h"

    /* Update the swatch nodes. */
    
    asprintf (&header, "const int N = %d;\n", self->swatches);

    [self unload];
    
    shader = [ShaderMold alloc];
        
    [shader initWithHandle: NULL];
    [shader declare: 6 privateUniforms: private];
    [shader add: 4 sourceStrings: (const GLchar *[4]){header, glsl_rand, glsl_color, glsl_vegetation_vertex} for: T_VERTEX_STAGE];

    {
        const char *foo = "layout(lines) in;"
            "layout(triangle_strip, max_vertices = 4) out;"
            "in vec3 shade[2];"
            "in int _index[2];"
            "out vec3 _shade;"
            "void main() {"
            "/*switch (_index[0]) {"
            "case 2:*/"
            "mat4 PM = projection * modelview;"
            "_shade = shade[0];"
            "gl_Position = PM * (gl_in[0].gl_Position + vec4(0, 0.005, 0, 0));"
            "EmitVertex();"
            "gl_Position = PM * (gl_in[0].gl_Position - vec4(0, 0.005, 0, 0));"
            "EmitVertex();"
            "gl_Position = PM * (gl_in[1].gl_Position + vec4(0, 0.005, 0, 0));"
            "EmitVertex();"
            "gl_Position = PM * (gl_in[1].gl_Position - vec4(0, 0.005, 0, 0));"
            "EmitVertex();"
            "gl_PrimitiveID = gl_PrimitiveIDIn;"
            "EndPrimitive();"
            "/*break;"
            "}*/"
            "}";
        
        [shader addSourceString: foo for: T_GEOMETRY_STAGE];
    }

    /* Assemble the tessellation source. */
    /* Start with the global headers. */
    
    [shader addSourceFragement: glsl_vegetation_tesselation_control_header
                           for: T_TESSELATION_CONTROL_STAGE];
        
    [shader addSourceFragement: glsl_vegetation_tesselation_evaluation_header
                           for: T_TESSELATION_EVALUATION_STAGE];

    /* Add per-swatch headers. */
    
    for (child = self->down;
         child;
         child = child->right) {
        if ([child isKindOf: [Swatch class]]) {
            const char **sources = [(Swatch *)child implementation];

            if (sources) {
                [shader addSourceFragement: sources[0]
                                       for: T_TESSELATION_CONTROL_STAGE];
                    
                [shader addSourceFragement: sources[1]
                                       for: T_TESSELATION_EVALUATION_STAGE];
            }
        }
    }

    /* Start the main functions. */
    
    [shader addSourceFragement: "void main() { switch(seed[0].index) {\n" 
                           for: T_TESSELATION_CONTROL_STAGE];
        
    [shader addSourceFragement: "void main() {shade = color; _index = index; switch(index) {\n"
                           for: T_TESSELATION_EVALUATION_STAGE];

    /* Add switching code. */
    
    for (child = self->down, i = 0;
         child;
         child = child->right) {
        if ([child isKindOf: [Swatch class]]) {
            char *s;

            if ([child isMemberOf: [Barren class]]) {
                asprintf(&s,
                         "case %d: if(gl_InvocationID == 0) gl_TessLevelOuter[0] = gl_TessLevelOuter[1] = gl_TessLevelOuter[2] = gl_TessLevelInner[0] = gl_TessLevelInner[1] = 0;break;\n",
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
                [shader addSourceFragement: s
                 for: T_TESSELATION_EVALUATION_STAGE];
            }
                
            i += 1;
        }
    }
        
    [shader addSourceFragement: "}}"
                           for: T_TESSELATION_EVALUATION_STAGE];

    [shader addSourceFragement: "}}" 
                           for: T_TESSELATION_CONTROL_STAGE];

    [shader finishAssemblingSourceFor: T_TESSELATION_CONTROL_STAGE];
    [shader finishAssemblingSourceFor: T_TESSELATION_EVALUATION_STAGE];

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
