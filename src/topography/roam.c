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

#define CLAMP(x, low, high)  (((x) > (high)) ? (high) : (((x) < (low)) ? (low) : (x)))

static roam_Context *context;
static float viewport[4], transform[16], planes[6][4];

static void calculate_view_frustum()
{
    float *T, (*pi)[4];
    int i;

    T = transform;
    pi = planes;

    /* Left. */

    pi[0][0] = T[3] + T[0];
    pi[0][1] = T[7] + T[4];
    pi[0][2] = T[11] + T[8];
    pi[0][3] = T[15] + T[12];

    /* Right. */

    pi[1][0] = T[3] - T[0];
    pi[1][1] = T[7] - T[4];
    pi[1][2] = T[11] - T[8];
    pi[1][3] = T[15] - T[12];

    /* Bottom. */

    pi[2][0] = T[3] + T[1];
    pi[2][1] = T[7] + T[5];
    pi[2][2] = T[11] + T[9];
    pi[2][3] = T[15] + T[13];

    /* Top. */

    pi[3][0] = T[3] - T[1];
    pi[3][1] = T[7] - T[5];
    pi[3][2] = T[11] - T[9];
    pi[3][3] = T[15] - T[13];

    /* Near. */

    pi[4][0] = T[3] + T[2];
    pi[4][1] = T[7] + T[6];
    pi[4][2] = T[11] + T[10];
    pi[4][3] = T[15] + T[14];

    /* Far. */

    pi[5][0] = T[3] - T[2];
    pi[5][1] = T[7] - T[6];
    pi[5][2] = T[11] - T[10];
    pi[5][3] = T[15] - T[14];

    /* Normalize the planes. */

    for(i = 0 ; i < 6 ; i += 1) {
        float m;

	m = sqrt(pi[i][0] * pi[i][0] +
		 pi[i][1] * pi[i][1] +
		 pi[i][2] * pi[i][2]);

	pi[i][0] /= m;
	pi[i][1] /= m;
	pi[i][2] /= m;
	pi[i][3] /= m;
    }
}
 
void switch_to_context(roam_Context *new)
{
    context = new;
}

void look_up_sample(roam_Tileset *tiles, int i, int j, double *h, double *e)
{
    int a, b, c, d;
    unsigned int k, l, m;
    
    a = tiles->size[0];
    b = tiles->size[1];
    c = tiles->depth;
/*     d = (1 << tiles->depth) + 1; */
    d = 1 << tiles->depth;

    if (h) {
	*h = 0.0;
    }
	
    if (e) {
	*e = 0.0;
    }

    if (i >= 0 && i <= a * d && j >= 0 && j <= b * d) {
	/* Split the global index into patch/sample
	   indices and do the look-up. */
	
	k = ((j / d) * a) + (i / d);

	if (k >= a * b || !tiles->samples[k]) {
	    d += 1;
	    k = ((j / d) * a) + (i / d);
	}
    	
	if (tiles->samples[k]) {
	    double s, delta;
	    
	    delta = tiles->offsets[k];
	    s = tiles->scales[k];
	    l = tiles->orders[k];
	    m = (((j % d) >> (c - l)) << l) + ((j % d + i % d) >> (c - l));

	    if (h) {
		*h = s * tiles->samples[k][m] + delta;
	    }

	    /* if (!e) { */
	    /* 	printf ("(%d, %d) => %d (%d), %d, %d, %f\n", */
	    /* 		i, j, k, l, m, tiles->samples[k][m], *h); */
	    /* } */
	    /* printf ("%d => %f (%f, %f)\n", tiles->samples[k][m], *h, s, delta); */
	    
	    if (e) {
		int e_i = tiles->bounds[k][m];

		/* Interpret that maximum representable error bound
		   as 'infinite error'.  This basically means that
		   the diamond will always have maximum priority and
		   the respective triangles will span all planes. */

		*e = (e_i == USHRT_MAX) ? ((double)1 / 0) : (s * e_i);
	    }
	}
    }
}

static roam_Block *allocate_fresh_block (int pool)
{
    roam_Block *block;
    roam_Chunk *chunk;
    int i, size;

    size = pool == TRIANGLE_POOL ? sizeof (roam_Triangle) : sizeof (roam_Diamond);
    
    block = (roam_Block *)malloc (BLOCKING * size + sizeof (roam_Block));
    block->chunks = NULL;
    block->next = NULL;

    for (i = 0 ; i < BLOCKING ; i += 1) {
	chunk = (roam_Chunk *)(((char *)block) + sizeof (roam_Block) + i * size);
	chunk->next = block->chunks;
	block->chunks = chunk;
    }

    context->blocks[pool] += 1;

/*     printf ("Allocated block %d at %p.\n", context->blocks[pool], block); */

    return block;
}

static void *allocate_chunk (int pool)
{
    roam_Block *block;
    roam_Chunk *chunk;

    for (block = context->pools[pool];
	 block && !block->chunks;
	 block = block->next);

    if (!block) {
	block = allocate_fresh_block (pool);
	
	block->next = context->pools[pool];
	context->pools[pool] = block;
    }

    chunk = block->chunks;
    block->chunks = chunk->next;

    context->chunks[pool] += 1;

/*     printf ("Allocated chunk %d from block %p at %p (next comes %p).\n", */
/* 	    context->chunks, block, chunk, chunk->next); */
    
    assert(chunk);

    return (void *)chunk;
}

