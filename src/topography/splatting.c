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

#include <math.h>

#include <lua.h>
#include <lauxlib.h>

#include "splatting.h"
#include "glsl/splatting.h"

void add_splatting_sources(Elevation *elevation, ShaderMold *shader,
                           t_ProcessingStage stage)
{
    int i, j, k, l, n;

    n = elevation->swatches_n;

    /* Compile the splatting-related parameters. */

    lua_createtable(_L, 0, 2);
    lua_pushinteger(_L, n);
    lua_setfield (_L, -2, "swatches_n");

    lua_createtable(_L, n, 0);
        
    for (i = 0, j = 0, l = 0 ; i < n ; i += 1) {
        k = elevation->bands_n[i];
            
        lua_createtable(_L, 2, 0);

        lua_pushinteger(_L, j);
        lua_rawseti(_L, -2, 1);

        lua_pushinteger(_L, k);
        lua_rawseti(_L, -2, 2);

        lua_rawseti(_L, -2, i + 1);

        j += k;
        l = k > l ? k : l;
    }

    lua_setfield (_L, -2, "bands");

    lua_pushinteger(_L, j);
    lua_setfield (_L, -2, "bands_n");

    lua_pushinteger(_L, l);
    lua_setfield (_L, -2, "bands_max");

    assert(t_rendertemplate(_L, glsl_splatting) == LUA_OK);

    [shader addSourceString: lua_tostring(_L, -1) for: stage];
    /* _TRACE ("%s\n", source); */

    lua_pop(_L, 1);
}

void set_splatting_uniforms(Elevation *elevation, Shader *shader)
{
    roam_Tileset *tiles;
    double q;
    unsigned int separation_l, references_l, weights_l, resolutions_l;
    unsigned int factor_l, scale_l;
    int i, j, k, n;

    n = shader->name;
    
    /* Get uniform locations. */

    scale_l = glGetUniformLocation(n, "scale");
    separation_l = glGetUniformLocation (n, "separation");
    references_l = glGetUniformLocation (n, "references");
    weights_l = glGetUniformLocation (n, "weights");
    resolutions_l = glGetUniformLocation (n, "resolutions");
    factor_l = glGetUniformLocation (n, "factor");

    /* Bind the program and set uniforms. */

    glUseProgram(n);
    glUniform1f (separation_l, elevation->separation);
    glUniform1f (factor_l, elevation->albedo);

    tiles = &elevation->context.tileset;

    q = ldexpf(1, -tiles->depth);
    glUniform2f(scale_l,
                q / tiles->resolution[0], q / tiles->resolution[1]);

    /* Initialize reference color uniforms. */
    
    for (i = 0, k = 0 ; i < elevation->swatches_n ; i += 1) {
        elevation_SwatchDetail *swatch;
        
        swatch = &elevation->swatches[i];
        
        for (j = 0 ; j < elevation->bands_n[i] ; j += 1, k += 1) {
            t_set_indexed_sampler(shader, "detail",
                                  swatch->detail[j]->name, k);

            glUniform2fv (resolutions_l + k, 1, swatch->resolutions[j]);
        }

        glUniform3fv (references_l + i, 1, swatch->values);
        glUniform3fv (weights_l + i, 1, swatch->weights);
    }
}
