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

#ifndef _ROAM_H_
#define _ROAM_H_

#define SEED_BUFFER_SIZE (10000)
#define SEED_SIZE (3 * 3 * 4)
#define VEGETATION_LEVEL_BIAS 0

#define TRIANGLE_CHUNKING_FACTOR (512)
#define DIAMOND_CHUNKING_FACTOR (1024)
#define TRIANGLE_POOL (0)
#define DIAMOND_POOL (1)

#define QUEUE_SIZE (65536)
#define TREE_HEIGHT ((context->tileset.depth << 1) + 1)

/* Triangle cull flags */

#define RIGHT       (1 << 0)
#define LEFT        (1 << 1)
#define TOP         (1 << 2)
#define BOTTOM      (1 << 3)
#define NEAR        (1 << 4)
#define FAR         (1 << 5)
#define OUT         (1 << 6)
#define ALL_IN      (RIGHT | LEFT | TOP | BOTTOM | NEAR | FAR)

/* Triangle flags */

/* ... */

/* Diamond flags */

#define FLIPPED     (1 << 0)

#define is_locked(n) ((n)->diamond->level >=                \
                      (context->tileset.orders[(n)->index] << 1))

#define is_leaf(n) (!(n)->children[0])
#define is_out(n) ((n)->cullbits & OUT)
#define is_primary(n) (n->diamond->flags & FLIPPED ?        \
                       (n)->diamond->triangle != (n) :      \
                       (n)->diamond->triangle == (n))

#define is_pair(n) ((n)->neighbors[2] &&                    \
                    (n)->neighbors[2]->neighbors[2] == (n))

#define is_flagged(d, mask) (((d)->flags & (mask)) != 0)
#define is_fine(d) ((d)->level >= TREE_HEIGHT - 1 || (d)->error == 0.0)
#define is_coarse(d) ((d)->level <= 0 || isinf((d)->error))
#define is_queued(d) ((d) && (d)->queue)
#define is_visible(d) \
    (is_pair((d)->triangle) ?                                                \
     !((d)->triangle->cullbits &                                             \
       (d)->triangle->neighbors[2]->cullbits & OUT) :                        \
     !((d)->triangle->cullbits & OUT))

#define is_splittable(d) ((d) && !is_queued(d) && !is_fine(d) && is_visible(d))
#define is_mergeable(d) (d && !is_queued(d) && !is_coarse(d) &&               \
                         is_leaf((d)->triangle->children[0]) &&               \
                         is_leaf((d)->triangle->children[1]) &&               \
                         is_leaf((d)->triangle->neighbors[2]->children[0]) && \
                         is_leaf((d)->triangle->neighbors[2]->children[1]))

typedef struct {
    unsigned short **samples;
    unsigned short **bounds;
    double *scales, *offsets;
    unsigned int *imagery;
    int *orders;

    int size[2], depth;
    double resolution[2];
} roam_Tileset;

typedef struct {
    roam_Tileset tileset;

    void *pools[2];
    
    struct diamond *queues[2][QUEUE_SIZE];
    struct triangle *(*roots)[2];

    float *buffer;
    
    int chunks[2], blocks[2], queued[2];
    int culled, visible, drawn;
    int minimum, maximum;

    int target, frame;
} roam_Context;

struct triangle {
    struct diamond *diamond;
    struct triangle *neighbors[3], *children[2], *parent;
    unsigned char cullbits, flags;
    unsigned short index;
};

struct diamond {
    struct diamond *queue, *left, *right;
    struct triangle *triangle;

    float vertices[2][3], center[3];

    float error;
    unsigned short priority;
    char level, flags;
};

typedef struct triangle roam_Triangle;
typedef struct diamond roam_Diamond;

void look_up_sample(roam_Tileset *tiles, int i, int j, double *h, double *e);
void optimize_geometry(roam_Context *new, int frame);
void draw_geometry(roam_Context *new, float *buffer, int *ranges);
void seed_vegetation(roam_Context *new, double density, double bias,
                     unsigned int _ls, unsigned int _lo);
void free_mesh(roam_Context *new);
void *allocate_mesh(roam_Context *new);
void calculate_tile_bounds(unsigned short *heights, unsigned short *bounds,
                           int size);

#endif