static void free_chunk(int pool, void *chunk)
{
    assert (context->pools[pool]);
/*     printf ("Freeing chunk %d.\n", context->chunks); */
    context->chunks[pool] -= 1;

    ((roam_Chunk *)chunk)->next = context->pools[pool]->chunks;
    context->pools[pool]->chunks = chunk;
}

static void allocate_diamonds(roam_Diamond **d, int n)
{
    int i;
    
    for(i = 0 ; i < n ; i += 1) {
        d[i] = (roam_Diamond *)allocate_chunk (DIAMOND_POOL);
    }

    context->diamonds += n;
}

static void deallocate_diamonds(roam_Diamond **d, int n)
{
    int i;
    
    for(i = 0 ; i < n ; i += 1) {
        free_chunk(DIAMOND_POOL, d[i]);
    }

    context->diamonds -= n;
}

static void initialize_diamond(roam_Diamond *d, roam_Triangle *n,
                               float *v0, float *v1, int l)
{
    double c[3], e;

    if(l < TREE_HEIGHT - 1) {
	c[0] = (v0[0] + v1[0]) * 0.5;
	c[1] = (v0[1] + v1[1]) * 0.5;

	look_up_sample (&context->tileset, (int)c[0], (int)c[1], &c[2], &e);

	assert((nearbyint(c[0]) == c[0] && nearbyint(c[1]) == c[1]));
    } else {
	c[0] = (v0[0] + v1[0]) * 0.5;
	c[1] = (v0[1] + v1[1]) * 0.5;
	c[2] = (v0[2] + v1[2]) * 0.5;

	e = 0.0;
    }

    d->left = NULL;
    d->right = NULL;
    
    d->queue = NULL;
    d->triangle = n;

    d->vertices[0][0] = v0[0];
    d->vertices[0][1] = v0[1];
    d->vertices[0][2] = v0[2];

    d->vertices[1][0] = v1[0];
    d->vertices[1][1] = v1[1];
    d->vertices[1][2] = v1[2];

    d->center[0] = c[0];
    d->center[1] = c[1];
    d->center[2] = c[2];

    d->priority = 0;
    d->error = e;

    d->level = l;
    d->flags = 0;
}

static void flip_diamond(roam_Diamond *d)
{
    d->triangle = d->triangle->neighbors[2];
    d->flags ^= FLIPPED;
}

static void prioritize_diamond(roam_Diamond *d)
{
    float e, w, h;
    float *C, p_w[3], p_c[3], dp_c[3], dp_s[2], p_0[3], p_1[3];
    int i;
    
    e = d->error;

    if (isinf(d->error)) {
	d->priority = QUEUE_SIZE - 1;
    } else {
	p_w[0] = d->center[0];
	p_w[1] = d->center[1];
	p_w[2] = 0.5 * (d->vertices[0][2] + d->vertices[1][2]);

	C = transform;    
	w = viewport[2];
	h = viewport[3];

	/* Project the point to screen space. */

	p_c[0] = C[0] * p_w[0] + C[4] * p_w[1] + C[8] * p_w[2] + C[12];
	p_c[1] = C[1] * p_w[0] + C[5] * p_w[1] + C[9] * p_w[2] + C[13];
	p_c[2] = C[3] * p_w[0] + C[7] * p_w[1] + C[11] * p_w[2] + C[15];

	/* Project the error bound at the
	   center point into screen space. */

	dp_c[0] = e * C[8];
	dp_c[1] = e * C[9];
	dp_c[2] = e * C[11];

	p_0[0] = p_c[0] + dp_c[0];
	p_0[1] = p_c[1] + dp_c[1];
	p_0[2] = p_c[2] + dp_c[2];

	p_1[0] = p_c[0] - dp_c[0];
	p_1[1] = p_c[1] - dp_c[1];
	p_1[2] = p_c[2] - dp_c[2];

	dp_s[0] = 0.5 * w * (p_0[0] / p_0[2] - p_1[0] / p_1[2]);
	dp_s[1] = 0.5 * h * (p_0[1] / p_0[2] - p_1[1] / p_1[2]);

	i = (int)sqrt(dp_s[0] * dp_s[0] + dp_s[1] * dp_s[1]);
    
	if(is_visible(d)) {
	    d->priority = CLAMP(i, 0, QUEUE_SIZE - 1);
	} else {
	    d->priority = (CLAMP(i, 0, QUEUE_SIZE - 1) + 1) >> 1;
	}
    }
}

static void queue_diamond(roam_Diamond *d, roam_Diamond **q, int i)
{
    d->left = NULL;
    d->right = q[i];

    if (q[i]) {
	q[i]->left = d;
    }
    
    q[i] = d;
    d->queue = q[i];
}

