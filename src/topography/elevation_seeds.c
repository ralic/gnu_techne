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
#include "vegetation.h"
#include "elevation.h"

/* Check whether a triangle is within the seedable depth range.  The
 * last two tests ensure that a triangle that is larger than the
 * entire viewing volume (and will therefore fail the first three
 * tests) will still be seeded. */

#define should_seed(z_a, z_0, z_1)     \
    (z_a < 0 && z_a > horizon) ||      \
    (z_0 < 0 && z_0 > horizon) ||      \
    (z_1 < 0 && z_1 > horizon) ||      \
    z_a * z_0 < 0 ||                   \
    z_0 * z_1 < 0

static roam_Context *context;
static elevation_Bin *bins;
static float modelview[16], projection[16];
static double density, bias, horizon, error;

static int fine, coarse, highwater, initialized;

static void grow_bin(int i)
{
    if(bins[i].capacity == 0) {
        bins[i].capacity = 8;
    } else {
        bins[i].capacity *= 2;
    }

    if (bins[i].capacity > highwater) {
        highwater = bins[i].capacity;
    }
    
    bins[i].buffer = realloc (bins[i].buffer, bins[i].capacity * SEED_SIZE);
}
        
static void project(float *r, float *s)
{
    float z;

    z = fmax(-(r[2] + modelview[14]) + bias, bias);
    s[0] = projection[0] * (r[0] + modelview[12]) / z;
    s[1] = projection[5] * (r[1] + modelview[13]) / z;
}

static void seed_triangle(float *a, float *b_0, float *b_1,
                          float *a_r, float *b_0r, float *b_1r,
                          float *a_s, float *b_0s, float *b_1s,
                          int level)
{
    if (level < TREE_HEIGHT - 1) {
        double z_a, z_0, z_1;

        /* This is not a fine triangle, so calculate the depths of the
         * vertices to test whether to descend any further. */
        
        z_a = a_r[2] + modelview[14];
        z_0 = b_0r[2] + modelview[14];
        z_1 = b_1r[2] + modelview[14];
        
        if (should_seed(z_a, z_0, z_1)) {
            float c[3], c_r[3], c_s[2];

            /* Interpolate the center vertex in object and rotated
             * space and recurse. */
            
            c[0] = 0.5 * (b_0[0] + b_1[0]);
            c[1] = 0.5 * (b_0[1] + b_1[1]);
            c[2] = 0.5 * (b_0[2] + b_1[2]);
    
            c_r[0] = 0.5 * (b_0r[0] + b_1r[0]);
            c_r[1] = 0.5 * (b_0r[1] + b_1r[1]);
            c_r[2] = 0.5 * (b_0r[2] + b_1r[2]);

            project (c_r, c_s);

            /* _TRACE ("%f\n", fabs(z_c - 0.5 * (z_0 + z_1))); */
            
            seed_triangle(c, a, b_0, c_r, a_r, b_0r, c_s, a_s, b_0s, level + 1);
            seed_triangle(c, b_1, a, c_r, b_1r, a_r, c_s, b_1s, a_s, level + 1);
        }
    } else {
        char *p;
        double n_0, u[2], v[2];
        int i, j;

        /* Project all vertices into screen space. */

        /* Calculate the screen-space area of the triangle and
         * multiply by density and bias to get the number of seeds. */
        
        for (i = 0 ; i < 2 ; i += 1) {
            u[i] = b_0s[i] - a_s[i];
            v[i] = b_1s[i] - a_s[i];
        }

        /* _TRACEM (4, 4, "f", projection); */

        n_0 = round(density *
                    0.5 * sqrt((u[0] * u[0] + u[1] * u[1]) *
                               (v[0] * v[0] + v[1] * v[1])));
        
        /* This simple search should be preffered to binary searching
         * as the vast majority of seeds are likely to end up in the
         * first bins. */
        
        for (j = 0;
             j < BINS_N - 1 && n_0 > 0.5 * (bins[j].mean + bins[j + 1].mean);
             j += 1);

        /* _TRACE ("%d\n", j); */
        
        if (bins[j].fill == bins[j].capacity) {
            grow_bin(j);
        }

        /* printf ("%f\n", bias * density / -z); */
        /* assert (j <= 31); */
        /* assert (n <= density); */
        
        p = bins[j].buffer + bins[j].fill * SEED_SIZE;
        memcpy (p, a, 3 * sizeof(float));
        memcpy (p + 3 * sizeof(float), b_0, 3 * sizeof(float));
        memcpy (p + 6 * sizeof(float), b_1, 3 * sizeof(float));

        bins[j].sum += n_0;
        bins[j].fill += 1;
        error += (bins[j].mean - n_0) * (bins[j].mean - n_0);
        fine += 1;
    }
}

