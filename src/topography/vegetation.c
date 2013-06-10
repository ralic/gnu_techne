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

#define N_K 32               /* Number of stiffness samples. */
#define N_S 32               /* Number of blade segments. */
#define K_0 2.1              /* Lower bound of stiffness parameter. */
#define K_1 2.5              /* Upper bound of stiffness parameter. */

static unsigned int deflections;

@implementation Vegetation

+(void)initialize
{
    float *texels, x, y, k, theta_0, phi, l_i, A, Delta;
    int i, j;

    /* Calculate the deflection texture. */

    texels = malloc(N_K * N_S * 3 * sizeof(float));

    for (j = 0 ; j < N_K ; j += 1) {
        x = y = theta_0 = 0;
        k = K_0 + (K_1 - K_0) * (float)j / (N_K - 1);
        
        for (i = 0 ; i < N_S ; i += 1) {
            texels[3 * ((N_S * j) + i) + 0] = x;
            texels[3 * ((N_S * j) + i) + 1] = y;
            texels[3 * ((N_S * j) + i) + 2] = theta_0;

            if (i == N_S) {
                break;
            }
            
            l_i = (1 - (float)i / (N_S - 1));
            A = l_i * l_i / k;
            Delta = 1 - 4 / 2.42 * A * (M_PI / 2 - theta_0 - A);
            phi = (1 - sqrt(Delta)) / (2 / 2.42 * A);

            if (phi > M_PI / 2) {
                phi = M_PI - phi;
            }
            
            theta_0 += M_PI / 2 - theta_0 - phi;
            x += sin(theta_0) / (N_S - 1);
            y += cos(theta_0) / (N_S - 1);
        }
    }
    
    /* for (i = N_K / 2, j = 0 ; j < N_S ; j += 1) { */
    /*     _TRACE ("%f, %f, %f\n", texels[3 * ((N_S * i) + j) + 0], texels[3 * ((N_S * i) + j) + 1], texels[3 * ((N_S * i) + j) + 2]); */
    /* } */

    glGenTextures (1, &deflections);
    glBindTexture(GL_TEXTURE_2D, deflections);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, N_S, N_K, 0, GL_RGB,
                 GL_FLOAT, texels);
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    free (texels);
}

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
    self->locations.references = glGetUniformLocation (self->name, "references");
    self->locations.weights = glGetUniformLocation (self->name, "weights");
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

    [self setSamplerUniform: "deflections" to: deflections];
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
