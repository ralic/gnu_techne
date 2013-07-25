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
    self->seeding.density = 10000;
    self->seeding.ceiling = 1000;
    self->seeding.clustering[0] = 1.0 / 0.0;
    self->seeding.clustering[1] = 0.0;

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
    self->locations.clustering = glGetUniformLocation(parent->name, "clustering");

    self->units.base = [parent getUnitForSamplerUniform: "base"];
}

-(void) draw: (int)frame
{
    double q;
    roam_Tileset *tiles;
    seeding_Bin *b;
    int i, j;

    tiles = &self->context->tileset;

    t_push_modelview (self->matrix, T_MULTIPLY);

    /* Seed the vegetation. */
    
    begin_seeding (&self->seeding, self->context);
    
    glBindBuffer(GL_ARRAY_BUFFER, self->buffer);
    glBindVertexArray(self->name);
    glActiveTexture(GL_TEXTURE0 + self->units.base);

    glPatchParameteri(GL_PATCH_VERTICES, 1);
    
    q = ldexpf(1, -tiles->depth);
    glUniform2f(self->locations.scale,
                q / tiles->resolution[0], q / tiles->resolution[1]);
    
    glEnable (GL_MULTISAMPLE);

#ifdef RASTERTIZER_DISCARD
    glEnable (GL_RASTERIZER_DISCARD);
#endif

    for (i = 0, b = &self->seeding.bins[0];
         i < BINS_N;
         i += 1, b += 1) {
        b->patches = 0;
    }
    
    for (i = 0 ; i < tiles->size[0] ; i += 1) {    
	for (j = 0 ; j < tiles->size[1] ; j += 1) {
            int k, l, n;

            l = i * tiles->size[1] + j;
            n = seed_tile(l);
            
            for (k = 0, b = &self->seeding.bins[0];
                 k < BINS_N;
                 k += 1, b += 1) {

                if(b->fill > 0) {
                    double r, *C;
                    
                    glUniform2f(self->locations.offset,
                                -i + 0.5 * tiles->size[0],
                                -j + 0.5 * tiles->size[1]);
                    
                    glBindTexture(GL_TEXTURE_2D, tiles->imagery[l]);
                    glPointSize(1);

                    glBufferData (GL_ARRAY_BUFFER, n * SEED_SIZE, NULL,
                                  GL_STREAM_DRAW);
                    glBufferSubData (GL_ARRAY_BUFFER, 0,
                                     b->fill * SEED_SIZE, b->buffer);

                    C = self->seeding.clustering;
                    r = round(b->center / C[0]);

                    if (r >= 1.0) {
                        glUniform3f(self->locations.clustering,
                                    C[0], C[1],
                                    self->seeding.ceiling / C[0] - r);
                        glDrawArraysInstanced(GL_PATCHES, 0, b->fill, r);

                        b->patches += b->fill * r;
                    } else {
                        glUniform3f(self->locations.clustering,
                                    round(b->center), C[1], 0);
                        glDrawArraysInstanced(GL_PATCHES, 0, b->fill, 1);
                        
                        b->patches += b->fill;
                    }
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

-(int) _get_ceiling
{
    lua_pushnumber(_L, self->seeding.ceiling);

    return 1;
}

-(void) _set_ceiling
{
    self->seeding.ceiling = lua_tonumber(_L, 3);
}

-(int) _get_clustering
{
    int i;
    
    lua_createtable (_L, 2, 0);

    for (i = 0 ; i < 2 ; i += 1) {
        lua_pushnumber (_L, self->seeding.clustering[i]);
        lua_rawseti (_L, -2, i + 1);
    }
    
    return 1;
}

-(void) _set_clustering
{
    int i;
    
    if(lua_istable(_L, 3)) {
        for (i = 0 ; i < 2 ; i += 1) {
            lua_rawgeti (_L, 3, i + 1);
            self->seeding.clustering[i] = lua_tonumber (_L, -1);
            lua_pop(_L, 1);
        }
    } else {
        self->seeding.clustering[0] = 1.0 / 0.0;
        self->seeding.clustering[1] = 0.0;
    }
}

-(int) _get_bins
{
    int i;
    
    lua_createtable (_L, BINS_N, 0);

    for (i = 0 ; i < BINS_N ; i += 1) {
        seeding_Bin *b;

        b = &self->seeding.bins[i];
        
        lua_createtable (_L, 4, 0);
        lua_pushnumber (_L, b->center);
        lua_rawseti (_L, -2, 1);
        lua_pushinteger (_L, b->triangles);
        lua_rawseti (_L, -2, 2);
        lua_pushinteger (_L, b->patches);
        lua_rawseti (_L, -2, 3);
        lua_pushinteger (_L, b->capacity);
        lua_rawseti (_L, -2, 4);

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
