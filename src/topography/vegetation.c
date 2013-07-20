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

    /* { */
    /*     double s, x_0, y_0; */
        
    /*     for (j = 0 ; j < N_K ; j += 1) { */
    /*         for (i = 0, s = x_0 = y_0 = 0 ; i < N_S ; i += 1) { */
    /*             double x, y, dx, dy; */
                
    /*             x = texels[3 * ((N_S * j) + i) + 0]; */
    /*             y = texels[3 * ((N_S * j) + i) + 1]; */

    /*             dx = x - x_0; dy = y - y_0; */
    /*             x_0 = x; y_0 = y; */

    /*             s += sqrt(dx * dx + dy * dy); */
    /*         } */

    /*         _TRACE ("%f\n", s); */
    /*     } */
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
                             "resolutions", "planes", "clustering"};
    char *header;
    ShaderMold *shader;
    int i, collect;
        
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

    self->masks = malloc (self->elevation->swatches_n * sizeof(int));

    for (i = 0 ; i < self->elevation->swatches_n ; i += 1) {
        self->masks[i] = LUA_REFNIL;
    }
    
    [super init];

    /* Are we profiling? */
    
    lua_getglobal (_L, "options");

    lua_getfield (_L, -1, "profile");
    collect = lua_toboolean (_L, -1);
    lua_pop (_L, 2);

    /* Create the program. */

    asprintf (&header, "const int N = %d;\n%s",
              self->elevation->swatches_n,
              collect ? "#define COLLECT_STATISTICS\n" : "");

    [self unload];
    
    shader = [ShaderMold alloc];
        
    [shader initWithHandle: NULL];
    [shader declare: 10 privateUniforms: private];
    [shader add: 4 sourceStrings: (const char *[4]){header, glsl_rand, glsl_color, glsl_vegetation_vertex} for: T_VERTEX_STAGE];

    [shader addSourceString: glsl_vegetation_tesselation_control
                        for: T_TESSELATION_CONTROL_STAGE];
    [shader add: 3 sourceStrings: (const char *[3]){header, glsl_rand, glsl_vegetation_tesselation_evaluation}
                        for: T_TESSELATION_EVALUATION_STAGE];
    [shader addSourceString: glsl_vegetation_geometry
                        for: T_GEOMETRY_STAGE];
    
    /* Add the fragment source. */
    
    [shader add: 2 sourceStrings: (const char *[2]){header, glsl_vegetation_fragment} for: T_FRAGMENT_STAGE];
    [shader link];

    [self load];

    /* Initialize uniforms. */

    glUseProgram(self->name);

    {
        unsigned int factor_l, references_l, weights_l, resolutions_l;
        
        factor_l = glGetUniformLocation (self->name, "factor");
        references_l = glGetUniformLocation (self->name, "references");
        weights_l = glGetUniformLocation (self->name, "weights");
        resolutions_l = glGetUniformLocation (self->name, "resolutions");

        self->locations.intensity = glGetUniformLocation (self->name, "intensity");
        self->locations.planes = glGetUniformLocation (self->name, "planes");

        glUniform1f (factor_l, self->elevation->albedo);

        /* Initialize reference color uniforms. */
        
        for (i = 0 ; i < self->elevation->swatches_n ; i += 1) {
            elevation_SwatchDetail *swatch;

            swatch = &self->elevation->swatches[i];
        
            [self setSamplerUniform: "detail" to: swatch->detail->name atIndex: i];
            
            glUniform2fv (resolutions_l + i, 1, swatch->resolution);
            glUniform3fv (references_l + i, 1, swatch->values);
            glUniform3fv (weights_l + i, 1, swatch->weights);
        }
    }

    [self setSamplerUniform: "deflections" to: deflections];
}

-(void)free
{
    int i;
    
    for (i = 0 ; i < self->elevation->swatches_n ; i += 1) {
        luaL_unref(_L, LUA_REGISTRYINDEX, self->masks[i]);
    }

    free(self->masks);

    luaL_unref(_L, LUA_REGISTRYINDEX, self->reference_1);

    [super free];
}

-(int) _get_element
{
    return 0;
}

-(void) _set_element
{
    Texture *texture;
    int n;
    
    n = lua_tointeger(_L, 2);

    if (n > self->elevation->swatches_n) {
        t_print_warning ("Ignoring configuration specified at index %d which is too large.\n");
        return;
    }

    lua_rawgeti(_L, 3, 1);
    texture = t_testtexture(_L, -1, GL_TEXTURE_2D);
    self->masks[n - 1] = luaL_ref(_L, LUA_REGISTRYINDEX);

    [self setSamplerUniform: "masks" to: texture->name atIndex: n - 1];
}

-(void) draw: (int)frame
{
    Atmosphere *atmosphere;

    glEnable (GL_DEPTH_TEST);
    glEnable (GL_SAMPLE_ALPHA_TO_COVERAGE);
    
    glUseProgram(self->name);

    atmosphere = [Atmosphere instance];
    
    if (atmosphere) {
        glUniform3fv (self->locations.intensity, 1, atmosphere->intensity);
    }
    
    [super draw: frame];

    glDisable (GL_DEPTH_TEST);
    glDisable (GL_SAMPLE_ALPHA_TO_COVERAGE);
}

@end
