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

#define should_seed(z_a, z_0, z_1)              \
    (z_a < 0 && z_a > parameters.threshold) ||  \
    (z_0 < 0 && z_0 > parameters.threshold) ||  \
    (z_1 < 0 && z_1 > parameters.threshold) ||  \
    z_a * z_0 < 0 ||                            \
    z_0 * z_1 < 0

static struct {
    char *buffer;
    int fill, capacity;
} bins[32];

static roam_Context *context;
static float modelview[16], normal[3];
static struct {
    double density, bias, threshold;
} parameters;

static int total, coarse, fine, highwater;

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
        float z, r, n_0;
        long int l, m;
        int i, j, n;

        /* If proper winding is observed then no two triangles can
         * share the same base vertices (the left base vertex of one
         * will be the right base vertex of the opposite and vice
         * versa).  We can therefore transform the coordinates of the
         * a vertex through a pairing function and use the result as a
         * unique seed for the RNG. */

        l = b_0[0] + b_1[0];
        m = b_0[1] + b_1[1];
        srand48((((l + m) * (l + m + 1)) >> 1) + m);

        /* fprintf(stderr, "%d\n", (((l + m) * (l + m + 1)) >> 1) + m); */
        
        z = fmin((z_0 + z_1 + z_a) / 3.0, -parameters.bias);
        n_0 = parameters.bias * parameters.bias * parameters.density / z / z;
        j = (int)fmin(32 * n_0, 31);
        n = (int)ceil(n_0);
        r = sqrt(1.0 / n);
        
        /* printf ("%f\n", parameters.bias * parameters.density / -z); */
        /* assert (j <= 31); */
        /* assert (n <= parameters.density); */

        for (i = 0 ; i < n ; i += 1) {
            double r_1, r_2, sqrtr_1, k[3];
            float c[3];
            char *p;

            r_1 = drand48();
            r_2 = drand48();
            
            sqrtr_1 = sqrt(r_1);
            
            k[0] = 1 - sqrtr_1;
            k[1] = sqrtr_1 * (1 - r_2);
            k[2] = sqrtr_1 * r_2;
            
            c[0] = k[0] * a[0] + k[1] * b_0[0] + k[2] * b_1[0];
            c[1] = k[0] * a[1] + k[1] * b_0[1] + k[2] * b_1[1];
            c[2] = k[0] * a[2] + k[1] * b_0[2] + k[2] * b_1[2];

            if (bins[j].fill == bins[j].capacity) {
                grow_bin(j);
            }
            
            p = bins[j].buffer + bins[j].fill * SEED_SIZE;
            memcpy (p, c, 3 * sizeof(float));
            memcpy (p + 3 * sizeof(float), normal, 3 * sizeof(float));
            memcpy (p + 6 * sizeof(float), &r, sizeof(float));

            bins[j].fill += 1;            
            total += 1;
        }
        
        fine += 1;
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
                float *a, *b_0, *b_1, u[3], v[3];
                int i;

                p = n->parent;
                d = n->diamond;
                e = p->diamond;
                i = is_primary(n);

                a = e->center;
                b_0 = d->vertices[!i];
                b_1 = d->vertices[i];
            
                /* Calculate the triangle's normal. */
            
                u[0] = b_0[0] - a[0];
                u[1] = b_0[1] - a[1];
                u[2] = b_0[2] - a[2];

                v[0] = b_1[0] - a[0];
                v[1] = b_1[1] - a[1];
                v[2] = b_1[2] - a[2];

                t_cross (normal, u, v);
                t_normalize_3 (normal);
            
                seed_triangle (a, b_0, b_1, z_a, z_0, z_1, d->level);

                coarse += 1;
            }
        }
    }
}

void seed_vegetation(roam_Context *new, double density, double bias,
                     unsigned int _ls, unsigned int _lo)
{
    roam_Tileset *tiles;
    int i, j;

    coarse = fine = total = 0;
    context = new;
    parameters.density = density;
    parameters.bias = bias;
    parameters.threshold = -sqrt(bias * bias * density * 32);
    tiles = &context->tileset;
    
    t_copy_modelview (modelview);
    
    glPatchParameteri(GL_PATCH_VERTICES, 1);
    
    glUniform1f(_ls, ldexpf(1, -tiles->depth));
    glActiveTexture(GL_TEXTURE0);

    /* glEnable (GL_RASTERIZER_DISCARD); */
    
    for (i = 0 ; i < tiles->size[0] ; i += 1) {    
	for (j = 0 ; j < tiles->size[1] ; j += 1) {
            roam_Triangle *n_0, *n_1;
            roam_Diamond *d, *p_0, *p_1;
            float *b_0, *b_1, *a_0, *a_1;
            float z_a0, z_a1, z_b0, z_b1;
	    int q, k;

            for (k = 0 ; k < 32 ; k += 1) {
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

            for (k = 0 ; k < 32 ; k += 1) {
                if(bins[k].fill > 0) {
                    /* printf ("%d, %d\n", k, bins[k].fill); */
                    glBufferData (GL_ARRAY_BUFFER, highwater * SEED_SIZE, NULL, GL_STREAM_DRAW);
                    glBufferSubData (GL_ARRAY_BUFFER, 0, bins[k].fill * SEED_SIZE, bins[k].buffer);
                    glDrawArraysInstanced(GL_PATCHES, 0, bins[k].fill, k + 1);
                }
            }
	}
    }

    /* glDisable (GL_RASTERIZER_DISCARD); */

    /* if (fine > 0) abort(); */
    /* _TRACE ("Horizon: %g, Seeds: %d, Coarse: %d, Fine: %d\n", parameters.threshold, total, coarse, fine); */
}
