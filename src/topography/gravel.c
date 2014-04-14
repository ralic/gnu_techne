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
#include "gravel.h"
#include "shader.h"

@implementation Gravel

-(void)init
{
    const char *private[] = {"intensity", "direction", "direction_w"};
    const char *header;
    ShaderMold *shader;
        
#include "glsl/rand.h"
#include "glsl/vegetation_common.h"
#include "glsl/gravel_vertex.h"
#include "glsl/gravel_tesselation_control.h"
#include "glsl/gravel_tesselation_evaluation.h"
#include "glsl/gravel_geometry.h"
#include "glsl/gravel_fragment.h"
    
    [super init];

    /* Create the program. */

    header = _PROFILING ? "#define COLLECT_STATISTICS\n" : "";

    [self unload];
    
    shader = [ShaderMold alloc];
        
    [shader initWithHandle: NULL];
    [shader declare: 1 privateUniforms: private];
    
    [shader addSourceString: glsl_gravel_vertex for: T_VERTEX_STAGE];

    [shader add: 3 sourceStrings: (const char *[3]){glsl_rand,
                                                    glsl_vegetation_common,
                                                    glsl_gravel_tesselation_control}
            for: T_TESSELATION_CONTROL_STAGE];

    [shader add: 4
            sourceStrings: (const char *[4]){header,
                                             glsl_rand,
                                             glsl_vegetation_common,
                                             glsl_gravel_tesselation_evaluation}
            for: T_TESSELATION_EVALUATION_STAGE];

    [shader add: 2
            sourceStrings: (const char *[2]){glsl_vegetation_common,
                                             glsl_gravel_geometry}
            for: T_GEOMETRY_STAGE];
    
    /* Add the fragment source. */
    
    [shader add: 2
            sourceStrings: (const char *[2]){header, glsl_gravel_fragment}
            for: T_FRAGMENT_STAGE];

    [shader link];
    [self load];

    /* Initialize uniforms. */

    self->locations.intensity = glGetUniformLocation (self->name, "intensity");
    self->locations.direction = glGetUniformLocation (self->name, "direction");
    self->locations.direction_w = glGetUniformLocation (self->name,
                                                        "direction_w");
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
    [super _get_];

    /* Convert segments to triangles and normalize. */
    
    lua_pushnumber(_L, lua_tonumber(_L, -1) / self->core.frames * 2);

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
        [self setCanopy: lua_tonumber(_L, 3)];
    }

    [super _set_];
}

@end
