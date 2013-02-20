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
    Elevation *mold;
    int l;

    /* Make a reference to the mold to make sure it's not
     * collected. */

    mold = t_tonode (_L, -1);
    self->reference = luaL_ref (_L, LUA_REGISTRYINDEX);
    
    [super initWithMode: GL_TRIANGLES];

    self->optimize = 1;
    
    self->context = &mold->context;
    self->ranges = (int *)calloc (self->context->tileset.size[0] *
                                  self->context->tileset.size[1],
                                  sizeof (int));

    /* Create the vertex buffer. */

    glGenBuffers(1, &self->buffer);
    l = 9 * self->context->target * sizeof(float);
    
    glBindBuffer (GL_ARRAY_BUFFER, self->buffer);
    glBufferData (GL_ARRAY_BUFFER, l, NULL, GL_STREAM_DRAW);

    self->vertices = malloc(l);
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
}

-(int) _get_target
{
    lua_pushnumber (_L, self->context->target);

    return 1;
}

-(void) _set_target
{
    int l;

    self->context->target = lua_tonumber (_L, 3);

    /* Update the vertex buffer size. */

    l = 9 * self->context->target * sizeof(float);

    glBindBuffer (GL_ARRAY_BUFFER, self->buffer);
    glBufferData (GL_ARRAY_BUFFER, l, NULL, GL_STREAM_DRAW);

    self->vertices = realloc(self->vertices, l);
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
    
    draw_geometry(self->context, self->vertices, self->ranges);

    /* Update the vertex buffer object. */
    
    l = 9 * self->context->target * sizeof(float);
    
    glBindBuffer (GL_ARRAY_BUFFER, self->buffer);
    glBufferData (GL_ARRAY_BUFFER, l, NULL, GL_STREAM_DRAW);
    glBufferData (GL_ARRAY_BUFFER, l, self->vertices, GL_STREAM_DRAW);

    /* Prepare to draw. */
    
    glBindVertexArray(self->name);
    glUniform1f(self->locations.scale, ldexpf(1, -tiles->depth));
    glActiveTexture(GL_TEXTURE0);
    
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