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
#include "swatch.h"
#include "atmosphere.h"
#include "splat.h"

@implementation Splat

-(void) init
{
    char *header;
    const char *private[] = {"base", "detail", "offset", "scale",
                             "power", "factor", "references", "weights",
                             "resolutions"};
    
    ShaderMold *shader;
    Node *child;
    int i;
        
#include "glsl/color.h"	
#include "glsl/splat_vertex.h"	
#include "glsl/splat_fragment.h"	

    /* Make a reference to the elevation to make sure it's not
     * collected. */

    self->elevation = t_tonode (_L, -1);
    self->reference_1 = luaL_ref (_L, LUA_REGISTRYINDEX);

    [super init];

    asprintf (&header, "const int N = %d;\n", self->elevation->swatches);
        
    shader = [ShaderMold alloc];
        
    [shader initWithHandle: NULL];
    [shader declare: 9 privateUniforms: private];
    [shader addSourceString: glsl_splat_vertex for: T_VERTEX_STAGE];
    [shader add: 3 sourceStrings: (const char *[3]){header, glsl_color, glsl_splat_fragment} for: T_FRAGMENT_STAGE];
    [shader link];

    [self load];

    /* Get uniform locations. */

    self->locations.power = glGetUniformLocation (self->name, "power");
    self->locations.references = glGetUniformLocation (self->name, "references");
    self->locations.weights = glGetUniformLocation (self->name, "weights");
    self->locations.resolutions = glGetUniformLocation (self->name, "resolutions");

    self->locations.turbidity = glGetUniformLocation (self->name, "turbidity");
    self->locations.factor = glGetUniformLocation (self->name, "factor");
    self->locations.beta_p = glGetUniformLocation (self->name, "beta_p");
    self->locations.beta_r = glGetUniformLocation (self->name, "beta_r");
    self->locations.direction = glGetUniformLocation (self->name, "direction");
    self->locations.intensity = glGetUniformLocation (self->name, "intensity");
        
    /* Splatting-related uniforms will remain constant as the
     * program is only used by this node so all uniform values can
     * be loaded beforehand. */

    glUseProgram(self->name);
    glUniform1f(self->locations.power, self->elevation->separation);
    glUniform1f (self->locations.factor, self->elevation->albedo);

    /* Initialize reference color uniforms. */
    
    for (child = self->elevation->down, i = 0 ; child ; child = child->right) {
        if ([child isKindOf: [Swatch class]]) {
            Swatch *swatch = (Swatch *)child;

            [self setSamplerUniform: "detail" to: swatch->detail->name atIndex: i];
            
            glUniform2fv (self->locations.resolutions + i, 1,
                          swatch->resolutions);
            glUniform3fv (self->locations.references + i, 1, swatch->values);
            glUniform3fv (self->locations.weights + i, 1, swatch->weights);

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