static void seed_subtree(roam_Triangle *n,
                         float *a_r, float *b_0r, float *b_1r)
{
    if(!is_out(n)) {
        double z_a, z_0, z_1;

        z_a = a_r[2] + modelview[14];
        z_0 = b_0r[2] + modelview[14];
        z_1 = b_1r[2] + modelview[14];
        
        if (should_seed(z_a, z_0, z_1)) {
            if(!is_leaf(n)) {
                float c_r[3];
                
                c_r[0] = 0.5 * (b_0r[0] + b_1r[0]);
                c_r[1] = 0.5 * (b_0r[1] + b_1r[1]);
                c_r[2] = 0.5 * (b_0r[2] + b_1r[2]);
                
                seed_subtree(n->children[0], c_r, a_r, b_0r);
                seed_subtree(n->children[1], c_r, b_1r, a_r);
            } else {
                roam_Triangle *p;
                roam_Diamond *d, *e;
                float *a, *b_0, *b_1, a_s[2], b_0s[2], b_1s[2];
                int i;

                p = n->parent;
                d = n->diamond;
                e = p->diamond;
                i = is_primary(n);

                a = e->center;
                b_0 = d->vertices[!i];
                b_1 = d->vertices[i];

                project(a_r, a_s);
                project(b_0r, b_0s);
                project(b_1r, b_1s);
                
                seed_triangle (a, b_0, b_1, a_r, b_0r, b_1r, a_s, b_0s, b_1s, d->level);

                coarse += 1;
            }
        }
    }
}

