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

static roam_Context *context;
static seeding_Context *seeding;
static double modelview[16], projection[16];
static double aspect, n_min, n_max, A_total, A_mean = 0;

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
    
    b->buffer = realloc (b->buffer, b->capacity * SEED_SIZE);
}
        
static void project(float *r, float *s)
{
    float z;

    z = -(r[2] + modelview[14]);

    /* Don't bother projecting points that are behind the near
     * plane. */
    
    if (z > 0) {
        s[0] = (r[0] + modelview[12]) / z;
        s[1] = (r[1] + modelview[13]) / z;
    } else {
        s[0] = s[1] = NAN;
    }
}

static int backfacing(float *a_r, float *b_0r, float *b_1r)
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
                          unsigned char bits, unsigned char level)
{
    int i, j;
    double r_min, r_max;

    /* If the parent is completely inside the frustum skip any further
     * tests. */
    
    if (((bits & ALL_IN) != ALL_IN)) {
        /* Culling agains the near or far planes is done by comparing
         * the eye-space depth agains the interval (0, horizon). */
        
        if ((bits & (NEAR | FAR)) != (NEAR | FAR)) {
            double z_a, z_0, z_1, z_min, z_max;
        
            z_a = -(a_r[2] + modelview[14]);
            z_0 = -(b_0r[2] + modelview[14]);
            z_1 = -(b_1r[2] + modelview[14]);

            z_min = fmin(z_a, fmin(z_0, z_1));
            z_max = fmax(z_a, fmax(z_0, z_1));

            if (z_min > 0) {
                bits |= NEAR;
            } else if(z_max < 0) {
                return;
            }
            
            if (z_max < -seeding->horizon) {
                bits |= FAR;
            } else if(z_min > -seeding->horizon) {
                return;
            }
        }

        /* For the other planes full-blown tests are necessary. */
        
        for (i = 0, j = 1;
             i < 4 && ((bits & ALL_IN) != ALL_IN);
             i += 1, j <<= 1) {
            double *pi, r_a, r_0, r_1;
            
            if (bits & j) {
                continue;
            }

            pi = context->planes[i];
            
            r_a = t_dot_43(pi, a);
            r_0 = t_dot_43(pi, b_0);
            r_1 = t_dot_43(pi, b_1);

            r_min = fmin(r_a, fmin(r_0, r_1));
            r_max = fmax(r_a, fmax(r_0, r_1));

            if (r_min > context->canopy) {
                bits |= j;
            } else if(r_max < -context->canopy) {
                return;
            }
        }
    }

    /* This triangle is visible so either descend or seed. */
    
    if (level < seeding->level) {
        float c[3], c_r[3], c_s[2];

        /* _TRACE ("%f\n", fmax(fmax(z_a, z_0), z_1)); */

        /* Interpolate the center vertex in object and rotated
         * space, project to screen space and recurse. */

        for (i = 0 ; i < 3 ; i += 1) {
            c[i] = 0.5 * (b_0[i] + b_1[i]);    
            c_r[i] = 0.5 * (b_0r[i] + b_1r[i]);
        }

        project (c_r, c_s);

        /* _TRACE ("%f\n", fabs(z_c - 0.5 * (z_0 + z_1))); */
            
        seed_triangle(c, a, b_0, c_r, a_r, b_0r, c_s, a_s, b_0s,
                      bits, level + 1);
        seed_triangle(c, b_1, a, c_r, b_1r, a_r, c_s, b_1s, a_s,
                      bits, level + 1);
    } else {
        seeding_Bin *bins;    
        char *p;
        double A, n_0, u[2], v[2];
        int j;

        /* Calculate the screen-space area of the triangle and
         * multiply by density to get the number of seeds. */
        
        u[0] = aspect * projection[0] * (b_0s[0] - a_s[0]);
        u[1] = projection[5] * (b_0s[1] - a_s[1]);
            
        v[0] = aspect * projection[0] * (b_1s[0] - a_s[0]);
        v[1] = projection[5] * (b_1s[1] - a_s[1]);

        /* _TRACEM (4, 4, "f", projection); */

        A = 0.5 * fabs(u[0] * v[1] - u[1] * v[0]);

        if (seeding->rolloff < 1) {
            A = pow(A, seeding->rolloff);
        }

        if(isnan(A)) {
            n_0 = seeding->ceiling;
        } else {
            n_0 = fmin(seeding->ceiling, fmax(seeding->density * A, 1));
        }
        
        if (n_min > n_0) {
            n_min = n_0;
        }

        if (n_max < n_0) {
            n_max = n_0;
        }

        if(!isnan(A)) {
            A_total += A;
        }

        /* This simple search should be preffered to binary searching
         * as the vast majority of seeds are likely to end up in the
         * first bins. */

        bins = seeding->bins;

        for (j = 0;
             j < BINS_N - 1 && n_0 > 0.5 * (bins[j].center + bins[j + 1].center);
             j += 1);
        
        if (bins[j].fill == bins[j].capacity) {
            grow_bin(j);
        }
            
        p = bins[j].buffer + bins[j].fill * SEED_SIZE;
        memcpy (p, a, 3 * sizeof(float));
        memcpy (p + 3 * sizeof(float), b_0, 3 * sizeof(float));
        memcpy (p + 6 * sizeof(float), b_1, 3 * sizeof(float));
        
        bins[j].fill += 1;
        seeding->triangles_n[1] += 1;
    }
}

