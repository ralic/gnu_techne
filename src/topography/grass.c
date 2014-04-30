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
#include "grass.h"
#include "shader.h"

#include "blades.h"

#define N_K 32               /* Number of stiffness samples. */
#define N_S 32               /* Number of blade segments. */

static unsigned int deflections;

@implementation Grass

+(void)initialize
{
    glGenTextures (1, &deflections);

    glBindTexture(GL_TEXTURE_2D, deflections);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, N_S, N_K, 0, GL_RGB,
                 GL_FLOAT, blades);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}

-(void)init
{
    const char *private[] = {"deflections", "intensity", "direction",
                             "direction_w", "triangles"};
    const char *header;
    ShaderMold *shader;

#include "glsl/rand.h"
#include "glsl/vegetation_common.h"
#include "glsl/grass_vertex.h"
#include "glsl/grass_tesselation_control.h"
#include "glsl/grass_tesselation_evaluation.h"
#include "glsl/grass_geometry.h"
#include "glsl/grass_fragment.h"

    [super init];

    /* Create the program. */

    header = _PROFILING ? "#define COLLECT_STATISTICS\n" : "";

    [self unload];

    shader = [ShaderMold alloc];

    [shader initWithHandle: NULL];
    [shader declare: 5 privateUniforms: private];

    [shader addSourceString: glsl_grass_vertex for: T_VERTEX_STAGE];

    [shader add: 3 sourceStrings: (const char *[3]){glsl_rand,
                                                    glsl_vegetation_common,
                                                    glsl_grass_tesselation_control}
            for: T_TESSELATION_CONTROL_STAGE];

    [shader add: 3
            sourceStrings: (const char *[3]){glsl_rand,
                                             glsl_vegetation_common,
                                             glsl_grass_tesselation_evaluation}
            for: T_TESSELATION_EVALUATION_STAGE];

    [shader add: 3
            sourceStrings: (const char *[3]){header,
                                             glsl_vegetation_common,
                                             glsl_grass_geometry}
            for: T_GEOMETRY_STAGE];

    /* Add the fragment source. */

    [shader add: 2
            sourceStrings: (const char *[2]){header, glsl_grass_fragment}
            for: T_FRAGMENT_STAGE];

    [shader link];
    [self load];

    /* Initialize uniforms. */

    self->locations.intensity = glGetUniformLocation (self->name, "intensity");
    self->locations.direction = glGetUniformLocation (self->name, "direction");
    self->locations.direction_w = glGetUniformLocation (self->name,
                                                        "direction_w");

    t_set_sampler(self, "deflections", deflections);
}

-(void) bind
{
    Atmosphere *atmosphere;

    [super bind];

    atmosphere = [Atmosphere instance];

    if (atmosphere) {
        glUniform3fv (self->locations.intensity, 1, atmosphere->intensity);
        glUniform3fv (self->locations.direction, 1, atmosphere->direction);
        glUniform3fv (self->locations.direction_w, 1, atmosphere->direction_w);
    }
}

-(int) _get_triangles
{
    t_pushcount (_L, &self->triangles);

    return 1;
}

-(void) _set_triangles
{
    T_WARN_READONLY;
}

-(void) _set_
{
    if (lua_type(_L, 2) == LUA_TSTRING &&
        !strcmp(lua_tostring(_L, 2), "height")) {
        lua_pushinteger(_L, 1);
        lua_gettable(_L, 3);
        lua_pushinteger(_L, 2);
        lua_gettable(_L, 3);

        [self setCanopy: lua_tonumber(_L, -1) + lua_tonumber(_L, -2)];

        lua_pop(_L, 2);
    }

    [super _set_];
}

-(void) draw: (int)frame
{
    if (_PROFILING) {
        unsigned int n;

        /* Count two triangles per segment. */

        t_get_and_reset_counter(self, "segments", &n);
        t_add_count_sample(&self->triangles, 2 * n);
    }

    [super draw: frame];
}

@end
