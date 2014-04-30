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
#include "seeds.h"
#include "shader.h"

@implementation Seeds

-(void)init
{
    const char *private[] = {"intensity"};
    ShaderMold *shader;

#include "glsl/rand.h"
#include "glsl/vegetation_common.h"
#include "glsl/seeds_vertex.h"
#include "glsl/seeds_tesselation_control.h"
#include "glsl/seeds_tesselation_evaluation.h"
#include "glsl/seeds_fragment.h"

    [super init];

    /* Create the program. */

    [self unload];

    shader = [ShaderMold alloc];

    [shader initWithHandle: NULL];
    [shader declare: 1 privateUniforms: private];
    [shader addSourceString: glsl_seeds_vertex for: T_VERTEX_STAGE];

    [shader add: 3 sourceStrings: (const char *[3]){glsl_rand,
                                                    glsl_vegetation_common,
                                                    glsl_seeds_tesselation_control}
                        for: T_TESSELATION_CONTROL_STAGE];

    [shader add: 4
            sourceStrings: (const char *[4]){glsl_rand,
                                             glsl_vegetation_common,
                                             glsl_seeds_tesselation_evaluation}
            for: T_TESSELATION_EVALUATION_STAGE];

    /* Add the fragment source. */

    [shader add: 2
            sourceStrings: (const char *[2]){glsl_seeds_fragment}
            for: T_FRAGMENT_STAGE];

    [shader link];
    [self load];

    /* Initialize uniforms. */

    glUseProgram(self->name);
    self->locations.intensity = glGetUniformLocation (self->name, "intensity");
}

-(void) bind
{
    Atmosphere *atmosphere;

    [super bind];

    atmosphere = [Atmosphere instance];

    if (atmosphere) {
        glUniform3fv (self->locations.intensity, 1, atmosphere->intensity);
    }
}

-(void) _set_
{
    if (lua_type(_L, 2) == LUA_TSTRING &&
        !strcmp(lua_tostring(_L, 2), "height")) {
        [self setCanopy: lua_tonumber(_L, 3)];
    }

    [super _set_];
}

@end
