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
    Node *child;
    const char *private[] = {"base", "detail", "offset", "scale", "power",
                             "factor", "references", "weights",
                             "resolutions"};
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

    /* Make a reference to the elevation to make sure it's not
     * collected. */

    self->elevation = t_tonode (_L, -1);
    self->reference_1 = luaL_ref (_L, LUA_REGISTRYINDEX);

    [super init];

    /* Create the program. */

    asprintf (&header, "const int N = %d;\n", self->elevation->swatches);

    [self unload];
    
    shader = [ShaderMold alloc];
        
    [shader initWithHandle: NULL];
    [shader declare: 9 privateUniforms: private];
    [shader add: 4 sourceStrings: (const char *[4]){header, glsl_rand, glsl_color, glsl_vegetation_vertex} for: T_VERTEX_STAGE];

    /* Assemble the tessellation source. */
    /* Start with the global headers. */
    
    [shader addSourceFragement: glsl_vegetation_tesselation_control_header
                           for: T_TESSELATION_CONTROL_STAGE];
        
    [shader addSourceFragement: glsl_vegetation_tesselation_evaluation_header
                           for: T_TESSELATION_EVALUATION_STAGE];
        
    [shader addSourceFragement: glsl_vegetation_geometry_header
                           for: T_GEOMETRY_STAGE];

    /* Add per-swatch sources to the program as well as function
     * declarations. */
    
    for (child = self->elevation->down;
         child;
         child = child->right) {
        if ([child isKindOf: [Swatch class]] &&
            ![child isMemberOf: [Barren class]]) {
            Swatch *swatch = (Swatch *)child;
            char *s;
            
            /* Tessellation control stage. */

            [swatch addSourceToVegetationShader: shader for: T_TESSELATION_CONTROL_STAGE];
                
            asprintf(&s, "void %s_control();\n", [child name]);
            [shader addSourceFragement: s for: T_TESSELATION_CONTROL_STAGE];

            /* Tessellation evaluation stage. */
            
            [swatch addSourceToVegetationShader: shader for: T_TESSELATION_EVALUATION_STAGE];
            
            asprintf(&s, "void %s_evaluation();\n", [child name]);
            [shader addSourceFragement: s for: T_TESSELATION_EVALUATION_STAGE];

            /* Geometry stage. */
            
            [swatch addSourceToVegetationShader: shader for: T_GEOMETRY_STAGE];
                
            asprintf(&s, "void %s_geometry();\n", [child name]);
            [shader addSourceFragement: s for: T_GEOMETRY_STAGE];
        }
    }

    /* Start the main functions. */
    
    [shader addSourceFragement: "void main() { switch(index_tc = index[0]) {\n" 
                           for: T_TESSELATION_CONTROL_STAGE];
        
    [shader addSourceFragement: "void main() { switch(index_te = index_tc) {\n"
                           for: T_TESSELATION_EVALUATION_STAGE];
        
    [shader addSourceFragement: "void main() { switch(index_te[0]) {\n"
                           for: T_GEOMETRY_STAGE];

    /* Add switching code. */
    
    for (child = self->elevation->down, i = 0;
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

    /* Initialize uniforms. */

    glUseProgram(self->name);

    self->locations.power = glGetUniformLocation (self->name, "power");
    self->locations.factor = glGetUniformLocation (self->name, "factor");
    self->locations.references = glGetUniformLocation (name, "references");
    self->locations.weights = glGetUniformLocation (name, "weights");
    self->locations.resolutions = glGetUniformLocation (self->name, "resolutions");

    glUniform1f(self->locations.power, self->elevation->separation);
    glUniform1f (self->locations.factor, self->elevation->albedo);

    /* Initialize reference color uniforms. */
    
    for (child = self->elevation->down, i = 0 ; child ; child = child->right) {
        if ([child isKindOf: [Swatch class]]) {
            Swatch *swatch = (Swatch *)child;

            glUniform3fv(self->locations.references + i, 1, swatch->values);
            glUniform3fv(self->locations.weights + i, 1, swatch->weights);
            glUniform2fv (self->locations.resolutions + i, 1,
                          swatch->resolutions);

            [swatch configureVegetationShader: self];
            [self setSamplerUniform: "detail" to: swatch->detail->name atIndex: i];
           
            i += 1;
        }
    }
}

-(void)free
{
    luaL_unref(_L, LUA_REGISTRYINDEX, self->reference_1);

    [super free];
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