static void dequeue_diamond(roam_Diamond *d, roam_Diamond **q, int i)
{
    if (d->left) {
	d->left->right = d->right;
    } else {
	q[i] = d->right;
    }

    if (d->right) {
	d->right->left = d->left;
    }

    d->left = NULL;
    d->right = NULL;
    d->queue = NULL;
}

static void queue_into_Qs(roam_Diamond *d)
{
    roam_Diamond **Qs = context->queues[0];

    if(is_splittable(d)) {
        prioritize_diamond(d);
        queue_diamond(d, Qs, d->priority);

        /* Update maximum split priority. */

	if(d->priority > context->maximum) {
            context->maximum = d->priority;
        }

        context->queued[0] += 1;
    }
}

static void dequeue_from_Qs(roam_Diamond *d)
{
    roam_Diamond **Qs = context->queues[0];
    int i;

    if(is_queued(d)) {
        dequeue_diamond(d, Qs, d->priority);

        /* Update maximum split priority. */

        for(i = context->maximum ; i > 0 && !Qs[i] ; i -= 1);
        context->maximum = i;

        context->queued[0] -= 1;
    }
}

static void queue_into_Qm(roam_Diamond *d)
{
    roam_Diamond **Qm = context->queues[1];

    /* Fine triangles are never put into Qs but
       they might be force-split nevertheless and
       should therefore be alllowed to be put into
       Qm. */

    if(is_mergeable(d)) {
        prioritize_diamond(d);
        queue_diamond(d, Qm, d->priority);

        /* Update minimum merge priority. */
	
        if(d->priority < context->minimum) {
            context->minimum = d->priority;
        }

        context->queued[1] += 1;
    }
}

static void dequeue_from_Qm(roam_Diamond *d)
{
    roam_Diamond **Qm = context->queues[1];
    int i;

    if(is_queued(d)) {
        dequeue_diamond(d, Qm, d->priority);

        /* Update minimum merge priority. */

        for(i = context->minimum ; i < QUEUE_SIZE - 1 && !Qm[i] ; i += 1);
        context->minimum = i;
        context->queued[1] -= 1;
    }
}

static void allocate_triangles(roam_Triangle **n, int k)
{
    int i;
    
    for(i = 0 ; i < k ; i += 1) {
        n[i] = (roam_Triangle *)allocate_chunk (TRIANGLE_POOL);
    }

    context->triangles += k;
}

static void deallocate_triangles(roam_Triangle **n, int k)
{
    int i;
    
    for(i = 0 ; i < k ; i += 1) {
	free_chunk (TRIANGLE_POOL, n[i]);
    }

    context->triangles -= k;
}

static void initialize_triangle(roam_Triangle *c, roam_Diamond *d, roam_Triangle *p,
                                roam_Triangle *n_0, roam_Triangle *n_1, roam_Triangle *n_2,
				int i)
{
    c->diamond = d;

    c->parent = p;
    c->children[0] = NULL;
    c->children[1] = NULL;

    c->neighbors[0] = n_0;
    c->neighbors[1] = n_1;
    c->neighbors[2] = n_2;

    /* 
     * Initialize the triangle to OUT to make
     * sure it will be properly counted	during
     * culling later on.
     */
    
    c->index = i;
    c->flags = 0;
    c->cullbits = OUT;

    /* if (p && d->error > p->diamond->error + 0.25) { */
    /* 	printf ("??? %d: (%g, %g):%g => (%g,%g):%g\n", */
    /* 		d->level, */
    /* 		p->diamond->center[0], p->diamond->center[1], p->diamond->error, */
    /* 		d->center[0], d->center[1], d->error); */
    /* } */
}

static void classify_triangle(roam_Triangle *n, int b)
{
    float (*p)[4], *v[3];
    double r_0, r_min, r_max, r[3];
    int i, j, k, l;

    /* printf ("%f, %f, %d\n", n->diamond->error, n->parent->diamond->error, b); */
    /* assert (!(isinf(n->diamond->error)) || b == 0 || b == INVALID); */

    if(b != OUT && b != ALL_IN) {
        r_0 = n->diamond->error;

	v[2] = n->diamond->center;

	if (isinf (r_0)) {
	    b = 0;
	} else {
	    p = planes;
	
	    v[0] = n->diamond->vertices[0];
	    v[1] = n->diamond->vertices[1];
	    v[2] = n->parent->diamond->center;

	    /* While it hasn't been found to be OUT, test
	       the triangle against the planes its parent
	       wasn't IN. */

	    for (i = 0, j = 1 ; i < 6 && b != OUT ; i += 1, j <<= 1) {
		if (!(b & j)) {
		    for(k = 0 ; k < 3 ; k += 1) {
			r[k] = (p[i][0] * v[k][0] +
				p[i][1] * v[k][1] +
				p[i][2] * v[k][2] +
				p[i][3]);
		    }

		    /* Take the minimum distance over all vertices. */
 
		    for(l = 0, r_min = +1.0 / 0.0;
			l < 3;
			r_min = r[l] < r_min ? r[l] : r_min, l += 1);

		    /* And the maximum distance. */
	       
		    for(l = 0, r_max = -1.0 / 0.0;
			l < 3;
			r_max = r[l] > r_max ? r[l] : r_max, l += 1);

		    if (r_min > r_0 && r_max > -r_0) {
			b |= j;
		    } else if(r_min < r_0 && r_max < -r_0) {
			b = OUT;
		    }
		}
	    }
	}
    }

    n->cullbits = b;
}

