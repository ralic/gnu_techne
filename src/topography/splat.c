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
    static const char *list[1] = {"palette"};
    
    [super init];
    [self set: 1 prerequisites: list];

    self->albedo = 1;
    self->separation = 1;
    self->pigments_n = 0;   
    self->pigments = NULL;
}

-(void) free
{
    int i;
    
    /* Free the current resources. */
	
    if (self->pigments) {
        for (i = 0 ; i < self->pigments_n ; i += 1) {
            luaL_unref (_L, LUA_REGISTRYINDEX, self->pigments[i].reference);
        }
        
        free (self->pigments);
    }
        
    [super free];
}

-(int) _get_albedo
{
    lua_pushnumber (_L, self->albedo);

    return 1;
}

-(int) _get_separation
{
    lua_pushnumber (_L, self->separation);

    return 1;
}

-(void) _set_albedo
{
    self->albedo = lua_tonumber (_L, -1);
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
            splat_Pigment *pigment;

            pigment = &self->pigments[i]; 
            lua_createtable (_L, 3, 0);

            /* The texture. */
            
            lua_rawgeti(_L, LUA_REGISTRYINDEX, pigment->reference);
            lua_rawseti(_L, -2, 1);

            /* The resolution. */
            
            lua_createtable (_L, 2, 0);

            for (j = 0; j < 2 ; j += 1) {
                lua_pushnumber(_L, pigment->values[j]);
                lua_rawseti(_L, -2, j + 1);
            }

            lua_rawseti(_L, -2, 2);

            /* The color. */
            
            lua_createtable (_L, 3, 0);

            for (j = 0; j < 3 ; j += 1) {
                if (pigment->values[j + 5] > 0) {
                    lua_pushnumber(_L, pigment->values[j + 2]);
                } else {
                    lua_pushnil(_L);
                }
                
                lua_rawseti(_L, -2, j + 1);
            }

            lua_rawseti(_L, -2, 3);            
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
        self->pigments = (splat_Pigment *)calloc (n, sizeof (splat_Pigment));
    } else {
        if (self->pigments_n != n) {
            t_print_error("Once set, the splatting palette size cannot change.\n");
            abort();
        }

        /* Free the current resources. */
	
        for (i = 0 ; i < self->pigments_n ; i += 1) {
            luaL_unref (_L, LUA_REGISTRYINDEX, self->pigments[i].reference);
        }
    }
        
    /* And load all pigments. */
	    
    for (j = 0 ; j < self->pigments_n ; j += 1) {
        lua_rawgeti(_L, 3, j + 1);

        if (!lua_isnil (_L, -1)) {
            splat_Pigment *pigment;
            Texture *texture;

            pigment = &self->pigments[j]; 

            /* The detail map pixels. */
		
            lua_rawgeti (_L, -1, 1);

            texture = t_testtexture (_L, -1, GL_TEXTURE_2D);
            
            if (texture) {
                pigment->texture = texture->name;
            } else {
                pigment->texture = 0;
            }
            
            pigment->reference = luaL_ref (_L, LUA_REGISTRYINDEX);
		
            /* The resolution. */
		
            lua_rawgeti (_L, -1, 2);

            for (i = 0 ; i < 2 ; i += 1) {
                lua_pushinteger (_L, i + 1);
                lua_gettable (_L, -2);
                pigment->values[i] = lua_tonumber (_L, -1);
                lua_pop(_L, 1);
            }

            lua_pop(_L, 1);
		
            /* The HSV color target. */
		
            lua_rawgeti (_L, -1, 3);

            if (lua_istable (_L, -1)) {
                for (i = 0 ; i < 3 ; i += 1) {
                    lua_pushinteger (_L, i + 1);
                    lua_gettable (_L, -2);

                    if (lua_isnumber(_L, -1)) {
                        pigment->values[i + 2] = lua_tonumber (_L, -1);
                        pigment->values[i + 5] = 1;
                    } else {
                        pigment->values[i + 2] = 0;
                        pigment->values[i + 5] = 0;
                    }			    
			
                    lua_pop(_L, 1);
                }
            }

            lua_pop(_L, 1);
        }

        lua_pop(_L, 1);
    }

    if (self->name == 0) {
        const char *private[6] = {"base", "detail", "offset", "scale",
                                  "power", "matrices"};
        char *header;
        ShaderMold *shader;
        
#include "glsl/splat_vertex.h"	
#include "glsl/splat_fragment.h"	

        asprintf (&header, "const int N = %d;\n", self->pigments_n);
        
	shader = [ShaderMold alloc];
        
        [shader initWithHandle: NULL];
        [shader declare: 6 privateUniforms: private];
	[shader addSource: glsl_splat_vertex for: T_VERTEX_STAGE];
	[shader add: 2 sourceStrings: (const GLchar *[2]){header, glsl_splat_fragment} for: T_FRAGMENT_STAGE];
	[shader link];

        [self load];

        /* Get uniform locations. */
        
        self->locations.base = glGetUniformLocation (self->name, "base");
        self->locations.detail = glGetUniformLocation (self->name, "detail");
        self->locations.power = glGetUniformLocation (self->name, "power");
        self->locations.matrices = glGetUniformLocation (self->name, "matrices");

        self->locations.turbidity = glGetUniformLocation (self->name, "turbidity");
        self->locations.factor = glGetUniformLocation (self->name, "factor");
        self->locations.beta_p = glGetUniformLocation (self->name, "beta_p");
        self->locations.beta_r = glGetUniformLocation (self->name, "beta_r");
        self->locations.direction = glGetUniformLocation (self->name, "direction");
        self->locations.intensity = glGetUniformLocation (self->name, "intensity");
        
        /* _TRACE ("%d, %d, %d, %d\n", self->locations.base, self->locations.detail, self->locations.power, self->locations.matrices); */
        
        /* Splatting-related uniforms will remain constant as the
         * program is only used by this node so all uniform values can
         * be loaded beforehand. */

        glUseProgram(self->name);
        
        glUniform1i(self->locations.base, 0);
        glUniform1f(self->locations.power, self->separation);

        for (i = 0 ; i < self->pigments_n ; i += 1) {
            splat_Pigment *pigment;

            pigment = &self->pigments[i]; 

            /* Detail texture samplers are assigned consecutive
             * texture units, starting from 1 since 0 is used by the
             * base texture. */
            
            glUniform1i(self->locations.detail + i, i + 1);

            /* Load the coeffcient matrix for the pigment. */
            
            {
                float M[9] = {
                    pigment->values[2], 
                    pigment->values[3],
                    pigment->values[4],

                    pigment->values[5],
                    pigment->values[6],
                    pigment->values[7],

                    0.625 / pigment->values[0],
                    0.625 / pigment->values[1],
                    0
                };

                glUniformMatrix3fv (self->locations.matrices + i, 1,
                                    GL_FALSE, M);
            }
        }        
    }
}

-(void) draw: (int)frame
{
    Atmosphere *atmosphere;
    int i;

    atmosphere = [Atmosphere instance];
    
    glEnable (GL_CULL_FACE);
    glEnable (GL_DEPTH_TEST);

    /* Bind the program and all textures. */
    
    glUseProgram(self->name);

    glUniform1f (self->locations.factor, self->albedo);
    
    if (atmosphere) {
        glUniform3fv (self->locations.direction, 1, atmosphere->direction);
        glUniform3fv (self->locations.intensity, 1, atmosphere->intensity);
        glUniform3fv (self->locations.beta_r, 1, atmosphere->rayleigh);
        glUniform1f (self->locations.beta_p, atmosphere->mie);
        glUniform1f (self->locations.turbidity, atmosphere->turbidity);
    }
    
    for (i = 0 ; i < self->pigments_n ; i += 1) {
        glActiveTexture(GL_TEXTURE1 + i);
        glBindTexture(GL_TEXTURE_2D, self->pigments[i].texture);
    }
    
    [super draw: frame];

    glDisable (GL_DEPTH_TEST);
    glDisable (GL_CULL_FACE);
}

@end
