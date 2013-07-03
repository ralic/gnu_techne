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
#include <math.h>
#include <GL/gl.h>

#include "roam.h"
#include "seeding.h"
#include "algebra.h"
#include "techne.h"

//#define K_MEANS

/* Check whether a triangle is within the seedable depth range.  The
 * last two tests ensure that a triangle that is larger than the
 * entire viewing volume (and will therefore fail the first three
 * tests) will still be seeded. */

#define should_seed(z_a, z_0, z_1)              \
    (z_a < 0 && z_a > seeding->horizon) ||      \
    (z_0 < 0 && z_0 > seeding->horizon) ||      \
    (z_1 < 0 && z_1 > seeding->horizon) ||      \
    z_a * z_0 < 0 ||                            \
    z_0 * z_1 < 0

static roam_Context *context;
static seeding_Context *seeding;
static float modelview[16], projection[16];
static double aspect;
static double n_min, n_max;
static int highwater;

static void recenter_bins(double d)
{
    seeding_Bin *b;
    double f, c;
    int i;

    if (!isinf(n_min)) {
        c = pow(d, 1.0 / (BINS_N - 1));

        for (i = 0, b = &seeding->bins[0], f = 1;
             i < BINS_N;
             i += 1, b += 1, f *= c) {
            b->center = n_min * f;
        }
    }
}

static void grow_bin(int i)
{
    seeding_Bin *b;

    b = &seeding->bins[i];
    
    if(b->capacity == 0) {
        b->capacity = 8;
    } else {
        b->capacity *= 2;
    }

    if (b->capacity > highwater) {
        highwater = b->capacity;
    }
    
    b->buffer = realloc (b->buffer, b->capacity * SEED_SIZE);
}
        
static void project(float *r, float *s)
{
    float z;

    z = fmax(-(r[2] + modelview[14]) + seeding->bias, seeding->bias);
    s[0] = aspect * projection[0] * (r[0] + modelview[12]) / z;
    s[1] = projection[5] * (r[1] + modelview[13]) / z;
}

static int cull(float *a_r, float *b_0r, float *b_1r)
{
    float u[3], v[3], a_e[3], n[3];
    int i;

    for (i = 0 ; i < 3 ; i += 1) {
        u[i] = a_r[i] - b_0r[i];
        v[i] = a_r[i] - b_1r[i];
        a_e[i] = a_r[i] + modelview[12 + i];
    }

    /* No need to normalize since it doesn't affect the sign of the
     * dot product. */
    
    t_cross(n, u, v);
    
    /* _TRACE ("%f\n", n_z); */
    return t_dot_3(n, a_e) > 0;
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
        seeding_Bin *bins;    
        char *p;
        double A, n_0, u[2], v[2];
        int i, j;

        /* Project all vertices into screen space. */

        /* Calculate the screen-space area of the triangle and
         * multiply by density and bias to get the number of seeds. */
        
        for (i = 0 ; i < 2 ; i += 1) {
            u[i] = b_0s[i] - a_s[i];
            v[i] = b_1s[i] - a_s[i];
        }

        /* _TRACEM (4, 4, "f", projection); */
        A = 0.5 * sqrt((u[0] * u[0] + u[1] * u[1]) *
                       (v[0] * v[0] + v[1] * v[1]));
        n_0 = fmax(seeding->density * A, 1);

        if (n_min > n_0) {
            n_min = n_0;
        }

        if (n_max < n_0) {
            n_max = n_0;
        }
        
        /* This simple search should be preffered to binary searching
         * as the vast majority of seeds are likely to end up in the
         * first bins. */

        bins = seeding->bins;

        for (j = 0;
             j < BINS_N - 1 && n_0 > 0.5 * (bins[j].center + bins[j + 1].center);
             j += 1);

        /* _TRACE ("%d\n", j); */
        
        if (bins[j].fill == bins[j].capacity) {
            grow_bin(j);
        }

        /* printf ("%f\n", bias * density / -z); */
        /* assert (j <= 31); */
        /* assert (n <= density); */

        /* { */
        /*     float c[2], m; */

        /*     c[0] = (a[0] + b_0[0] + b_1[0]) / 3; */
        /*     c[1] = (a[1] + b_0[1] + b_1[1]) / 3; */
        /*     m = sqrt(c[0] * c[0] + c[1] * c[1]); */
            
        /*     _TRACE ("%f, %f\n", c[0] / m, c[1] / m); */
        /* } */
            
        p = bins[j].buffer + bins[j].fill * SEED_SIZE;
        memcpy (p, a, 3 * sizeof(float));
        memcpy (p + 3 * sizeof(float), b_0, 3 * sizeof(float));
        memcpy (p + 6 * sizeof(float), b_1, 3 * sizeof(float));

#ifdef K_MEANS
        bins[j].sum += n_0;
#endif
        
        bins[j].fill += 1;
        seeding->error += (bins[j].center - n_0) * (bins[j].center - n_0);
        seeding->triangles_n[1] += 1;
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
                float c_r[3], *c;

                c = n->diamond->center;
                
                t_transform_4RT3(c_r, modelview, c);
                
                seed_subtree(n->children[0], c_r, a_r, b_0r);
                seed_subtree(n->children[1], c_r, b_1r, a_r);
            } else if (!cull (a_r, b_0r, b_1r)) {
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

                seeding->triangles_n[0] += 1;
            }
        }
    }
}

void initialize_seeding (seeding_Context *seeding_in)
{
    /* Initialize the bin centers using a power series.  This tends to
     * make bin seed density ranges inversely proportional to the
     * number of seeds that are expected to fall into them given a
     * frustum viewing volume. */

    seeding = seeding_in;
    recenter_bins(seeding_in->density);
}

void begin_seeding (seeding_Context *seeding_in, roam_Context *context_in)
{
    seeding_Bin *b;
    int i, viewport[4];

    t_copy_modelview (modelview);
    t_copy_projection (projection);

    glGetIntegerv(GL_VIEWPORT, viewport);
    aspect = (double)viewport[2] / (double)viewport[3];

    seeding_in->triangles_n[0] = 0;
    seeding_in->triangles_n[1] = 0;
    seeding_in->error = 0;
    n_min = 1.0 / 0.0;
    n_max = 0;
                    
    context = context_in;
    seeding = seeding_in;

    /* Reset the bins. */
    
    for (i = 0, b = &seeding->bins[0] ; i < BINS_N ; i += 1, b += 1) {
#ifdef K_MEANS
        if (b->total > 0) {
            b->center = b->sum / b->total;
        }

        b->sum = 0;
#endif
        
        b->total = 0;

        /* assert (k == BINS_N - 1 || bins[k].center <= bins[k + 1].center); */
    }
}

int seed_tile (int i)
{
    seeding_Bin *b;
    roam_Triangle *n_0, *n_1;
    roam_Diamond *d, *p_0, *p_1;
    float *b_0, *b_1, *a_0, *a_1;
    float b_0r[3], b_1r[3], a_0r[3], a_1r[3];
    int q, j;

    /* Update the bins. */
    
    for (j = 0, b = &seeding->bins[0] ; j < BINS_N ; j += 1, b += 1) {
        b->total += b->fill;
        b->fill = 0;
    }
            
    n_0 = context->roots[i][0];
    n_1 = context->roots[i][1];
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

    return highwater;
}

void finish_seeding ()
{
#ifndef K_MEANS
    recenter_bins(n_max / n_min);
#endif
}
