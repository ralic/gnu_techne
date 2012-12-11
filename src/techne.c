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

static void accumulate(Node *root, long long int intervals[2]) {
    Node *child;
    
    intervals[0] += root->profile.intervals[0];
    intervals[1] += root->profile.intervals[1];
    
    for (child = root->down ; child ; child = child->right) {
	accumulate (child, intervals);
    }
}

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
    Root *root;
    long long int runtime, totals[4], intervals[2];
    double c;
    int h, j;

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

    runtime = t_get_cpu_time() - zero;
    
    /* Output statistics for profiled nodes. */
    
    t_print_message("Ran a total of %d iterations in %.1f seconds at "
                    "%.1f ms per iteration.\n",
		    iterations, runtime * 1e-9, runtime / (iterations * 1e6));
    
    c = 100.0 / runtime;
    h = lua_gettop(_L);

    /* Gather all profiled nodes. */
    
    lua_getglobal (_L, "options");
    lua_getfield (_L, -1, "profile");
    lua_replace (_L, -2);

    if (lua_type(_L, -1) == LUA_TSTRING) {
        lua_newtable(_L);
        lua_insert(_L, -2);
        lua_rawseti(_L, -2, 1);
    }

    if (lua_type(_L, h + 1) == LUA_TTABLE) {
        int n;

        n = lua_rawlen(_L, h + 1);

        for (j = 0 ; j < n ; j += 1) {
            lua_pushstring (_L, "return ");
            lua_rawgeti(_L, h + 1, j + 1);
            lua_concat(_L, 2);

            if (luaL_loadstring(_L, lua_tostring(_L, -1)) ||
                lua_pcall(_L, 0, 1, 0)) {
                lua_rawgeti(_L, h + 1, j + 1);
                
                t_print_warning ("Could not resolve node at '%s'\n",
                                 lua_tostring(_L, -1));
                lua_pop(_L, 3);
            } else if (!t_isnode(_L, -1)) {
                lua_rawgeti(_L, h + 1, j + 1);
                
                t_print_warning ("Value at '%s' is not a node (it is a %s value).\n",
                                 lua_tostring(_L, -1),
                                 lua_typename(_L, lua_type(_L, -2)));
                lua_pop(_L, 3);
            } else {
                lua_replace(_L, -2);
            }
        }
    }

    lua_remove(_L, h + 1);

    /* If no profiled nodes have been specified just rerturn the
     * profile of all builtins and roots. */
    
    if (lua_gettop(_L) == h) {
	for (builtin = [Builtin nodes];
	     builtin;
	     builtin = (Builtin *)builtin->right) {
            t_pushuserdata(_L, 1, builtin);
        }
        
	for (root = [Root nodes];
	     root;
	     root = (Root *)root->right) {
            t_pushuserdata(_L, 1, root);
        }
    }

    memset (totals, 0, sizeof(totals));

    t_print_message(
	"+------------------------------------------------------+------------------------+\n"
	"|                                 cummulative          |   self                 |\n"
	"|node                             core    user   total |   core    user   total |\n"
	"+------------------------------------------------------+------------------------+\n"
	);
    
    for (j = h + 1 ; j <= lua_gettop(_L) ; j += 1) {
	Node *node, *other, *ancestor;
        int i;
	
        memset (intervals, 0, sizeof(intervals));

        assert (t_isnode(_L, j));
        node = t_tonode(_L, j);
        
        accumulate (node, intervals);

        for (i = h + 1 ; i <= lua_gettop(_L) ; i += 1) {
            if (i == j) {
                continue;
            }
            
            other = t_tonode(_L, i);

            for (ancestor = node ; ancestor ; ancestor = ancestor->up) {
                if (ancestor == other) {
                    goto skip;
                }
            }
        }
        
	totals[0] += intervals[0];
	totals[1] += intervals[1];
	totals[2] += node->profile.intervals[0];
	totals[3] += node->profile.intervals[1];
        
    skip:
        
        luaL_callmeta (_L, j, "__tostring");
        
	t_print_message(
	    "|%-30s %5.1f%%  %5.1f%%  %5.1f%% | %5.1f%%  %5.1f%%  %5.1f%% |\n",
	    lua_tostring (_L, -1),
	    c * (intervals[0] - intervals[1]),
	    c * intervals[1], c * intervals[0],
	    c * (node->profile.intervals[0] -
		 node->profile.intervals[1]),
	    c * node->profile.intervals[1],
	    c * node->profile.intervals[0]);

        lua_pop (_L, 1);
    }

    t_print_message(
	"+------------------------------------------------------+------------------------+\n"
	"|total                          %5.1f%%  %5.1f%%  %5.1f%% | %5.1f%%  %5.1f%%  %5.1f%% |\n"
	"+------------------------------------------------------+------------------------+\n",
	c * (totals[0] - totals[1]), c * totals[1], c * totals[0],
	c * (totals[2] - totals[3]), c * totals[3], c * totals[2]);

    lua_settop(_L, h);
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
