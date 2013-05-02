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

#include <config.h>
#include <stdio.h>

#include <lualib.h>
#include <lauxlib.h>

#include "prompt/prompt.h"
#include "techne.h"
#include "root.h"
#include "dynamics.h"
#include "network.h"
#include "input.h"
#include "graphics.h"
#include "accoustics.h"
#include "shader.h"

static int interactive = 0;   /* Accept input on the tty. */
static int iterate = 0;       /* Keep the main loop running. */
static int iterations = 0;

static long long int zero;
static long long int *intervals;
static long long int *beginnings;

static Techne *instance;

static void profilinghook (lua_State *L, lua_Debug *ar)
{
    if (lua_getstack (L, 1, ar) == 0) {
	if (ar->event == LUA_HOOKCALL) {
	    beginnings[1] = t_get_cpu_time();
	} else if (ar->event == LUA_HOOKRET) {
	    intervals[1] += t_get_cpu_time() - beginnings[1];
	}
    }
}

void t_begin_interval (Node *node)
{
    long long int t;
    
    t = t_get_cpu_time();

    if (!node->up) {
	lua_sethook (_L, profilinghook, LUA_MASKCALL | LUA_MASKRET, 0);
    } else {
	assert (node->up->profile.beginnings == beginnings);
	assert (node->up->profile.intervals == intervals);
	intervals[0] += t - beginnings[0];
    }

    intervals = node->profile.intervals;
    beginnings = node->profile.beginnings;
    beginnings[0] = t;
}

void t_end_interval (Node *node)
{
    long long int t;
    
    assert (intervals == node->profile.intervals);
    assert (beginnings == node->profile.beginnings);

    t = t_get_cpu_time();
    intervals[0] += t - beginnings[0];

    if (node->up) {
	intervals = node->up->profile.intervals;
	beginnings = node->up->profile.beginnings;
	beginnings[0] = t;
    } else {
	lua_sethook (_L, 0, 0, 0);
    }
}

@implementation Techne
-(void) init
{
    [super init];
    
    lua_pushstring (_L, "techne");
    lua_setfield (_L, -2, "tag");

    /* Greet the user. */

    t_print_message ("This is Techne, version %s.\n", VERSION);
    t_print_timing_resolution();

    instance = self;
}

+(Builtin *)instance
{
    return instance;
}

-(void) iterate
{
    Builtin *builtin;

    zero = t_get_cpu_time();
    
    t_begin_interval(self);
    
    while (iterate || interactive) {
	if (interactive) {
	    t_print_message ("Dropping to a shell.  "
			     "Press ctrl-d to continue.\n");

	    luap_enter(_L);
	    interactive = 0;
	}

        t_end_interval(self);
                
	for (builtin = [Builtin nodes];
	     builtin;
	     builtin = (Builtin *)builtin->right) {
	    if (builtin != self) {
		[builtin iterate];
	    }
	}

        t_begin_interval(self);
	
        iterations += 1;
    }

    t_end_interval(self);
}

-(int) _get_interactive
{
    lua_pushboolean (_L, interactive);
    
    return 1;
}

-(int) _get_iterate
{
    lua_pushboolean (_L, iterate);
    
    return 1;
}
 
-(int) _get_time
{
    lua_pushnumber (_L, (t_get_cpu_time() - zero) / 1e9);
    
    return 1;
}
	
-(int) _get_iterations
{
    lua_pushnumber (_L, iterations);

    return 1;
}

-(void) _set_time
{
    T_WARN_READONLY;
}

-(void) _set_iterate
{
    iterate = lua_toboolean (_L, -1);
}

-(void) _set_interactive
{
    interactive = lua_toboolean (_L, -1);
}
	
-(void) _set_iterations
{
    T_WARN_READONLY;
}

@end

int luaopen_techne (lua_State *L)
{
    [[Techne alloc] init];

    return 1;
}
