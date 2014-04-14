/* Copyright (C) 2014 Papavasileiou Dimitris                             
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

#include <lua.h>
#include <lauxlib.h>

#include "gl.h"
#include "structures.h"
#include "memory.h"
#include "techne.h"
#include "profiling.h"
#include "graphic.h"

static long long unsigned int luatime;
static int frame = 1, block = 0;

static void *pool;

static void profilinghook (lua_State *L, lua_Debug *ar)
{
    static long long unsigned int t_0;
    
    /* We're only insterested when the outermost function starts and
     * finishes so check that the current function is the only one on
     * the stack. */
    
    if (lua_getstack (L, 1, ar) == 0) {
	if (ar->event == LUA_HOOKCALL) {
	    t_0 = t_get_cpu_time();
	} else if (ar->event == LUA_HOOKRET) {
            static int first = 1;

            if (first) {
                first = 0;
            } else {
                luatime += t_get_cpu_time() - t_0;
            }
	}
    }
}

void t_advance_profiling_frame ()
{
    frame += 1;
}

void t_begin_cpu_interval (t_CPUProfile *profile)
{
    if (_PROFILING) {
        assert (profile->frame <= 0);

        /* The absolute value of the frame is the current frame
         * number, while the sign signifies whether we're currently
         * measuring an interval (positive) or not (negative). */
        
        profile->frames += (profile->frame != -frame);
        profile->frame = frame;
        
        profile->total[0] = t_get_cpu_time();
        profile->lua[0] = luatime;
    }
}

void t_end_cpu_interval (t_CPUProfile *profile)
{
    if (_PROFILING) {
        assert(profile->frame == frame);

        /* Accumulate the interval. */
        
        profile->total[1] += (t_get_cpu_time() -
                                profile->total[0]);
        profile->lua[1] += (luatime -
                              profile->lua[0]);
        
        /* Mark the interval as closed. */
        
        profile->frame *= -1;
    }
}

static struct queryset *allocate_queryset()
{
    struct queryset *set;

    set = t_allocate_pooled(pool);
    glGenQueries(2, set->queries);

    return set;
}

void t_free_profiling_queries(t_GPUProfile *profile)
{
    struct queryset *next;
    
    while((next = profile->sets->right)) {
        t_circular_unlink(next);
        glDeleteQueries(2, next->queries);
        t_free_pooled(pool, next);
    }

    glDeleteQueries(2, profile->sets->queries);
    t_free_pooled(pool, profile->sets);
}

static void accumulate_gpu_interval (t_GPUProfile *profile)
{
    GLuint64 t_0, t_1;
            
    glGetQueryObjectui64v(profile->sets->queries[0],
                          GL_QUERY_RESULT, &t_0);
    glGetQueryObjectui64v(profile->sets->queries[1],
                          GL_QUERY_RESULT, &t_1);

    profile->interval += t_1 - t_0;
    profile->frames += 1;
}

void t_begin_gpu_interval (t_GPUProfile *profile)
{
    if (_PROFILING) {
        if (!profile->sets) {
            struct queryset *set;

            set = allocate_queryset();
            profile->sets = t_make_circular(set);
        } else if (!block) {
            int available;

            /* Check whether any of the previous queries are ready. */
                
            glGetQueryObjectiv(profile->sets->queries[1],
                               GL_QUERY_RESULT_AVAILABLE,
                               &available);

            if (!available) {
                struct queryset *next;
                
                next = allocate_queryset();
                t_circular_link_before(next, profile->sets);
            } else {
                /* Query the timers and accumulate the (previous)
                 * interval. */

                accumulate_gpu_interval(profile);
                profile->sets = profile->sets->right;
            }
        }

        /* Start measuring a new interval. */
        
        glQueryCounter(profile->sets->left->queries[0], GL_TIMESTAMP);
    }
}

void t_end_gpu_interval (t_GPUProfile *profile)
{
    if (_PROFILING) {
        glQueryCounter(profile->sets->left->queries[1], GL_TIMESTAMP);

        if (block) {
            accumulate_gpu_interval(profile);
        }
    }
}

void t_enable_profiling ()
{
    /* Get the configuration. */
    
    lua_getglobal (_L, "options");

    /* The window manager class. */
    
    lua_getfield (_L, -1, "profileblock");
    block = lua_toboolean (_L, -1);
    lua_pop (_L, 1);

    lua_pop (_L, 1);
    
    /* Set the profiling hook. */
    
    lua_sethook (_L, profilinghook, LUA_MASKCALL | LUA_MASKRET, 0);

    /* Create the pool. */

    pool = t_build_pool(64, sizeof(struct queryset), T_FREEABLE);
    
    _PROFILING = 1;
}