static void expand_triangle(roam_Triangle *p)
{
    roam_Triangle *c[2];
    roam_Diamond *d[2];
    int i;

    c[0] = p->children[0];
    c[1] = p->children[1];

    /* Update the links of neighboring triangles after a split. */
    
    for(i = 0 ; i < 3 && p->neighbors[0]->neighbors[i] != p ; i += 1);
    p->neighbors[0]->neighbors[i] = c[0];

    for(i = 0 ; i < 3 && p->neighbors[1]->neighbors[i] != p ; i += 1);
    p->neighbors[1]->neighbors[i] = c[1];

    /* If the new triangles aren't part of existing diamonds, create
       new ones. */

    /* For primary triangles within a diamond, vertices[0] is the left
       and vertices[1] the right vertex quite unlike secondary triangles
       where this order is reversed. */

    i = is_primary(p);

    /* Left child. */
    
    if(p->neighbors[0]->neighbors[2] == c[0]) {
	d[0] = p->neighbors[0]->diamond;
	assert (d[0]->level == p->diamond->level + 1);
    } else {
	assert (p->neighbors[0]->diamond->level == p->diamond->level);
	allocate_diamonds(&d[0], 1);

	initialize_diamond(d[0], c[0],
			   p->parent->diamond->center,
			   p->diamond->vertices[!i],
			   p->diamond->level + 1);
    }

    initialize_triangle(c[0], d[0], p,
                        c[1], p->neighbors[2]->children[1], p->neighbors[0],
			p->index);

    /* 
     * If the new triangle is at the finest cached level
     * lock it to prevent splits until further notice.
     */

    classify_triangle(c[0], p->cullbits);
    
    /* Right child. */

    if(p->neighbors[1]->neighbors[2] == c[1]) {
	d[1] = p->neighbors[1]->diamond;
	assert (d[1]->level == p->diamond->level + 1);
    } else {
	assert (p->neighbors[1]->diamond->level == p->diamond->level);
	allocate_diamonds(&d[1], 1);

	initialize_diamond(d[1], c[1],
			   p->diamond->vertices[i],
			   p->parent->diamond->center,
			   p->diamond->level + 1);
    }

    initialize_triangle(c[1], d[1], p,
                        p->neighbors[2]->children[0], c[0], p->neighbors[1],
			p->index);
    
    classify_triangle(c[1], p->cullbits);
    
    /* Update the book. */
    
    context->visible += !is_out(c[0]) + !is_out(c[1]) - !is_out(p);
}

static void collapse_triangle(roam_Triangle *p)
{
    roam_Triangle *c[2];
    roam_Diamond *d[2];
    int i;

    c[0] = p->children[0];
    c[1] = p->children[1];

    d[0] = c[0]->diamond;
    d[1] = c[1]->diamond;

    /* Deallocate the children's diamonds if they
       aren't referenced by their base neighbors,
       otherwise flip the diamonds to promote the
       remaining secondary triangle to primary. */

    if(d[0] != c[0]->neighbors[2]->diamond) {
	deallocate_diamonds(&d[0], 1);
    } else if(d[0]->triangle == c[0]) {
	flip_diamond(d[0]);
    }

    if(d[1] != c[1]->neighbors[2]->diamond) {
	deallocate_diamonds(&d[1], 1);
    } else if(d[1]->triangle == c[1]) {
	flip_diamond(d[1]);
    } 

    p->neighbors[0] = c[0]->neighbors[2];
    p->neighbors[1] = c[1]->neighbors[2];

    /* Update the links of neighboring triangles after a merge. */

    for(i = 0 ; i < 3 && p->neighbors[0]->neighbors[i] != c[0] ; i += 1);
    p->neighbors[0]->neighbors[i] = p;

    for(i = 0 ; i < 3 && p->neighbors[1]->neighbors[i] != c[1] ; i += 1);
    p->neighbors[1]->neighbors[i] = p;

    p->children[0] = NULL;
    p->children[1] = NULL;

    /* Update the book. */

    context->visible -= !is_out(c[0]) + !is_out(c[1]) - !is_out(p);
}

