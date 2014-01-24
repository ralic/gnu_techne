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

static float *buffer;
static int triangles_n;
    
static void draw_subtree(roam_Triangle *n)
{
    if(!is_out(n)) {
	if(!is_leaf(n)) {
	    draw_subtree(n->children[0]);
	    draw_subtree(n->children[1]);
	} else {
	    roam_Triangle *p;
	    roam_Diamond *d, *e;
	    int i;

	    p = n->parent;
	    d = n->diamond;
	    e = p->diamond;
	    i = is_primary(n);

            /* if ((n->cullbits & ALL_IN) == ALL_IN) return; */
            
            memcpy (buffer + 9 * triangles_n,
                    d->vertices[!i], 3 * sizeof(float));
            memcpy (buffer + 9 * triangles_n + 3,
                    d->vertices[i], 3 * sizeof(float));
            memcpy (buffer + 9 * triangles_n + 6,
                    e->center, 3 * sizeof(float));
            
	    triangles_n += 1;
	}
    }
}

static void draw_geometry(roam_Context *context, float *buffer_in,
                          int *ranges)
{
    roam_Tileset *tiles;
    int i, j;

    buffer = buffer_in;
    tiles = &context->tileset;
    
    /* Draw the geometry into the provided buffers. */
    
    triangles_n = 0;

    for (i = 0 ; i < tiles->size[0] ; i += 1) {    
	for (j = 0 ; j < tiles->size[1] ; j += 1) {
	    int k = i * tiles->size[1] + j;

	    draw_subtree(context->roots[k][0]);
	    draw_subtree(context->roots[k][1]);

            ranges[k] = triangles_n;
	}
    }
}       

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

-(int) _get_triangles
{
    lua_createtable (_L, 3, 0);

    lua_pushinteger (_L, self->context->triangles);
    lua_rawseti(_L, -2, 1);
    
    lua_pushinteger (_L, self->context->culled);
    lua_rawseti(_L, -2, 2);

    lua_pushinteger (_L, self->context->visible);
    lua_rawseti(_L, -2, 3);

    return 1;
}

-(void) _set_triangles
{
    T_WARN_READONLY;
}

-(int) _get_diamonds
{
    lua_createtable (_L, 3, 0);

    lua_pushinteger (_L, self->context->diamonds);
    lua_rawseti(_L, -2, 1);
    
    lua_pushinteger (_L, self->context->queued[0]);
    lua_rawseti(_L, -2, 2);

    lua_pushinteger (_L, self->context->queued[1]);
    lua_rawseti(_L, -2, 3);

    return 1;
}

-(void) _set_diamonds
{
    T_WARN_READONLY;
}
 
-(int) _get_profile
{
    int i;
    
    [super _get_profile];

    for (i = 0 ; i < 4 ; i += 1) {
        lua_pushnumber(_L, self->context->intervals[i] * 1e-9);
        lua_rawseti(_L, -2, i + 3);
    }
    
    return 1;
}

-(void) draw: (int)frame
{
    roam_Tileset *tiles;
    float q, *vert;
    int i, j, l;
    
    tiles = &self->context->tileset;

    t_push_modelview (self->matrix, T_MULTIPLY);

    if (self->optimize) {
        optimize_geometry(self->context, frame);
    }
    
    l = 9 * self->context->target * sizeof(float);

    glPolygonOffset(1, 1);
    glEnable (GL_POLYGON_OFFSET_FILL);

    /* Update the vertex buffer object. */
    
    glBindBuffer (GL_ARRAY_BUFFER, self->buffer);
    glBufferData (GL_ARRAY_BUFFER, l, NULL, GL_STREAM_DRAW);
    vert = glMapBuffer (GL_ARRAY_BUFFER, GL_WRITE_ONLY);
    
    draw_geometry(self->context, vert, self->ranges);

    i = glUnmapBuffer(GL_ARRAY_BUFFER);
    assert(i);

    /* Prepare to draw. */
    
    glActiveTexture(GL_TEXTURE0 + self->units.base);

    glBindVertexArray(self->name);

    q = ldexpf(1, -tiles->depth);
    glUniform2f(self->locations.scale,
                q / tiles->resolution[0],
                q / tiles->resolution[1]);
    
    for (i = 0 ; i < tiles->size[0] ; i += 1) {    
	for (j = 0 ; j < tiles->size[1] ; j += 1) {
	    int l, k = i * tiles->size[1] + j;

            glUniform2f(self->locations.offset,
                        -i + 0.5 * tiles->size[0],
                        -j + 0.5 * tiles->size[1]);
            
            glBindTexture(GL_TEXTURE_2D, tiles->imagery[k]);

            l = k > 0 ? self->ranges[k - 1] : 0;
            
            glDrawArrays (self->mode, 3 * l, 3 * (self->ranges[k] - l));
        }
    }
    
    glDisable (GL_POLYGON_OFFSET_FILL);
    
    t_pop_modelview ();
    
    [super draw: frame];
}

@end
