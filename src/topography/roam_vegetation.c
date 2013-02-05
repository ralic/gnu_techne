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

static roam_Context *context;
static float modelview[16];
static int seeds_n;

static void seed_triangle(float *a, float *b_0, float *b_1,
                          float z_a, float z_0, float z_1, int level)
{    
    if (level < TREE_HEIGHT - 1) {
        if ((z_a < 0 || z_0 < 0 || z_1 < 0) &&
            (z_a > -500 || z_0 > -500 || z_1 > -500)) {
            float b_c[3], z_c;
    
            b_c[0] = 0.5 * (b_0[0] + b_1[0]);
            b_c[1] = 0.5 * (b_0[1] + b_1[1]);
            b_c[2] = 0.5 * (b_0[2] + b_1[2]);

            z_c = 0.5 * (z_0 + z_1);

            /* _TRACE ("%f\n", fabs(z_c - 0.5 * (z_0 + z_1))); */
            
            seed_triangle(b_c, a, b_0, z_c, z_a, z_0, level + 1);
            seed_triangle(b_c, b_1, a, z_c, z_1, z_a, level + 1);
        }
    } else {
        float z;
        int i, n;

        /* { */
        /*     float c[3]; */
        /*     c[0] = (a[0] + b_0[0] + b_1[0]) / 3.0; */
        /*     c[1] = (a[1] + b_0[1] + b_1[1]) / 3.0; */
        /*     c[2] = (a[2] + b_0[2] + b_1[2]) / 3.0; */
            
        /*     glVertexAttrib3fv(0, c); */
            
        /*     seeds_n += 1; */
        /* } */

        /* assert (a[0] >= 0 && a[1] >= 0); */
        srand48((long int)(0.5 * (a[0] + a[1]) * (a[0] + a[1] + 1) + a[1]));

        z = fmin(z_0 + z_1 + z_a, 0);
        n = (int)(200 * expf(0.01 * z));

        for (i = 0 ; i < n ; i += 1) {
            double r_1, r_2, sqrtr_1, k[3];
            float c[3];

            r_1 = drand48();
            r_2 = drand48();
            sqrtr_1 = sqrt(r_1);
            
            k[0] = 1 - sqrtr_1;
            k[1] = sqrtr_1 * (1 - r_2);
            k[2] = sqrtr_1 * r_2;
            
            c[0] = k[2] * a[0] + k[0] * b_0[0] + k[1] * b_1[0];
            c[1] = k[2] * a[1] + k[0] * b_0[1] + k[1] * b_1[1];
            c[2] = k[2] * a[2] + k[0] * b_0[2] + k[1] * b_1[2];
            
            glVertexAttrib3fv(0, c);
            
            seeds_n += 1;
        }
    }
}

static void seed_subtree(roam_Triangle *n)
{
    if(!is_out(n)) {
	if(!is_leaf(n)) {
	    seed_subtree(n->children[0]);
	    seed_subtree(n->children[1]);
	} else {
	    roam_Triangle *p;
	    roam_Diamond *d, *e;
            float *a, *b_0, *b_1, z_a, z_0, z_1;
            float u[3], v[3], w[3];
	    int i;

	    p = n->parent;
	    d = n->diamond;
	    e = p->diamond;
	    i = is_primary(n);

            a = e->center;
            b_0 = d->vertices[!i];
            b_1 = d->vertices[i];
            
            u[0] = b_0[0] - a[0];
            u[1] = b_0[1] - a[1];
            u[2] = b_0[2] - a[2];

            v[0] = b_1[0] - a[0];
            v[1] = b_1[1] - a[1];
            v[2] = b_1[2] - a[2];

            t_cross (w, u, v);
            t_normalize_3 (w);

            glVertexAttrib3fv(1, w);

            z_a = modelview[2] * a[0] +
                modelview[6] * a[1] +
                modelview[10] * a[2] +
                modelview[14];

            z_0 = modelview[2] * b_0[0] +
                modelview[6] * b_0[1] +
                modelview[10] * b_0[2] +
                modelview[14];

            z_1 = modelview[2] * b_1[0] +
                modelview[6] * b_1[1] +
                modelview[10] * b_1[2] +
                modelview[14];
            
            seed_triangle (a, b_0, b_1, z_a, z_0, z_1, d->level);
	}
    }
}

void seed_vegetation(roam_Context *new, unsigned int _ls, unsigned int _lo)
{
    roam_Tileset *tiles;
    int i, j;

    seeds_n = 0;
    context = new;
    tiles = &context->tileset;
    
    t_copy_modelview (modelview);

    glUniform1f(_ls, ldexpf(1, -tiles->depth));
    glActiveTexture(GL_TEXTURE0);
    
    for (i = 0 ; i < tiles->size[0] ; i += 1) {    
	for (j = 0 ; j < tiles->size[1] ; j += 1) {
	    int k = i * tiles->size[1] + j;

            glUniform2f(_lo, j, i);
            glBindTexture(GL_TEXTURE_2D, tiles->imagery[k]);

            glBegin(GL_POINTS);
            
	    seed_subtree(context->roots[k][0]);
	    seed_subtree(context->roots[k][1]);
    
            glEnd();
	}
    }

    /* _TRACE ("%d\n", seeds_n); */
}
