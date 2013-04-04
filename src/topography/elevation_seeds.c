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

#include <lua.h>
#include <lauxlib.h>

#include "techne.h"
#include "roam.h"
#include "vegetation.h"
#include "elevation.h"

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

    self->units.base = [parent getUnitForSamplerUniform: "base"];
}

-(void) draw: (int)frame
{
    roam_Tileset *tiles;
    
    tiles = &self->context->tileset;

    {
        float M[16] = {tiles->resolution[0], 0, 0, 0,
                       0, tiles->resolution[1], 0, 0,
                       0, 0, 1, 0,
                       -(1 << (tiles->depth - 1)) * tiles->size[0] * tiles->resolution[0],
                       -(1 << (tiles->depth - 1)) * tiles->size[1] * tiles->resolution[1],
                       0, 1};
    
        t_push_modelview (self->matrix, T_MULTIPLY);
        t_load_modelview (M, T_MULTIPLY);
    }

    optimize_geometry(self->context, frame);

    glBindBuffer(GL_ARRAY_BUFFER, self->buffer);
    glBindVertexArray(self->name);
    glActiveTexture(GL_TEXTURE0 + self->units.base);

    seed_vegetation (self->context, self->density, self->bias,
                     self->locations.scale, self->locations.offset);

    t_pop_modelview ();
    
    [super draw: frame];
}

-(int) _get_density
{
    lua_pushnumber(_L, self->density);

    return 1;
}

-(void) _set_density
{
    self->density = lua_tonumber(_L, 3);
}

-(int) _get_bias
{
    lua_pushnumber(_L, self->bias);

    return 1;
}

-(void) _set_bias
{
    self->bias = lua_tonumber(_L, 3);
}

@end