static int split_triangle_pair(roam_Triangle *n)
{
    roam_Triangle *p[2];

    /* Force-split the triangle's base neighbor if needed. */

    if((is_pair(n) || split_triangle_pair(n->neighbors[2])) &&
       !is_locked(n) && !is_locked(n->neighbors[2])) {

        /* At this point the situation should look like this:

		   +
		  /.\
		 / . \  p0
		/  .  \ 
	       /c00.c01\
	   v0 /    .    \ v1
	 - - +-----x-----+ - - 
	      \    .    /
	       \c11.c10/
		\  .  /   
		 \ . /  p1
		  \./
		   +
	*/
	
	p[0] = n;
	p[1] = n->neighbors[2];

	/* Allocate the children. */

	allocate_triangles(p[0]->children, 2);
	allocate_triangles(p[1]->children, 2);

	assert(p[0]->diamond == p[1]->diamond);
    
	/* Remove ancestors from the queues. */

	dequeue_from_Qs(p[1]->diamond);

	dequeue_from_Qm(p[0]->parent->diamond);
	dequeue_from_Qm(p[1]->parent->diamond);

	/* Expand the parents. */
    
	expand_triangle(p[0]);
	expand_triangle(p[1]);

	queue_into_Qm(p[0]->diamond);
	queue_into_Qm(p[1]->diamond);

	/* Add the children to the split queue. */
    
	queue_into_Qs(p[0]->children[0]->diamond);
	queue_into_Qs(p[0]->children[1]->diamond);
	queue_into_Qs(p[1]->children[0]->diamond);
	queue_into_Qs(p[1]->children[1]->diamond);

	/* if (!(p[0]->diamond->error >= p[0]->children[0]->diamond->error && */
	/*       p[0]->diamond->error >= p[0]->children[1]->diamond->error && */
	/*       p[0]->diamond->error >= p[1]->children[0]->diamond->error && */
	/*       p[0]->diamond->error >= p[1]->children[1]->diamond->error && */
	/*       p[1]->diamond->error >= p[0]->children[0]->diamond->error && */
	/*       p[1]->diamond->error >= p[0]->children[1]->diamond->error && */
	/*       p[1]->diamond->error >= p[1]->children[0]->diamond->error && */
	/*       p[1]->diamond->error >= p[1]->children[1]->diamond->error)) { */
	/*     printf ("!!! (%f, %f) -> (%f, %f, %f, %f)\n",  */
	/* 	    p[0]->diamond->error, p[1]->diamond->error, */
	/* 	    p[0]->children[0]->diamond->error, */
	/* 	    p[0]->children[1]->diamond->error, */
	/* 	    p[1]->children[0]->diamond->error, */
	/* 	    p[1]->children[1]->diamond->error); */
	/* } */
	
	return 1;
    } else {
	return 0;
    }
}

static void merge_triangle_pair(roam_Triangle *n)
{
    roam_Triangle *p[2], *c[4];

    p[0] = n;
    p[1] = n->neighbors[2];

    c[0] = p[0]->children[0];
    c[1] = p[0]->children[1];
    c[2] = p[1]->children[0];
    c[3] = p[1]->children[1];
    
    assert(p[0]->diamond == p[1]->diamond);
    
    /* Update the queues. */

    dequeue_from_Qm(p[0]->diamond); 

    dequeue_from_Qs(p[0]->children[0]->diamond);
    dequeue_from_Qs(p[0]->children[1]->diamond); 
    dequeue_from_Qs(p[1]->children[0]->diamond);
    dequeue_from_Qs(p[1]->children[1]->diamond);
		    
    collapse_triangle(p[0]);
    collapse_triangle(p[1]);

    queue_into_Qs(p[0]->diamond);
    queue_into_Qs(p[1]->diamond);

    queue_into_Qm(p[0]->parent->diamond);
    queue_into_Qm(p[1]->parent->diamond);

    /* Deallocate the children. */

    deallocate_triangles(c, 4);
}

static void reorder_queue(roam_Diamond **Q)
{
    roam_Diamond *l, *d, *next;
    int i;

    for(i = 0, l = NULL; i < QUEUE_SIZE ; i += 1) {
        for(d = Q[i] ; d ; d = next) {
            next = d->right;

	    dequeue_diamond (d, Q, i);
	    queue_diamond (d, &l, 0);
        }
    }

    while((d = l)) {
        prioritize_diamond(d);

	dequeue_diamond (d, &l, 0);
	queue_diamond (d, Q, d->priority);
    }
}

static void reclassify_subtree(roam_Triangle *n, int p)
{
    int f;

    f = n->cullbits;
    
    classify_triangle(n, p);
        
    if(!is_leaf(n)) {
        /* If the triangle was OUT or ALL_IN and still is
	   then there's nothing more to be done for its
	   subtree, it will still be correct. */

        if ((f & n->cullbits) != OUT &&
	    (f & n->cullbits) != ALL_IN) {
            reclassify_subtree(n->children[0], n->cullbits);
            reclassify_subtree(n->children[1], n->cullbits);
        }
    } else { 
        /* If the OUT bit changed state update
           the book and queue accordingly. */

        if ((f ^ n->cullbits) & OUT) {
            if (n->cullbits & OUT) {
                dequeue_from_Qs(n->diamond);

                context->visible -= 1;
            } else {
                queue_into_Qs(n->diamond);

                context->visible += 1;
            }
        }
    }

    context->culled += 1;
}

#if 0

static void test_subtree(roam_Triangle *r)
{
    /* ... */
    puts("**");
    assert (!(r->cullbits & ALL_IN) || !(r->cullbits & OUT));
    assert (!(r->cullbits & OUT) || r->cullbits == OUT);
    
    if(!is_leaf(r)) {
	test_subtree(r->children[0]);
	test_subtree(r->children[1]);
    }
}

