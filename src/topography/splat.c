/* Copyright (C) 2012 Papavasileiou Dimitris
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
#include "splatting.h"
#include "splat.h"

@implementation Splat

-(void) init
{
    const char *private[] = {"base", "detail", "offset", "scale",
                             "separation", "factor", "references", "weights",
                             "resolutions"};

    ShaderMold *shader;

#include "glsl/color.h"
#include "glsl/splat_vertex.h"
#include "glsl/splat_fragment.h"

    /* Make a reference to the elevation to make sure it's not
     * collected. */

    self->elevation = t_tonode (_L, -1);
    self->reference_1 = luaL_ref (_L, LUA_REGISTRYINDEX);

    [super init];

    shader = [ShaderMold alloc];

    [shader initWithHandle: NULL];
    [shader declare: 9 privateUniforms: private];
    [shader addSourceString: glsl_splat_vertex for: T_VERTEX_STAGE];

    add_splatting_sources(self->elevation, shader, T_FRAGMENT_STAGE);
    [shader add: 2
            sourceStrings: (const char *[2]){glsl_color, glsl_splat_fragment}
            for: T_FRAGMENT_STAGE];

    [shader link];
    [self load];

    /* Get uniform locations. */

    self->locations.turbidity = glGetUniformLocation (self->name, "turbidity");
    self->locations.beta_p = glGetUniformLocation (self->name, "beta_p");
    self->locations.beta_r = glGetUniformLocation (self->name, "beta_r");
    self->locations.direction = glGetUniformLocation (self->name, "direction");
    self->locations.intensity = glGetUniformLocation (self->name, "intensity");

    set_splatting_uniforms(self->elevation, self);
}

-(void)free
{
    luaL_unref(_L, LUA_REGISTRYINDEX, self->reference_1);

    [super free];
}

-(void) bind
{
    Atmosphere *atmosphere;

    [super bind];

    atmosphere = [Atmosphere instance];

    /* Set atmospheric parameters. */

    if (atmosphere) {
        glUniform3fv (self->locations.direction, 1, atmosphere->direction);
        glUniform3fv (self->locations.intensity, 1, atmosphere->intensity);
        glUniform3fv (self->locations.beta_r, 1, atmosphere->rayleigh);
        glUniform1f (self->locations.beta_p, atmosphere->mie);
        glUniform1f (self->locations.turbidity, atmosphere->turbidity);
    }
}

-(void) draw: (int)frame
{
    glEnable (GL_CULL_FACE);
    glEnable (GL_DEPTH_TEST);

    glUseProgram(self->name);

    [self bind];
    [super draw: frame];

    glDisable (GL_DEPTH_TEST);
    glDisable (GL_CULL_FACE);
}

@end
