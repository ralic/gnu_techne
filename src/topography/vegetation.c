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
#include "atmosphere.h"
#include "vegetation.h"
#include "shader.h"

@implementation Vegetation

-(void)init
{
    const char *private[] = {"base", "detail", "offset", "scale",
                             "factor", "references", "weights",
                             "resolutions"};
    char *header;
    ShaderMold *shader;
    int i;
        
#include "glsl/color.h"
#include "glsl/rand.h"
#include "glsl/vegetation_vertex.h"
#include "glsl/vegetation_tesselation_control.h"
#include "glsl/vegetation_tesselation_evaluation.h"
#include "glsl/vegetation_geometry.h"
#include "glsl/vegetation_fragment.h"

    /* Make a reference to the elevation to make sure it's not
     * collected. */

    self->elevation = t_tonode (_L, -1);
    self->reference_1 = luaL_ref (_L, LUA_REGISTRYINDEX);

    [super init];

    /* Create the program. */

    asprintf (&header, "const int N = %d;\n", self->elevation->swatches_n);

    [self unload];
    
    shader = [ShaderMold alloc];
        
    [shader initWithHandle: NULL];
    [shader declare: 8 privateUniforms: private];
    [shader add: 4 sourceStrings: (const char *[4]){header, glsl_rand, glsl_color, glsl_vegetation_vertex} for: T_VERTEX_STAGE];

    [shader addSourceString: glsl_vegetation_tesselation_control
                        for: T_TESSELATION_CONTROL_STAGE];
    [shader addSourceString: glsl_vegetation_tesselation_evaluation
                        for: T_TESSELATION_EVALUATION_STAGE];
    [shader addSourceString: glsl_vegetation_geometry
                        for: T_GEOMETRY_STAGE];
    
    /* Add the fragment source. */
    
    [shader addSourceString: glsl_vegetation_fragment for: T_FRAGMENT_STAGE];
    [shader link];

    [self load];

    /* Initialize uniforms. */

    glUseProgram(self->name);

    self->locations.factor = glGetUniformLocation (self->name, "factor");
    self->locations.references = glGetUniformLocation (name, "references");
    self->locations.weights = glGetUniformLocation (name, "weights");
    self->locations.resolutions = glGetUniformLocation (self->name, "resolutions");
    self->locations.intensity = glGetUniformLocation (self->name, "intensity");

    glUniform1f (self->locations.factor, self->elevation->albedo);

    /* Initialize reference color uniforms. */
    
    for (i = 0 ; i < self->elevation->swatches_n ; i += 1) {
        elevation_SwatchDetail *swatch;

        swatch = &self->elevation->swatches[i];
        
        [self setSamplerUniform: "detail" to: swatch->detail->name atIndex: i];
            
        glUniform2fv (self->locations.resolutions + i, 1,
                      swatch->resolution);
        glUniform3fv (self->locations.references + i, 1, swatch->values);
        glUniform3fv (self->locations.weights + i, 1, swatch->weights);
    }
}

-(void)free
{
    luaL_unref(_L, LUA_REGISTRYINDEX, self->reference_1);

    [super free];
}

-(void) draw: (int)frame
{
    Atmosphere *atmosphere;

    /* glEnable (GL_CULL_FACE); */
    glEnable (GL_DEPTH_TEST);
    
    glUseProgram(self->name);

    atmosphere = [Atmosphere instance];
    
    if (atmosphere) {
        glUniform3fv (self->locations.intensity, 1, atmosphere->intensity);
    }
    
    [super draw: frame];

    glDisable (GL_DEPTH_TEST);
    /* glDisable (GL_CULL_FACE); */
}

@end