static void seed_subtree(roam_Triangle *n,
                         float *a_r, float *b_0r, float *b_1r)
{
    if(!is_out(n)) {
        double z_a, z_0, z_1, z_min;

        /* Culling for ROAM triangles has mostly been done.  We only
         * need to check against the far (seeding) plane.  */
        
        z_a = -(a_r[2] + modelview[14]);
        z_0 = -(b_0r[2] + modelview[14]);
        z_1 = -(b_1r[2] + modelview[14]);

        z_min = fmin(z_a, fmin(z_0, z_1));

        if (z_min > -seeding->horizon) {
            return;
        }
        
        if(!is_leaf(n)) {
            float c_r[3], *c;

            /* Transform the center vertex to rotated space.  A simple
             * interpolation won't do here as the center vertex is
             * offset by the heightmap. */
            
            c = n->diamond->center;
            t_transform_4RT3(c_r, modelview, c);
                
            seed_subtree(n->children[0], c_r, a_r, b_0r);
            seed_subtree(n->children[1], c_r, b_1r, a_r);
        } else if (!backfacing (a_r, b_0r, b_1r)) {
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

            /* Subdivide this leaf ROAM triangle further until we've
             * reached the finest level. */
            
            project(a_r, a_s);
            project(b_0r, b_0s);
            project(b_1r, b_1s);
                
            seed_triangle (a, b_0, b_1, a_r, b_0r, b_1r, a_s, b_0s, b_1s,
                           0, d->level);

            seeding->triangles_n[0] += 1;
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
    int viewport[4];

    t_copy_modelview (modelview);
    t_copy_projection (projection);

    glGetIntegerv(GL_VIEWPORT, viewport);
    aspect = (double)viewport[2] / (double)viewport[3];

    seeding_in->triangles_n[0] = 0;
    seeding_in->triangles_n[1] = 0;
    n_min = 1.0 / 0.0;
    n_max = A_total = 0;
    
    context = context_in;
    seeding = seeding_in;
}

void seed_tile (int i)
{
    seeding_Bin *b;
    roam_Triangle *n_0, *n_1;
    roam_Diamond *d, *p_0, *p_1;
    float *b_0, *b_1, *a_0, *a_1;
    float b_0r[3], b_1r[3], a_0r[3], a_1r[3];
    int q, j;

    /* Flush the bins. */
    
    for (j = 0, b = &seeding->bins[0] ; j < BINS_N ; j += 1, b += 1) {
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

    /* Transform the base vertices of the tile to rotated space. */

    t_transform_4RT3(b_0r, modelview, b_0);
    t_transform_4RT3(b_1r, modelview, b_1);
    t_transform_4RT3(a_0r, modelview, a_0);
    t_transform_4RT3(a_1r, modelview, a_1);

    seed_subtree(n_0, a_0r, b_0r, b_1r);
    seed_subtree(n_1, a_1r, b_1r, b_0r);
}

void finish_seeding ()
{
    recenter_bins(n_max / n_min);
        
    {
        static int n = 0;

        n += 1;
        A_mean += (A_total - A_mean) / n;
    }
    /* _TRACE ("%d\n", seeding->triangles_n[1]); */
    /* _TRACE ("%f\n", A_mean); */
}