static int split_highest_pairs(lua_State *L)
{
    roam_Diamond *d;
    int i, j, n;

    n = (int)lua_tonumber(L, 1);

    /* Update queue priorities. */
 
    reorder_queue(context->queues[0]);
    reorder_queue(context->queues[1]);

    /* And make sure the book stays up-to-date as well. */
 
    for(i = 0 ; i < QUEUE_SIZE - 1 && !context->queues[1][i] ; i += 1);
    for(j = QUEUE_SIZE - 1 ; j >= 1 && !context->queues[0][j] ; j -= 1);

    context->minimum = i;
    context->maximum = j;

    for(i = 0 ; i < n && context->maximum > 0 ; i += 1) {
	d = context->queues[0][context->maximum];
	split_triangle_pair(d->triangle);
    }

    return 0;
}

static int merge_lowest_pairs(lua_State *L)
{
    roam_Diamond *d;
    int i, j, n;

    n = (int)lua_tonumber(L, 1);

    /* Update queue priorities. */

    reorder_queue(context->queues[0]);
    reorder_queue(context->queues[1]);

    /* And make sure the book stays up-to-date as well. */

    for(i = 0 ; i < QUEUE_SIZE - 1 && !context->queues[1][i] ; i += 1);
    for(j = QUEUE_SIZE - 1 ; j >= 1 && !context->queues[0][j] ; j -= 1);

    context->minimum = i;
    context->maximum = j;

    for(i = 0 ; i < n && context->minimum < QUEUE_SIZE - 1 ; i += 1) {
	d = context->queues[1][context->minimum];
	merge_triangle_pair(d->triangle);
    }

    return 0;
}

static void create_levels(roam_Triangle *r, int n)
{
    if(n > 0) {
	if(is_leaf(r)) {
	    split_triangle_pair(r);
	}

	create_levels(r->children[0], n - 1);
	create_levels(r->children[1], n - 1);
    }
}

static int create_tree_level(lua_State *L)
{
    int i, n;

    n = (int)lua_tonumber(L, 1);
    
    for (i = 0 ; i < context->size[0] * context->size[1] ; i += 1) {
	create_levels(context->roots[i][0], n);
	create_levels(context->roots[i][1], n);
    }

    return 0;
}

static void traverse_quadtree(roam_Triangle *n, roam_Triangle *m, int l)
{
    if(n && is_pair(n)) {
  	if(!is_leaf(n)) {
	    traverse_quadtree(n->children[1]->children[1],
			      n->children[0]->children[0],
			      l + 1);
	    
	    traverse_quadtree(n->children[1]->children[0],
			      m->children[0]->children[1],
			      l + 1);

	    traverse_quadtree(n->children[0]->children[1],
			      m->children[1]->children[0],
			      l + 1);

	    traverse_quadtree(m->children[0]->children[0],
			      m->children[1]->children[1],
			      l + 1);
	}

	glColor3f(1, 0, 0);
	
	glBegin(GL_QUADS);
	glVertex3fv(n->diamond->vertices[0]);
	glVertex3fv(n->parent->diamond->center);
	glVertex3fv(n->diamond->vertices[1]);
	glVertex3fv(m->parent->diamond->center);
	glEnd();
    }
}

#endif

static void draw_subtree(roam_Triangle *n)
{
    if(!is_out(n)) {
	if(!is_leaf(n)) {
	    draw_subtree(n->children[0]);
	    draw_subtree(n->children[1]);
	} else {
	    roam_Triangle *p;
	    roam_Diamond *d, *e;
            float *b;
	    int i, k;

	    p = n->parent;
	    d = n->diamond;
	    e = p->diamond;
	    i = is_primary(n);

            b = context->buffer;
            k = context->drawn;
            
            memcpy (b + 9 * k, d->vertices[!i], 3 * sizeof(float));
            memcpy (b + 9 * k + 3, d->vertices[i], 3 * sizeof(float));
            memcpy (b + 9 * k + 6, e->center, 3 * sizeof(float));

	    context->drawn += 1;
	}
    }
}

