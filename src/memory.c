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

struct chunk {
    struct chunk *next;
    void *blocks;
};

struct pool {
    struct chunk *chunks, *current;
    int factor, size, allocated[2];
};

static void add_new_chunk(struct pool *pool)
{
    struct chunk *new;
    
    new = malloc (sizeof (struct chunk) + pool->size * pool->factor);
    new->blocks = (void *)new + sizeof (struct chunk);
    new->next = NULL;
    
    if (!pool->chunks) {
	pool->chunks = new;
    }

    if (pool->current) {
	pool->current->next = new;
    }
    
    pool->current = new;
    pool->allocated[1] += 1;    
}

void *t_build_pool(int factor, size_t size)
{
    struct pool *pool;

    pool = malloc (sizeof(struct pool));

    pool->factor = factor;
    pool->size = size;
    pool->chunks = NULL;
    pool->current = NULL;
    pool->allocated[0] = 0;
    pool->allocated[1] = 0;

    return (void *)pool;
}

void *t_allocate_from_pool (void *p)
{
    struct pool *pool;
    void *block;
    int i;

    pool = (struct pool *)p;
    
    if (!pool->chunks) {
	/* This is the first allocation. */
	
	add_new_chunk (pool);
    }

    i = pool->allocated[0] % pool->factor;
    block = pool->current->blocks + i * pool->size;
    
    if ((i + 1) == pool->factor) {
	/* Allocate a new chunk if we've run out. */
	    
	if (!pool->current->next) {
	    add_new_chunk (pool);
	}
    }

    pool->allocated[0] += 1;

    return block;
}

void t_reset_pool (void *p)
{
    struct pool *pool;

    pool = (struct pool *)p;
    pool->allocated[0] = 0;
    pool->current = pool->chunks;
}
