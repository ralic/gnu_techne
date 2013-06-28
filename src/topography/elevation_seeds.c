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
#include <math.h>
#include <lua.h>
#include <lauxlib.h>

#include "gl.h"

#include "algebra.h"
#include "techne.h"
#include "roam.h"
#include "seeding.h"
#include "vegetation.h"
#include "elevation.h"

//#define RASTERTIZER_DISCARD   

@implementation ElevationSeeds

-(void) init
{
    Elevation *elevation;

    /* Make a reference to the elevation to make sure it's not
     * collected. */

    elevation = t_tonode (_L, -1);
    self->reference = luaL_ref (_L, LUA_REGISTRYINDEX);
    
    [super initWithMode: GL_POINTS];

    self->context = &elevation->context;
    self->seeding.horizon = -100;

    initialize_seeding(&self->seeding);

    /* Create the VBO. */
    
    glGenBuffers(1, &self->buffer);
}

-(void) free
{
    glDeleteBuffers(1, &self->buffer);
    luaL_unref (_L, LUA_REGISTRYINDEX, self->reference);
        
    [super free];
}

-(void) meetParent: (Shader *)parent
{
    int i;
    
    [super meetParent: parent];
    
    if (![parent isKindOf: [Vegetation class]]) {
	t_print_warning("%s node has no Vegetation parent.\n",
			[self name]);
	
	return;
    }

    /* Bind the VBOs into the VAO. */
    
    glBindVertexArray(self->name);

    glBindBuffer(GL_ARRAY_BUFFER, self->buffer);

    i = glGetAttribLocation(parent->name, "apex");
    glVertexAttribPointer(i, 3, GL_FLOAT, GL_FALSE, SEED_SIZE, (void *)0);
    glEnableVertexAttribArray(i);

    i = glGetAttribLocation(parent->name, "left");
    glVertexAttribPointer(i, 3, GL_FLOAT, GL_FALSE, SEED_SIZE, (void *)(3 * sizeof(float)));
    glEnableVertexAttribArray(i);

    i = glGetAttribLocation(parent->name, "right");
    glVertexAttribPointer(i, 3, GL_FLOAT, GL_FALSE, SEED_SIZE, (void *)(6 * sizeof(float)));
    glEnableVertexAttribArray(i);

    /* Query uniform locations. */
    
    self->locations.scale = glGetUniformLocation(parent->name, "scale");
    self->locations.offset = glGetUniformLocation(parent->name, "offset");
    self->locations.planes = glGetUniformLocation(parent->name, "planes");

    {
        unsigned int i;
        float M[16];

        copy_setup_transform(self->context, M);
        
        i = glGetUniformLocation(parent->name, "setup");
        glUniformMatrix4fv (i, 1, GL_FALSE, M);
    }

    self->units.base = [parent getUnitForSamplerUniform: "base"];
}

-(void) draw: (int)frame
{
    float M[16], P[16], T[16], planes[6][4];
    roam_Tileset *tiles;
    int i, j;

    tiles = &self->context->tileset;

    t_push_modelview (self->matrix, T_MULTIPLY);

    /* Seed the vegetation. */
    
    optimize_geometry(self->context, frame);
    begin_seeding (&self->seeding, self->context);
    
    glBindBuffer(GL_ARRAY_BUFFER, self->buffer);
    glBindVertexArray(self->name);
    glActiveTexture(GL_TEXTURE0 + self->units.base);

    glPatchParameteri(GL_PATCH_VERTICES, 1);
    glUniform1f(self->locations.scale, ldexpf(1, -tiles->depth));

    t_copy_modelview(M);
    t_copy_projection(P);
    t_concatenate_4T(T, P, M);
    calculate_view_frustum(planes, T);
    glUniform4fv(self->locations.planes, 5, (float *)planes);
    
    glEnable (GL_MULTISAMPLE);

#ifdef RASTERTIZER_DISCARD
    glEnable (GL_RASTERIZER_DISCARD);
#endif

    for (i = 0 ; i < tiles->size[0] ; i += 1) {    
	for (j = 0 ; j < tiles->size[1] ; j += 1) {
            seeding_Bin *b;
            int k, l, n;

            l = i * tiles->size[1] + j;
            n = seed_tile(l);
            
            for (k = 0, b = &self->seeding.bins[0];
                 k < BINS_N;
                 k += 1, b += 1) {

                if(b->fill > 0) {
                    glUniform2f(self->locations.offset, j, i);
                    glBindTexture(GL_TEXTURE_2D, tiles->imagery[l]);
                    glPointSize(1);
                    
                    /* printf ("%f\n", b->center); */
                    glBufferData (GL_ARRAY_BUFFER, n * SEED_SIZE, NULL, GL_STREAM_DRAW);
                    glBufferSubData (GL_ARRAY_BUFFER, 0, b->fill * SEED_SIZE, b->buffer);
                    
                    glDrawArraysInstanced(GL_PATCHES, 0, b->fill, round(b->center));
                }
            }
	}
    }

    /* _TRACE ("error: %f\n", seeding->error); */

    glDisable (GL_MULTISAMPLE);
    
#ifdef RASTERTIZER_DISCARD
    glDisable (GL_RASTERIZER_DISCARD);
#endif

    finish_seeding();
    t_pop_modelview ();
    
    [super draw: frame];
}

-(int) _get_density
{
    lua_pushnumber(_L, self->seeding.density);

    return 1;
}

-(void) _set_density
{
    self->seeding.density = lua_tonumber(_L, 3);
}

-(int) _get_bias
{
    lua_pushnumber(_L, self->seeding.bias);

    return 1;
}

-(void) _set_bias
{
    self->seeding.bias = lua_tonumber(_L, 3);
}

-(int) _get_bins
{
    int i;
    
    lua_createtable (_L, BINS_N, 0);

    for (i = 0 ; i < BINS_N ; i += 1) {
        seeding_Bin *b;

        b = &self->seeding.bins[i];
        
        lua_createtable (_L, 3, 0);
        lua_pushnumber (_L, b->center);
        lua_rawseti (_L, -2, 1);
        lua_pushinteger (_L, b->total);
        lua_rawseti (_L, -2, 2);
        lua_pushinteger (_L, b->capacity);
        lua_rawseti (_L, -2, 3);

        lua_rawseti (_L, -2, i + 1);
    }
    
    return 1;
}

-(void) _set_bins
{
    T_WARN_READONLY;
}

-(int) _get_triangles
{
    int i;
    
    lua_createtable (_L, 2, 0);

    for (i = 0 ; i < 2 ; i += 1) {
        lua_pushnumber (_L, self->seeding.triangles_n[i]);
        lua_rawseti (_L, -2, i + 1);
    }
    
    return 1;
}

-(void) _set_triangles
{
    T_WARN_READONLY;
}

-(int) _get_horizon
{
    lua_pushnumber (_L, -self->seeding.horizon);
    
    return 1;
}

-(void) _set_horizon
{
    self->seeding.horizon = -lua_tonumber (_L, 3);
}

-(int) _get_error
{
    lua_pushnumber (_L, self->seeding.error);
    
    return 1;
}

-(void) _set_error
{
    T_WARN_READONLY;
}

@end
