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
#include "elevation.h"

@implementation ElevationShape

-(void) init
{
    Elevation *elevation;

    /* Make a reference to the elevation to make sure it's not
     * collected. */

    elevation = t_tonode (_L, -1);
    self->reference = luaL_ref (_L, LUA_REGISTRYINDEX);
    
    [super initWithMode: GL_TRIANGLES];

    self->optimize = 1;
    
    self->context = &elevation->context;
    self->ranges = (int *)calloc (self->context->tileset.size[0] *
                                  self->context->tileset.size[1],
                                  sizeof (int));

    /* Create the vertex buffer. */

    glGenBuffers(1, &self->buffer);
}

-(void) free
{
    luaL_unref (_L, LUA_REGISTRYINDEX, self->reference);

    free (self->ranges);
        
    [super free];
}

-(void) meetParent: (Shader *)parent
{
    int i;
    
    [super meetParent: parent];

    i = glGetAttribLocation(parent->name, "positions");

    /* Bind the VBO into the VAO. */
    
    glBindBuffer(GL_ARRAY_BUFFER, self->buffer);
    glBindVertexArray(self->name);
    glVertexAttribPointer(i, 3, GL_FLOAT, GL_FALSE, 0, (void *)0);
    glEnableVertexAttribArray(i);

    self->locations.scale = glGetUniformLocation(parent->name, "scale");
    self->locations.offset = glGetUniformLocation(parent->name, "offset");
    self->units.base = [parent getUnitForSamplerUniform: "base"];
}

-(int) _get_target
{
    lua_pushnumber (_L, self->context->target);

    return 1;
}

-(void) _set_target
{
    self->context->target = lua_tonumber (_L, 3);
}

-(int) _get_optimize
{
    lua_pushboolean (_L, self->optimize);

    return 1;
}

-(void) _set_optimize
{
    self->optimize = lua_toboolean (_L, 3);
}

-(void) draw: (int)frame
{
    roam_Tileset *tiles;
    float *vert;
    int i, j, l;
    
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
    
    if (self->optimize) {
        optimize_geometry(self->context, frame);
    }

    l = 9 * self->context->target * sizeof(float);
    
    /* Update the vertex buffer object. */
    
    glBindBuffer (GL_ARRAY_BUFFER, self->buffer);
    glBufferData (GL_ARRAY_BUFFER, l, NULL, GL_STREAM_DRAW);
    vert = glMapBuffer (GL_ARRAY_BUFFER, GL_WRITE_ONLY);
    
    draw_geometry(self->context, vert, self->ranges);

    assert(glUnmapBuffer(GL_ARRAY_BUFFER));

    /* Prepare to draw. */
    
    glActiveTexture(GL_TEXTURE0 + self->units.base);

    glBindVertexArray(self->name);
    glUniform1f(self->locations.scale, ldexpf(1, -tiles->depth));
    
    for (i = 0 ; i < tiles->size[0] ; i += 1) {    
	for (j = 0 ; j < tiles->size[1] ; j += 1) {
	    int l, k = i * tiles->size[1] + j;

            glUniform2f(self->locations.offset, j, i);
            glBindTexture(GL_TEXTURE_2D, tiles->imagery[k]);

            l = k > 0 ? self->ranges[k - 1] : 0;
            
            glDrawArrays (self->mode, 3 * l, 3 * (self->ranges[k] - l));
        }
    }
    
    t_pop_modelview ();
    
    [super draw: frame];
}

@end