void optimize_geometry(roam_Context *new, int frame)
{
    roam_Tileset *tiles;
    roam_Diamond *d = NULL, *d_0;
    float M[16], P[16];
    int i, j, delta, overlap;

    context = new;

    /* Don't optimize more than once per frame. */

    assert(context->frame <= frame);
    if (context->frame == frame) {
        return;
    }
    
    tiles = &context->tileset;
    
    /* Update the book. */

    context->culled = 0;

    glGetFloatv(GL_VIEWPORT, viewport);

    /* Combine the modelview and projection matrices. */

    t_copy_modelview(M);
    t_copy_projection(P);
    t_concatenate_4T(transform, P, M);
    
    calculate_view_frustum();

    /* Reclassify the mesh for the current frame. */
    
    for (i = 0 ; i < tiles->size[0] * tiles->size[1] ; i += 1) {
	reclassify_subtree(context->roots[i][0], 0);
	reclassify_subtree(context->roots[i][1], 0);
    }

    /* Update the queue priorities... */
    
    reorder_queue(context->queues[0]);
    reorder_queue(context->queues[1]);

    /* ...and make sure the book stays up-to-date as well. */
    
    for(i = 0 ; i < QUEUE_SIZE - 1 && !context->queues[1][i] ; i += 1);
    for(j = QUEUE_SIZE - 1 ; j >= 1 && !context->queues[0][j] ; j -= 1);

    context->minimum = i;
    context->maximum = j;

    /* Now split and/or merge until the queue priorities
       don't overlap and the target triangle count has
       (more or less) been reached.

       Also make sure we don't continuously split and
       remerge the same diamond. */

    do {
	delta = context->visible - context->target;
	overlap = context->maximum - context->minimum;
	d_0 = d;

	/* If both of the conditions below fail we can't
	   proceed and the fact that d stays equal to d_0
	   makes sure we exit the loop. */

        if(delta < 0 && context->maximum > 0) {
            d = context->queues[0][context->maximum];

	    assert(!is_fine(d));
	    if(!split_triangle_pair(d->triangle)) {
		assert(is_leaf(d->triangle));
		dequeue_from_Qs(d);
	    }
        } else if (context->minimum < QUEUE_SIZE - 1) {
            d = context->queues[1][context->minimum];

            merge_triangle_pair(d->triangle);
        }

    } while (d_0 != d && (overlap > 1 || abs(delta) > 5));

    while (context->visible > context->target) {
        d = context->queues[1][context->minimum];

        merge_triangle_pair(d->triangle);
    }

    context->frame = frame;
    
    /* for (i = 0 ; i < context->size[0] * context->size[1] ; i += 1) { */
    /* 	test_subtree(context->roots[i][0]); */
    /* 	test_subtree(context->roots[i][1]); */
    /* } */
    
#if 0
    printf("%d triangles, %d diamonds, %d culled, %d visible, %d drawn\n"
           "Qs_max = %d, Qm_min = %d, |Qs| = %d, |Qm| = %d\n",
           context->triangles, context->diamonds,
	   context->culled, context->visible, context->drawn,
           context->maximum, context->minimum,
           context->queued[0], context->queued[1]);
#endif
}

void draw_geometry(roam_Context *new, float *buffer, int *ranges)
{
    roam_Tileset *tiles;
    int i, j;

    context = new;
    tiles = &context->tileset;
    
    /* Draw the geometry into the provided buffers. */
    
    context->buffer = buffer;
    context->drawn = 0;

    for (i = 0 ; i < tiles->size[0] ; i += 1) {    
	for (j = 0 ; j < tiles->size[1] ; j += 1) {
	    int k = i * tiles->size[1] + j;

	    draw_subtree(context->roots[k][0]);
	    draw_subtree(context->roots[k][1]);

            ranges[k] = context->drawn;
	}
    }
}       

static void seed_triangle(float *a, float *b_0, float *b_1, int level)
{
    float c[2];
    
    if (level < TREE_HEIGHT - 1) {
        c[0] = 0.5 * (b_0[0] + b_1[0]);
        c[1] = 0.5 * (b_0[1] + b_1[1]);

        seed_triangle(c, a, b_0, level + 1);
        seed_triangle(c, b_1, a, level + 1);
    } else {
        glVertex2f((a[0] + b_0[0] + b_1[0]) / 3.0,
                   (a[1] + b_0[1] + b_1[1]) / 3.0);
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
	    int i;

	    p = n->parent;
	    d = n->diamond;
	    e = p->diamond;
	    i = is_primary(n);

            seed_triangle (e->center, d->vertices[!i], d->vertices[i],
                           d->level);
	}
    }
}

void seed_vegetation(roam_Context *new)
{
    roam_Tileset *tiles;
    int i, j;

    context = new;
    tiles = &context->tileset;

    for (i = 0 ; i < tiles->size[0] ; i += 1) {    
	for (j = 0 ; j < tiles->size[1] ; j += 1) {
	    int k = i * tiles->size[1] + j;

	    seed_subtree(context->roots[k][0]);
	    seed_subtree(context->roots[k][1]);
	}
    }
}