static void seed_vegetation(ElevationSeeds *seeds,
                            double density_in, double bias_in,
                            unsigned int _ls, unsigned int _lo)
{
    roam_Tileset *tiles;
    int i, j;

    t_copy_modelview (modelview);
    t_copy_projection (projection);
    
    coarse = fine = error = 0;
    context = seeds->context;
    bins = seeds->bins;
    horizon = -seeds->horizon;        
    density = density_in;
    bias = bias_in;
    tiles = &context->tileset;

    glPatchParameteri(GL_PATCH_VERTICES, 1);
    glUniform1f(_ls, ldexpf(1, -tiles->depth));

    /* Initialize the bin centers using a power series.  This tends to
     * make bin seed density ranges inversely proportional to the
     * number of seeds that are expected to fall into them given a
     * frustum viewing volume. */
    
    if (!initialized) {
        double f, c;
    
        c = pow(density, 1.0 / (BINS_N - 1));

        for (i = 0, f = 1 ; i < BINS_N ; i += 1, f *= c) {
            bins[i].mean = f;
            /* bins[i].mean = 1 + i * (density - 1) / (BINS_N - 1); */
            /* printf ("%f\n", bins[i].mean); */
        }

        initialized = 1;
    }

    /* Update the cluster centers. */
    
    for (i = 0 ; i < BINS_N ; i += 1) {
        if (bins[i].total > 0) {
            bins[i].mean = bins[i].sum / bins[i].total;
        }

        bins[i].total = 0;
        bins[i].sum = 0;

        /* assert (k == BINS_N - 1 || bins[k].mean <= bins[k + 1].mean); */
    }
    
    glEnable (GL_MULTISAMPLE);

//#define RASTERTIZER_DISCARD   
#ifdef RASTERTIZER_DISCARD
    glEnable (GL_RASTERIZER_DISCARD);
#endif

    for (i = 0 ; i < tiles->size[0] ; i += 1) {    
	for (j = 0 ; j < tiles->size[1] ; j += 1) {
            roam_Triangle *n_0, *n_1;
            roam_Diamond *d, *p_0, *p_1;
            float *b_0, *b_1, *a_0, *a_1;
            float b_0r[3], b_1r[3], a_0r[3], a_1r[3];
	    int q, k;
            
            for (k = 0 ; k < BINS_N ; k += 1) {
                bins[k].total += bins[k].fill;
                bins[k].fill = 0;
            }

            k = i * tiles->size[1] + j;

            glUniform2f(_lo, j, i);
            glBindTexture(GL_TEXTURE_2D, tiles->imagery[k]);
            glPointSize(1);

            n_0 = context->roots[k][0];
            n_1 = context->roots[k][1];
            d = n_0->diamond;
            p_0 = n_0->parent->diamond;
            p_1 = n_1->parent->diamond;
            q = is_primary(n_0);

            b_0 = d->vertices[!q];
            b_1 = d->vertices[q];
            a_0 = p_0->center;
            a_1 = p_1->center;

            /* Calculate the distance from the eye to each vertex. */

            t_transform_4RT3(b_0r, modelview, b_0);
            t_transform_4RT3(b_1r, modelview, b_1);
            t_transform_4RT3(a_0r, modelview, a_0);
            t_transform_4RT3(a_1r, modelview, a_1);

            seed_subtree(n_0, a_0r, b_0r, b_1r);
            seed_subtree(n_1, a_1r, b_1r, b_0r);

            for (k = 0 ; k < BINS_N ; k += 1) {
                if(bins[k].fill > 0) {
                    /* printf ("%f\n", bins[k].mean); */
                    glBufferData (GL_ARRAY_BUFFER, highwater * SEED_SIZE, NULL, GL_STREAM_DRAW);
                    glBufferSubData (GL_ARRAY_BUFFER, 0, bins[k].fill * SEED_SIZE, bins[k].buffer);
                    glDrawArraysInstanced(GL_PATCHES, 0, bins[k].fill, round(bins[k].mean));
                }
            }
	}
    }

    /* _TRACE ("error: %f\n", error); */

    glDisable (GL_MULTISAMPLE);
    
#ifdef RASTERTIZER_DISCARD
    glDisable (GL_RASTERIZER_DISCARD);
#endif

    seeds->triangles_n[0] = coarse;
    seeds->triangles_n[1] = fine;
    seeds->error = error;
    /* if (fine > 0) abort(); */
    /* _TRACE ("Horizon: %g, Coarse: %d, Fine: %d\n", horizon, coarse, fine); */
}

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
    self->horizon = 100;

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

    seed_vegetation (self, self->density, self->bias,
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

-(int) _get_bins
{
    int i;
    
    lua_createtable (_L, BINS_N, 0);

    for (i = 0 ; i < BINS_N ; i += 1) {
        lua_createtable (_L, 3, 0);
        lua_pushnumber (_L, self->bins[i].mean);
        lua_rawseti (_L, -2, 1);
        lua_pushinteger (_L, self->bins[i].total);
        lua_rawseti (_L, -2, 2);
        lua_pushinteger (_L, self->bins[i].capacity);
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
        lua_pushnumber (_L, self->triangles_n[i]);
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
    lua_pushnumber (_L, self->horizon);
    
    return 1;
}

-(void) _set_horizon
{
    self->horizon = lua_tonumber (_L, 3);
}

-(int) _get_error
{
    lua_pushnumber (_L, self->error);
    
    return 1;
}

-(void) _set_error
{
    T_WARN_READONLY;
}

@end
