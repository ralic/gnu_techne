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
#include "input.h"
#include "graphics.h"
#include "shader.h"

static int interactive = 0;   /* Accept input on the tty. */
static int iterate = 0;       /* Keep the main loop running. */
static int iterations = 0;

static Techne *instance;

int t_get_iterations ()
{
    return iterations;
}

@implementation Techne
-(void) init
{
    [super init];

    lua_pushstring (_L, "techne");
    lua_setfield (_L, -2, "tag");

    /* Greet the user. */

    t_print_message ("This is Techne, version %s.\n", VERSION);

    instance = self;
}

+(Builtin *)instance
{
    return instance;
}

-(void) iterate
{
    Builtin *builtin;
    long long unsigned int t;

    t_initialize_timing ();

    while (iterate || interactive) {
        t_begin_cpu_interval (&self->latency);
        t_begin_cpu_interval (&self->core);

        if (interactive) {
            t_print_message ("Dropping to a shell.  "
                             "Press ctrl-d to continue.\n");

            luap_enter(_L);
            interactive = 0;
        }

        t_end_cpu_interval (&self->core);

        for (builtin = [Builtin nodes];
             builtin;
             builtin = (Builtin *)builtin->right) {
            if (builtin != self) {
                [builtin iterate];
            }
        }

        t_end_cpu_interval (&self->latency);
        t_advance_profiling_frame();

        iterations += 1;
    }

    t = t_get_cpu_time();
    t_print_message("Ran a total of %d iterations in %.1f seconds at "
                    "%.1f ms per iteration (%.1f Hz).\n",
                    iterations, t * 1e-9, t * 1e-6 / iterations,
                    iterations / (t * 1e-9));
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
    lua_pushnumber (_L, t_get_cpu_time() * 1e-9);

    return 1;
}

-(int) _get_iterations
{
    lua_pushnumber (_L, iterations);

    return 1;
}

-(int) _get_profiling
{
    lua_pushboolean (_L, _PROFILING);

    return 1;
}

-(int) _get_latency
{
    if (self->latency.frames > 0) {
        lua_createtable(_L, 2, 0);
        lua_pushnumber(_L, self->latency.total[1] * 1e-9 / self->latency.frames);
        lua_rawseti(_L, -2, 1);
        lua_pushinteger(_L, self->latency.frames);
        lua_rawseti(_L, -2, 2);
    } else {
        lua_pushnil(_L);
    }

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

-(void) _set_profiling
{
    T_WARN_READONLY;
}

-(void) _set_latency
{
    T_WARN_READONLY;
}

@end

int luaopen_techne (lua_State *L)
{
    [[Techne alloc] init];

    return 1;
}
