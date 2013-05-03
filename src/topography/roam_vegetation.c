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

#include "gl.h"

#include "algebra.h"
#include "techne.h"
#include "roam.h"

#define should_seed(z_a, z_0, z_1)   \
    (z_a < 0 && z_a > threshold) ||  \
    (z_0 < 0 && z_0 > threshold) ||  \
    (z_1 < 0 && z_1 > threshold) ||  \
    z_a * z_0 < 0 ||                 \
    z_0 * z_1 < 0

static roam_Context *context;
static roam_Bin *bins;
static float modelview[16];
static double density, bias, threshold, error;

static int total, coarse, instances, highwater, initialized;

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

static void seed_triangle(float *a, float *b_0, float *b_1,
                          float z_a, float z_0, float z_1, int level)
{    
    if (level < TREE_HEIGHT - 1) {
        float b_c[3], z_c;
    
        b_c[0] = 0.5 * (b_0[0] + b_1[0]);
        b_c[1] = 0.5 * (b_0[1] + b_1[1]);
        b_c[2] = 0.5 * (b_0[2] + b_1[2]);

        z_c = 0.5 * (z_0 + z_1);

        /* _TRACE ("%f\n", fabs(z_c - 0.5 * (z_0 + z_1))); */
            
        if (should_seed(z_a, z_0, z_1)) {
            seed_triangle(b_c, a, b_0, z_c, z_a, z_0, level + 1);
            seed_triangle(b_c, b_1, a, z_c, z_1, z_a, level + 1);
        }
    } else {
        double z, n_0;
        int j;
        char *p;

        /* fprintf(stderr, "%d\n", (((l + m) * (l + m + 1)) >> 1) + m); */
        
        z = fmax(-(z_0 + z_1 + z_a) / 3.0, bias);
        n_0 = bias * bias * density / z / z * sqrt(fabs(modelview[10]));

        /* fprintf (stderr, "%f\n", n_0); */
        
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
        error += bins[j].mean - n_0;
        total += 1;
    }
}

static void seed_subtree(roam_Triangle *n, float z_a, float z_0, float z_1)
{
    if(!is_out(n)) {
        if (should_seed(z_a, z_0, z_1)) {

            if(!is_leaf(n)) {
                float z_c;
        
                z_c = 0.5 * (z_0 + z_1);

                seed_subtree(n->children[0], z_c, z_a, z_0);
                seed_subtree(n->children[1], z_c, z_1, z_a);
            } else {
                roam_Triangle *p;
                roam_Diamond *d, *e;
                float *a, *b_0, *b_1;
                int i;

                p = n->parent;
                d = n->diamond;
                e = p->diamond;
                i = is_primary(n);

                a = e->center;
                b_0 = d->vertices[!i];
                b_1 = d->vertices[i];
            
                seed_triangle (a, b_0, b_1, z_a, z_0, z_1, d->level);

                coarse += 1;
            }
        }
    }
}

void seed_vegetation(roam_Context *context_in,
                     double density_in, double bias_in,
                     unsigned int _ls, unsigned int _lo)
{
    roam_Tileset *tiles;
    int i, j;

    coarse = instances = total = error = 0;
    context = context_in;
    density = density_in;
    bias = bias_in;
    threshold = -sqrt(bias_in * bias_in * density_in * fabs(modelview[10]));
    tiles = &context->tileset;
    bins = context->bins;
    
    t_copy_modelview (modelview);
    
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
    
    glEnable (GL_MULTISAMPLE);
    /* glEnable (GL_RASTERIZER_DISCARD); */

    for (i = 0 ; i < tiles->size[0] ; i += 1) {    
	for (j = 0 ; j < tiles->size[1] ; j += 1) {
            roam_Triangle *n_0, *n_1;
            roam_Diamond *d, *p_0, *p_1;
            float *b_0, *b_1, *a_0, *a_1;
            float z_a0, z_a1, z_b0, z_b1;
	    int q, k;
            
            for (k = 0 ; k < BINS_N ; k += 1) {
                if (bins[k].fill > 0) {
                    bins[k].mean = bins[k].sum / bins[k].fill;
                }

                bins[k].fill = 0;
                bins[k].sum = 0;

                /* assert (k == BINS_N - 1 || bins[k].mean <= bins[k + 1].mean); */
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
            
            z_a0 = modelview[2] * a_0[0] +
                modelview[6] * a_0[1] +
                modelview[10] * a_0[2] +
                modelview[14];
            
            z_a1 = modelview[2] * a_1[0] +
                modelview[6] * a_1[1] +
                modelview[10] * a_1[2] +
                modelview[14];
            
            z_b0 = modelview[2] * b_0[0] +
                modelview[6] * b_0[1] +
                modelview[10] * b_0[2] +
                modelview[14];

            z_b1 = modelview[2] * b_1[0] +
                modelview[6] * b_1[1] +
                modelview[10] * b_1[2] +
                modelview[14];

            seed_subtree(n_0, z_a0, z_b0, z_b1);
            seed_subtree(n_1, z_a1, z_b1, z_b0);

            for (k = 0 ; k < BINS_N ; k += 1) {
                if(bins[k].fill > 0) {
                    /* printf ("%f\n", bins[k].mean); */
                    glBufferData (GL_ARRAY_BUFFER, highwater * SEED_SIZE, NULL, GL_STREAM_DRAW);
                    glBufferSubData (GL_ARRAY_BUFFER, 0, bins[k].fill * SEED_SIZE, bins[k].buffer);
                    glDrawArraysInstanced(GL_PATCHES, 0, bins[k].fill, round(bins[k].mean));

                    instances += bins[k].fill * round(bins[k].mean);
                }
            }
	}
    }

    /* _TRACE ("error: %f\n", error); */

    glDisable (GL_MULTISAMPLE);
    /* glDisable (GL_RASTERIZER_DISCARD); */

    /* if (total > 0) abort(); */
    /* _TRACE ("Horizon: %g, Seeds: %d, Coarse: %d, Fine: %d\n", threshold, instances, coarse, total); */
}
