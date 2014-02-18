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

#define FLIPPED          (1 << 0)
#define SPLIT_QUEUED     (1 << 1)
#define MERGE_QUEUED     (1 << 2)

#define is_locked(d) (((d)->level >=                                    \
                       (context->tileset.orders[(d)->triangle->tile] << 1)))

#define is_leaf(n) (!(n)->children[0])
#define is_out(n) ((n)->cullbits & OUT)
#define is_primary(n) (n->diamond->flags & FLIPPED ?        \
                       (n)->diamond->triangle != (n) :      \
                       (n)->diamond->triangle == (n))

#define is_pair(n) ((n)->neighbors[2] &&                    \
                    (n)->neighbors[2]->neighbors[2] == (n))

#define is_fine(d) ((d)->level >= TREE_HEIGHT - 1 || (d)->error == 0.0)
#define is_coarse(d) ((d)->level <= 0 || isinf((d)->error))
#define is_queued(d) ((d)->flags & (SPLIT_QUEUED | MERGE_QUEUED))
#define is_queued_into(d, i) ((d->flags) & (SPLIT_QUEUED << i))
#define is_visible(d)                                                   \
    (is_pair((d)->triangle) ?                                           \
     !((d)->triangle->cullbits &                                        \
       (d)->triangle->neighbors[2]->cullbits & OUT) :                   \
     !((d)->triangle->cullbits & OUT))
#define is_allin(d)                                                     \
    (is_pair((d)->triangle) ?                                           \
     (((d)->triangle->cullbits &                                        \
       (d)->triangle->neighbors[2]->cullbits & ALL_IN) == ALL_IN) :     \
     (((d)->triangle->cullbits & ALL_IN)) == ALL_IN)

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
    double resolution[2], offset[2];
} roam_Tileset;

typedef struct {
    roam_Tileset tileset;

    void *pools[2];
    
    struct diamond *queues[2][QUEUE_SIZE];
    struct triangle *(*roots)[2];

    long long int intervals[4];

    double canopy, viewport[4], transform[16], planes[6][4];

    int queued[2];
    int triangles, diamonds, culled, visible;
    int minimum, maximum;

    int target, frame;
} roam_Context;

struct triangle {
    struct diamond *diamond;
    struct triangle *neighbors[3], *children[2], *parent;
    unsigned char cullbits, flags;
    unsigned short tile;
};

struct diamond {
    struct diamond *left, *right;
    struct triangle *triangle;

    float vertices[2][3], center[3];

    float error;
    unsigned short priority;
    char level, flags;
};

typedef struct triangle roam_Triangle;
typedef struct diamond roam_Diamond;

void look_up_sample(roam_Tileset *tiles, int i, int j, double *h, double *e);
void optimize_geometry(roam_Context *context_in, int frame);
void free_mesh(roam_Context *context_in);
void *allocate_mesh(roam_Context *context_in);
void calculate_tile_bounds(unsigned short *heights, unsigned short *bounds,
                           int size);

#endif
