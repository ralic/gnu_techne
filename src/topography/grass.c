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
#include "grass.h"
#include "shader.h"

@implementation Grass
-(void)init
{
    static const char *list[1] = {"palette"};
    
    [super init];
    [self set: 1 prerequisites: list];
    
    self->separation = 1;
    self->pigments_n = 0;   
    self->pigments = NULL;
}

-(int) _get_separation
{
    lua_pushnumber (_L, self->separation);

    return 1;
}

-(void) _set_separation
{
    self->separation = lua_tonumber (_L, -1);
}

-(int) _get_palette
{
    int i, j;
    
    if (self->pigments_n > 0) {
        lua_createtable (_L, self->pigments_n, 0);

        for (i = 0; i < self->pigments_n ; i += 1) {
            grass_Pigment *pigment;

            pigment = &self->pigments[i]; 
            lua_createtable (_L, 2, 0);

            /* The color. */

            array_createarray (_L, ARRAY_TFLOAT, pigment->color, 1, 3);
            lua_rawseti(_L, -2, 1);

            /* The reference. */
            
            lua_createtable (_L, 3, 0);

            for (j = 0; j < 3 ; j += 1) {
                if (pigment->values[j + 3] > 0) {
                    lua_pushnumber(_L, pigment->values[j]);
                } else {
                    lua_pushnil(_L);
                }
                
                lua_rawseti(_L, -2, j + 1);
            }

            lua_rawseti(_L, -2, 2);            
            lua_rawseti(_L, -2, i + 1);            
        }
    } else {
        lua_pushnil(_L);
    }
    
    return 1;
}

-(void) _set_palette
{
    int i, j, n;
	
    n = lua_rawlen (_L, 3);

    if (self->pigments_n == 0) {
        /* Allocate resources. */
	    
        self->pigments_n = n;
        self->pigments = (grass_Pigment *)calloc (n, sizeof (grass_Pigment));
    } else {
        if (self->pigments_n != n) {
            t_print_error("Once set, the vegetation palette size cannot change.\n");
            abort();
        }
    }
        
    /* And load all pigments. */
	    
    for (j = 0 ; j < self->pigments_n ; j += 1) {
        lua_rawgeti(_L, 3, j + 1);

        if (!lua_isnil (_L, -1)) {
            grass_Pigment *pigment;
            array_Array *array;

            pigment = &self->pigments[j]; 

            /* The color. */
		
            lua_rawgeti (_L, -1, 1);

            array = array_testcompatible (_L, -1,
                                          ARRAY_RANK | ARRAY_TYPE | ARRAY_SIZE,
                                          ARRAY_TFLOAT, 1, 3);

            if (array) {
                memcpy (pigment->color, array->values.any, 3 * sizeof(float));
            }
            
            lua_pop(_L, 1);
		
            /* The HSV color reference. */
		
            lua_rawgeti (_L, -1, 2);

            if (lua_istable (_L, -1)) {
                for (i = 0 ; i < 3 ; i += 1) {
                    lua_pushinteger (_L, i + 1);
                    lua_gettable (_L, -2);

                    if (lua_isnumber(_L, -1)) {
                        pigment->values[i] = lua_tonumber (_L, -1);
                        pigment->values[i + 3] = 1;
                    } else {
                        pigment->values[i] = 0;
                        pigment->values[i + 3] = 0;
                    }			    
			
                    lua_pop(_L, 1);
                }
            }

            lua_pop(_L, 1);
        }

        lua_pop(_L, 1);
    }

    if (self->name == 0) {
        const char *private[7] = {"base", "colors", "offset", "scale",
                                  "power", "references", "weights"};
        char *header;
        ShaderMold *shader;
        
#include "glsl/splat_common.h"
#include "glsl/grass_vertex.h"
#include "glsl/grass_geometry.h"
#include "glsl/grass_fragment.h"

        asprintf (&header, "const int N = %d;\n", self->pigments_n);
        
	shader = [ShaderMold alloc];
        
        [shader initWithHandle: NULL];
        [shader declare: 7 privateUniforms: private];
	[shader add: 3 sourceStrings: (const GLchar *[3]){header, glsl_splat_common, glsl_grass_vertex} for: T_VERTEX_STAGE];
	[shader add: 2 sourceStrings: (const GLchar *[2]){header, glsl_grass_geometry} for: T_GEOMETRY_STAGE];
	[shader addSource: glsl_grass_fragment for: T_FRAGMENT_STAGE];
	[shader link];

        [self load];

        /* Get uniform locations. */
        
        self->locations.base = glGetUniformLocation (self->name, "base");
        self->locations.colors = glGetUniformLocation (self->name, "colors");
        self->locations.power = glGetUniformLocation (self->name, "power");
        self->locations.references = glGetUniformLocation (self->name, "references");
        self->locations.weights = glGetUniformLocation (self->name, "weights");
        
        /* _TRACE ("%d, %d, %d, %d\n", self->locations.base, self->locations.detail, self->locations.power, self->locations.matrices); */

        /* Vegetation-related uniforms will remain constant as the
         * program is only used by this node so all uniform values can
         * be loaded beforehand. */

        glUseProgram(self->name);
        
        glUniform1i(self->locations.base, 0);
        glUniform1f(self->locations.power, self->separation);

        for (i = 0 ; i < self->pigments_n ; i += 1) {
            grass_Pigment *pigment;

            pigment = &self->pigments[i];

            glUniform3fv(self->locations.colors + i, 1, pigment->color);
            glUniform3fv (self->locations.references + i, 1,
                          &pigment->values[0]);
            glUniform3fv (self->locations.weights + i, 1,
                          &pigment->values[3]);
        }
    }
}

-(void) draw: (int)frame
{
    glEnable (GL_CULL_FACE);
    glEnable (GL_DEPTH_TEST);
    
    glUseProgram(self->name);
    
    [super draw: frame];

    glDisable (GL_DEPTH_TEST);
    glDisable (GL_CULL_FACE);
}

@end
