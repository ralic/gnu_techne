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
#include <string.h>
#include <time.h>
#include <sys/time.h>

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include "techne.h"

#define CHUNK_SIZE 128
#define READINGS 8

struct reading {
    double core, user;
};

static struct chunk {
    struct reading pool[CHUNK_SIZE][READINGS];
    struct chunk *next;
} *chunks, *current;

static int allocated[2];
static struct reading *frame, *row;

static long int once;

static void profilinghook (lua_State *L, lua_Debug *ar)
{
    static long int now, then;

    assert (frame);
    
    if (lua_getstack (L, 1, ar) == 0) {
	if (ar->event == LUA_HOOKCALL) {
	    then = t_get_cpu_time();
	} else if (ar->event == LUA_HOOKRET) {
	    now = t_get_cpu_time();
	    frame->user += (now - then) / 1e6;
	}
    }
}

void tprof_new_row ()
{
    int i;
    
    if (!chunks) {
	/* This is the first allocation. */

	current = chunks = malloc (sizeof (struct chunk));
	current->next = NULL;

	allocated[1] = 1;
    }

    i = allocated[0] % CHUNK_SIZE;
    row = memset(current->pool[i], 0, sizeof (struct reading[READINGS]));
    
    if ((i + 1) == CHUNK_SIZE) {
	/* Allocate a new chunk if we've run out. */
	    
	if (!current->next) {
	    current->next = malloc (sizeof (struct chunk));
	    current->next->next = NULL;

	    allocated[1] += 1;
	}
	    
	current = current->next;
    }

    allocated[0] += 1;
}

void tprof_begin (tprof_Frame reading)
{
    assert (reading < READINGS);
    frame = &row[reading];

    once = t_get_cpu_time();
    lua_sethook (_L, profilinghook, LUA_MASKCALL | LUA_MASKRET, 0);
}

void tprof_end (tprof_Frame reading)
{
    long int now;
    
    assert (reading < READINGS);
    assert (row + reading == frame);

    now = t_get_cpu_time();

    frame->core += (now - once) / 1e6;
}

void tprof_report ()
{
    FILE *log;
    struct chunk *chunk;
    int i, j, n;

    log = fopen ("/tmp/foo.dat", "w");

    if (!log) {
	t_print_warning ("Could not open profling log for writing.\n");
	return;
    }

    for (chunk = chunks, n = 0 ; chunk ; chunk = chunk->next) {
	for (i = 0;
	     i < CHUNK_SIZE && n < allocated[0];
	     i += 1, n += 1) {
	    for (j = 0 ; j < READINGS ; j += 1) {
		fprintf (log, "%.3f, %.3f%c",
			 chunk->pool[i][j].core,
			 chunk->pool[i][j].user,
			 j < READINGS - 1 ? '\t' : '\n');
	    }
	}
    }
}
