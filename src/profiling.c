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
#include "node.h"

static long long int *interval;
static long long int *beginning;

static void profilinghook (lua_State *L, lua_Debug *ar)
{
    if (lua_getstack (L, 1, ar) == 0) {
	if (ar->event == LUA_HOOKCALL) {
	    beginning[1] = t_get_cpu_time();
	} else if (ar->event == LUA_HOOKRET) {
	    interval[1] += t_get_cpu_time() - beginning[1];
	}
    }
}

void t_begin_interval (Node *node, tprof_Phase i)
{
    long long int t;
    
    assert (i < T_PHASE_COUNT);
    
    t = t_get_cpu_time();

    if (!node->up) {
	lua_sethook (_L, profilinghook, LUA_MASKCALL | LUA_MASKRET, 0);
    } else {
	assert (node->up->profile.beginning == beginning);
	assert (node->up->profile.intervals[i] == interval);
	interval[0] += t - beginning[0];
    }

    interval = node->profile.intervals[i];
    beginning = node->profile.beginning;
    beginning[0] = t;
}

void t_end_interval (Node *node, tprof_Phase i)
{
    long long int t;
    
    assert (i < T_PHASE_COUNT);
    assert (interval == node->profile.intervals[i]);
    assert (beginning == node->profile.beginning);

    t = t_get_cpu_time();
    interval[0] += t - beginning[0];

    if (node->up) {
	interval = node->up->profile.intervals[i];
	beginning = node->up->profile.beginning;
	beginning[0] = t;
    } else {
	lua_sethook (_L, 0, 0, 0);
    }
}
