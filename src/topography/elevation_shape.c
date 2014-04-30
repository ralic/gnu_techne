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

#include <string.h>

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

    self->locations.offset = glGetUniformLocation(parent->name, "offset");

    self->units.base = t_sampler_unit(parent, "base");
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
    t_pushcount(_L, &self->triangles);

    return 1;
}

-(void) _set_triangles
{
    T_WARN_READONLY;
}

-(int) _get_culled
{
    t_pushcount(_L, &self->culled);

    return 1;
}

-(void) _set_culled
{
    T_WARN_READONLY;
}

-(int) _get_visible
{
    t_pushcount(_L, &self->visible);

    return 1;
}

-(void) _set_visible
{
    T_WARN_READONLY;
}

-(int) _get_diamonds
{
    t_pushcount(_L, &self->diamonds);

    return 1;
}

-(void) _set_diamonds
{
    T_WARN_READONLY;
}

-(int) _get_splittable
{
    t_pushcount(_L, &self->splittable);

    return 1;
}

-(void) _set_splittable
{
    T_WARN_READONLY;
}

-(int) _get_mergeable
{
    t_pushcount(_L, &self->mergeable);

    return 1;
}

-(void) _set_mergeable
{
    T_WARN_READONLY;
}
 
-(int) _get_reculling
{
    t_pushcoreinterval(_L, &self->context->reculling);

    return 1;
}

-(void) _set_reculling
{
    T_WARN_READONLY;
}
 
-(int) _get_reordering
{
    t_pushcoreinterval(_L, &self->context->reordering);
    
    return 1;
}

-(void) _set_reordering
{
    T_WARN_READONLY;
}
 
-(int) _get_tessellation
{
    t_pushcoreinterval(_L, &self->context->tessellation);

    return 1;
}

-(void) _set_tessellation
{
    T_WARN_READONLY;
}

-(void) draw: (int)frame
{
    roam_Tileset *tiles;
    float *vert;
    int i, j, l;
    
    tiles = &self->context->tileset;

    t_push_modelview (self->matrix, T_MULTIPLY);

    if (self->optimize) {
        optimize_geometry(self->context, frame);

        if (_PROFILING) {
            t_add_count_sample (&self->triangles, self->context->triangles);
            t_add_count_sample (&self->culled, self->context->culled);
            t_add_count_sample (&self->visible, self->context->visible);
            t_add_count_sample (&self->diamonds, self->context->diamonds);
            t_add_count_sample (&self->splittable, self->context->queued[0]);
            t_add_count_sample (&self->mergeable, self->context->queued[1]);
        }
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
