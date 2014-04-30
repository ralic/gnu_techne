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
#include <structures.h>

#include "techne.h"

struct chunk {
    struct chunk *next;
    void *blocks;
};

struct pool {
    t_PoolMode mode;
    
    struct chunk *chunks, *current;
    int factor, size, chunks_n, blocks_n;
};

struct block {
    struct block *next;
};

static void reset_freeable_chunk (struct pool *pool, struct chunk *chunk)
{
    struct block *block;
    int i;

    /* Restring all the nodes in the chunk so that each points to the
     * next. */
    
    for (i = 0, chunk->blocks = NULL ; i < pool->factor ; i += 1) {
        block = (struct block *)(((char *)chunk) + sizeof (struct chunk) + i * pool->size);
        block->next = chunk->blocks;
        chunk->blocks = block;
    }
}

static struct chunk *add_new_chunk(struct pool *pool)
{
    struct chunk *new;

    /* Allocate the new chunk... */
    
    new = malloc (sizeof (struct chunk) + pool->size * pool->factor);
    new->next = NULL;

    /* ...reset it... */
    
    if (pool->mode == T_FLUSH_ONLY) {
        new->blocks = (void *)new + sizeof (struct chunk);
    } else {
        reset_freeable_chunk(pool, new);
    }

    /* ...and add it to the pool. */
    
    if (!pool->chunks) {
	pool->chunks = new;
    } else {
        if (pool->mode == T_FLUSH_ONLY) {
            assert (!pool->current->next);
            t_single_link_after(new, pool->current);
        } else {
            t_single_link_at_head (new, &pool->chunks);
        }
    }
    
    pool->chunks_n += 1;    

    return new;
}

void *t_build_pool(int factor, size_t size, t_PoolMode mode)
{
    struct pool *pool;

    pool = malloc (sizeof(struct pool));

    pool->mode = mode;
    pool->factor = factor;
    pool->size = size;
    pool->chunks = NULL;
    pool->current = NULL;
    pool->blocks_n = 0;
    pool->chunks_n = 0;

    return (void *)pool;
}

void *t_allocate_pooled (void *p)
{
    struct pool *pool;
    void *block;
    int i;

    pool = (struct pool *)p;

    if (pool->mode == T_FLUSH_ONLY) {
        if (!pool->chunks) {
            /* This is the first allocation. */
	
            pool->current = add_new_chunk (pool);
        }

        i = pool->blocks_n % pool->factor;
        block = pool->current->blocks + i * pool->size;
    
        if ((i + 1) == pool->factor) {
            /* Allocate a new chunk if we've run out. */
	    
            if (!pool->current->next) {
                pool->current = add_new_chunk (pool);
            } else {
                pool->current = pool->current->next;
            }
        }
    } else {
        struct chunk *chunk;

        /* Look for a chunk with free blocks. */
        
        for (chunk = pool->chunks;
             chunk && !chunk->blocks;
             chunk = chunk->next);

        /* If we're out, add a new chunk to the pool. */
        if (!chunk) {
            chunk = add_new_chunk (pool);
        }

        /* Remove the first block from the chunk and return. */

        block = chunk->blocks;
        t_single_unlink_head((struct block **)&chunk->blocks);
    }
    
    pool->blocks_n += 1;

    return block;
}

void t_free_pooled (void *p, void *block)
{
    struct pool *pool;

    pool = (struct pool *)p;

    assert(pool->mode == T_FREEABLE);
    assert(pool->chunks);

    /* Just stash the block back into the first available chunk so
     * that it can be picked up again. */
    
    t_single_link_at_head ((struct block *)block, &pool->chunks->blocks);
    pool->blocks_n -= 1;
}

void t_reset_pool (void *p)
{
    struct pool *pool;

    pool = (struct pool *)p;
    
    if (pool->mode == T_FLUSH_ONLY) {
        pool->current = pool->chunks;
    } else {
        struct chunk *chunk;
        
        for (chunk = pool->chunks ; chunk ; chunk = chunk->next) {
            reset_freeable_chunk(pool, chunk);
        }
    }

    pool->blocks_n = 0;
}

void t_flush_pool (void *p)
{
    struct pool *pool;
    struct chunk *chunk, *next;

    pool = (struct pool *)p;
    
    /* Free all allocated chunks. */
    
    for (chunk = pool->chunks ; chunk ; chunk = next) {
        next = chunk->next;	
        free(chunk);
    }
    
    pool->chunks = NULL;
    pool->current = NULL;
    pool->chunks_n = 0;
    pool->blocks_n = 0;
}

void t_free_pool (void *p)
{
    /* Free all allocated chunks as well as the pool itself. */

    t_flush_pool(p);
    free(p);
}
