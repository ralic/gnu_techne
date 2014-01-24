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
#include "atmosphere.h"
#include "splat.h"

@implementation Splat

-(void) init
{
    char *header;
    const char *private[] = {"base", "detail", "offset", "scale",
                             "separation", "factor", "references", "weights",
                             "resolutions"};

    ShaderMold *shader;
    unsigned int separation_l, references_l, weights_l, resolutions_l, factor_l;
    int i, j;

    
#include "glsl/color.h"	
#include "glsl/splatting.h"
#include "glsl/splat_vertex.h"	
#include "glsl/splat_fragment.h"	

    /* Make a reference to the elevation to make sure it's not
     * collected. */

    self->elevation = t_tonode (_L, -1);
    self->reference_1 = luaL_ref (_L, LUA_REGISTRYINDEX);

    [super init];

    asprintf (&header, "const int N = %d;\n", self->elevation->swatches_n);
        
    shader = [ShaderMold alloc];
        
    [shader initWithHandle: NULL];
    [shader declare: 9 privateUniforms: private];
    [shader addSourceString: glsl_splat_vertex for: T_VERTEX_STAGE];
    [shader add: 4 sourceStrings: (const char *[4]){header, glsl_color, glsl_splatting, glsl_splat_fragment} for: T_FRAGMENT_STAGE];
    [shader link];

    [self load];

    /* Get uniform locations. */

    separation_l = glGetUniformLocation (self->name, "separation");
    references_l = glGetUniformLocation (self->name, "references");
    weights_l = glGetUniformLocation (self->name, "weights");
    resolutions_l = glGetUniformLocation (self->name, "resolutions");
    factor_l = glGetUniformLocation (self->name, "factor");

    self->locations.turbidity = glGetUniformLocation (self->name, "turbidity");
    self->locations.beta_p = glGetUniformLocation (self->name, "beta_p");
    self->locations.beta_r = glGetUniformLocation (self->name, "beta_r");
    self->locations.direction = glGetUniformLocation (self->name, "direction");
    self->locations.intensity = glGetUniformLocation (self->name, "intensity");
        
    /* Splatting-related uniforms will remain constant as the
     * program is only used by this node so all uniform values can
     * be loaded beforehand. */

    glUseProgram(self->name);
    glUniform1f (separation_l, self->elevation->separation);
    glUniform1f (factor_l, self->elevation->albedo);

    /* Initialize reference color uniforms. */
    
    for (i = 0 ; i < self->elevation->swatches_n ; i += 1) {
        elevation_SwatchDetail *swatch;

        swatch = &self->elevation->swatches[i];

        for (j = 0 ; j < 3 ; j += 1) {
            [self setSamplerUniform: "detail"
                                 to: swatch->detail[j]->name atIndex: 3 * i + j];

            glUniform2fv (resolutions_l + 3 * i + j, 1,
                          swatch->resolutions[j]);
        }

        glUniform3fv (references_l + i, 1, swatch->values);
        glUniform3fv (weights_l + i, 1, swatch->weights);
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

    glEnable (GL_CULL_FACE);
    glEnable (GL_DEPTH_TEST);

    /* Bind the program and all textures. */
    
    glUseProgram(self->name);
    
    atmosphere = [Atmosphere instance];
    
    if (atmosphere) {
        glUniform3fv (self->locations.direction, 1, atmosphere->direction);
        glUniform3fv (self->locations.intensity, 1, atmosphere->intensity);
        glUniform3fv (self->locations.beta_r, 1, atmosphere->rayleigh);
        glUniform1f (self->locations.beta_p, atmosphere->mie);
        glUniform1f (self->locations.turbidity, atmosphere->turbidity);
    }

    [super draw: frame];

    glDisable (GL_DEPTH_TEST);
    glDisable (GL_CULL_FACE);
}

@end