void *allocate_mesh(roam_Context *new)
{
    roam_Tileset *tiles;
    roam_Triangle *(*T)[4];
    roam_Diamond *(*D)[3];
    int i, j, d, s, t;

    context = new;
    
    tiles = &context->tileset;

    context->frame = 0;
    context->triangles = 0;
    context->diamonds = 0;
    context->culled = 0;
    context->visible = 0;
    context->queued[0] = 0;
    context->queued[1] = 0;

    context->minimum = QUEUE_SIZE - 1;
    context->maximum = 0;

    context->pools[0] = NULL;
    context->pools[1] = NULL;

    context->blocks[0] = 0;
    context->blocks[1] = 0;
    context->chunks[0] = 0;
    context->chunks[1] = 0;

    context->roots = (roam_Triangle *(*)[2])calloc(context->tileset.size[0] *
                                                   context->tileset.size[1],
                                                   sizeof(roam_Triangle *[2]));

    /* The base mesh consists of a 5 by 5 lattice of diamonds.  Only
       the center diamond is visible.  The others are there merely to
       avoid NULL neighbors at the edges of any diamond that can
       potentially be split.  The outer ring of diamonds can never be
       split and are therefore safe to leave  with NULL neighbors (in
       other words: e two layer buffer of diamonds is necessary).
    
       Note that all triangles are marked OUT upon creation so that
       they will get inserted into Qs and counted as visible when we
       reclassify the mesh later on. */

    s = tiles->size[0];
    t = tiles->size[1];
    d = 1 << tiles->depth;

    /* Allocate all needed diamonds and triangles. */

    T = (roam_Triangle *(*)[4])calloc((s + 4) * (t + 4), sizeof(roam_Triangle *[4]));
    D = (roam_Diamond *(*)[3])calloc((s + 4) * (t + 4), sizeof(roam_Diamond *[3]));
    
    for (i = 0 ; i < (s + 4) * (t + 4) ; i += 1) {
	allocate_triangles(T[i], 4);
	allocate_diamonds(D[i], 3);
    }

    /* Initialize and hook up the base mesh. */

    for (i = 0 ; i < s ; i += 1) {
	for (j = 0 ; j < t ; j += 1) {
	    roam_Triangle **T_ij;
	    roam_Triangle **R_ij;

	    R_ij = context->roots[i * t + j];
	    T_ij = T[(i + 2) * (t + 4) + j + 2];

	    R_ij[0] = T_ij[0];
	    R_ij[1] = T_ij[1];
	}
    }

    for (i = 0 ; i < s + 4 ; i += 1) {
	for (j = 0 ; j < t + 4 ; j += 1) {
	    double e, h;
	    float v[4][3];
	    int i_c, i_r, i_l, i_t, i_b, i_p;

	    i_c = i * (t + 4) + j;
	    i_r = i * (t + 4) + j + 1;
	    i_l = i * (t + 4) + j - 1;
	    i_t = (i + 1) * (t + 4) + j;
	    i_b = (i - 1) * (t + 4) + j;
	    i_p = CLAMP(i - 2, 0, s - 1) * t + CLAMP(j - 2, 0, t - 1);
	    
	    /* Each diamond in the base mesh should look
	       like this:
	      
                           .       .
                          .         .
                         .           +
                        .             .
                       .               .
                      .                 .
                     .                   .
                 v1 .                     . v2
                   +...........+-----------+
                    .          |          /|.
                     .         |  ......./ | .
                      .        |  .     /  |  .
                       .       |  .    /.  |   .
                        .      |  .   / .  |    .
                         .     |  D[i][j]  |     .
                          .    |  . /   .  |      .
                           .   |  ./    .  |       .
                            .  |  /     .  |        .
                             . | /.......  |         .
                              .|/          |          .
                               +-----------+...........+
                             v0| === d === |           . v3
                                 .                   .
                                  .                 .
                                   .               .
                                    .             .
				     .           .
	    */

	    /* Calculate the base mesh vertices. */
	    
	    v[0][0] = (j - 2) * d;
	    v[0][1] = (i - 2) * d;
	    look_up_sample (tiles, v[0][0], v[0][1], &h, &e);
	    v[0][2] = h;

	    v[1][0] = (j - 3) * d;
	    v[1][1] = (i - 1) * d;
	    look_up_sample (tiles, v[1][0], v[1][1], &h, &e);
	    v[1][2] = h;

	    v[2][0] = (j - 1) * d;
	    v[2][1] = (i - 1) * d;
	    look_up_sample (tiles, v[2][0], v[2][1], &h, &e);
	    v[2][2] = h;

	    v[3][0] = j * d;
	    v[3][1] = (i - 2) * d;
	    look_up_sample (tiles, v[3][0], v[3][1], &h, &e);
	    v[3][2] = h;

	    /* Initialize base diamonds and triangles. */
	    
	    initialize_diamond(D[i_c][0], T[i_c][0], v[0], v[2],  0);
	    initialize_diamond(D[i_c][1], T[i_c][2], v[1], v[2], -1);
	    initialize_diamond(D[i_c][2], T[i_c][3], v[0], v[3], -1);

	    initialize_triangle(T[i_c][2], D[i_c][1], NULL,
				NULL, NULL, NULL, 0);
	    
	    initialize_triangle(T[i_c][3], D[i_c][2], NULL,
				NULL, NULL, NULL, 0);
	    
	    initialize_triangle(T[i_c][0], D[i_c][0], T[i_c][2],
				j > 0 ? T[i_l][1] : NULL,
				i < s + 3 ? T[i_t][1] : NULL,
				T[i_c][1], i_p);
	    
	    initialize_triangle(T[i_c][1], D[i_c][0], T[i_c][3],
				j < t + 3 ? T[i_r][0] : NULL,
				i > 0 ? T[i_b][0] : NULL,
				T[i_c][0], i_p);
	}
    }

    free(T);
    free(D);

    return (void *)context;
}

void free_mesh(roam_Context *new)
{
    roam_Block *block, *next;
    int i;

    context = new;
    
    /* Free all allocated blocks. */
    
    for (i = 0 ; i < 2 ; i += 1) {
	for (block = context->pools[i] ; block ; block = next) {
	    next = block->next;
	
	    free(block);
	}

	context->pools[i] = NULL;
    }

    free(context->roots);
}
