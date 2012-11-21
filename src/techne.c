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

static long long int beginning;

static void accumulate(Node *root, long long int (*intervals)[2]) {
    Node *child;
    int j;
    
    for (j = 0 ; j < T_PHASE_COUNT ; j += 1) {
	intervals[j][0] += root->profile.intervals[j][0];
	intervals[j][1] += root->profile.intervals[j][1];
    }
    
    for (child = root->down ; child ; child = child->right) {
	accumulate (child, intervals);
    }
}

@implementation Techne
-(void) initWithArgc: (int)argc andArgv: (char **)argv
{
    [super init];
    lua_pushstring (_L, "techne");
    lua_setfield (_L, -2, "tag");
    lua_setglobal (_L, "techne");

    /* Greet the user. */

    t_print_message ("This is Techne, version %s.\n", VERSION);
    t_print_timing_resolution();
    
    [[Input alloc] init];
    lua_setglobal (_L, "input");
    
    [[Network alloc] init];
    lua_setglobal (_L, "network");
    
    [[Dynamics alloc] init];
    lua_setglobal (_L, "dynamics");

    [[Graphics alloc] init];
    lua_setglobal (_L, "graphics");

    [[Accoustics alloc] init];
    lua_setglobal (_L, "accoustics");
}

-(void) iterate
{
    Builtin *builtin;
    Root *root;
    long long int runtime, totals[4], intervals[T_PHASE_COUNT][2];
    char *phases[T_PHASE_COUNT] = {"begin", "input", "step",
				   "transform", "prepare", "traverse",
				   "finish"};
    double c;
    int h, j;

    beginning = t_get_cpu_time();
    
    while (iterate || interactive) {
	if (interactive) {
	    t_print_message ("Dropping to a shell.  "
			     "Press ctrl-d to continue.\n");

	    luap_enter(_L);
	    interactive = 0;
	}

	for (root = [Root nodes] ; root ; root = (Root *)root->right) {
	    t_begin_interval (root, T_BEGIN_PHASE);    
	    [root begin];
	    t_end_interval (root, T_BEGIN_PHASE);
	}

	for (builtin = [Builtin nodes];
	     builtin;
	     builtin = (Builtin *)builtin->right) {
	    if (builtin != self) {
		[builtin iterate];
	    }
	}
	
        iterations += 1;
    }

    runtime = t_get_cpu_time() - beginning;
    
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

    while (lua_gettop(_L) > h) {
	Node *node;
	
        memset (intervals, 0, sizeof(intervals));
        memset (totals, 0, sizeof(totals));

        assert (t_isnode(_L, -1));
        node = t_tonode(_L, -1);
        
        accumulate (node, intervals);
	
        for (j = 0 ; j < T_PHASE_COUNT ; j += 1) {
            totals[0] += intervals[j][0];
            totals[1] += intervals[j][1];
            totals[2] += node->profile.intervals[j][0];
            totals[3] += node->profile.intervals[j][1];
        }

        luaL_callmeta (_L, -1, "__tostring");
        
        t_print_message(
            "\nPhase profile for %s:\n"
            "+----------------------------------+------------------------+\n"
            "|             cummulative          |   self                 |\n"
            "|phase        core    user   total |   core    user   total |\n"
            "+----------------------------------+------------------------+\n",
            lua_tostring (_L, -1));
	
        for (j = 0 ; j < T_PHASE_COUNT ; j += 1) {
            t_print_message(
                "|%-10s %5.1f%%  %5.1f%%  %5.1f%% | %5.1f%%  %5.1f%%  %5.1f%% |\n",
                phases[j],
                c * (intervals[j][0] - intervals[j][1]),
                c * intervals[j][1], c * intervals[j][0],
                c * (node->profile.intervals[j][0] -
                     node->profile.intervals[j][1]),
                c * node->profile.intervals[j][1],
                c * node->profile.intervals[j][0]);
        }

        t_print_message(
            "+----------------------------------+------------------------+\n"
            "|%-10s %5.1f%%  %5.1f%%  %5.1f%% | %5.1f%%  %5.1f%%  %5.1f%% |\n"
            "+----------------------------------+------------------------+\n",
            "total",
            c * (totals[0] - totals[1]), c * totals[1], c * totals[0],
            c * (totals[2] - totals[3]), c * totals[3], c * totals[2]);

        lua_pop (_L, 2);
    }
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
    lua_pushnumber (_L, (t_get_cpu_time() - beginning) / 1e9);
    
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
