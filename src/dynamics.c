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
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include "array/array.h"
#include "dynamics.h"
#include "techne.h"
#include "root.h"

static double timescale = 1, stepsize = 0.01, ceiling = 1.0 / 0.0;
static double interval = -1;
static int iterations = 0, collision = LUA_REFNIL;

static long int once;
static double now, then;

static Dynamics* instance;

long long unsigned int t_get_dynamics_time ()
{
    return (long long unsigned int)(now * 1e9);
}

@implementation Dynamics
-(void) init
{
    self->index = 3;

    [super init];

    lua_pushstring (_L, "dynamics");
    lua_setfield (_L, -2, "tag");

    /* Get the current real time. */

    once = t_get_real_time();
    instance = self;
}

+(Builtin *)instance
{
    return instance;
}

-(void) iterate
{
    Root *root;
    double delta;
    long int time;

    t_begin_cpu_interval (&self->core);

    time = t_get_real_time();

    if (interval < 0) {
        delta = timescale * ((double)(time - once) / 1e9);
    } else {
        delta = timescale * interval;
    }

    /* Calculate the time to advance for this iteration. */

    if (delta > ceiling) {
        /* printf ("Skewing the simulation clock by %.3fs\n", */
        /*          ceiling - delta); */
        now += ceiling;
    } else {
        now += delta;
    }

    t_end_cpu_interval (&self->core);

    /* Simulate the system forward. */

    for (delta = now - then;
         delta >= stepsize;
         then += stepsize, delta -= stepsize) {
        /* Step the tree. */

        for (root = [Root nodes] ; root ; root = (Root *)root->right) {
            t_step_subtree (root, stepsize, then);
        }
    }

    t_begin_cpu_interval (&self->core);

    /* Advance the real-world time. */

    once = time;
    t_end_cpu_interval (&self->core);

    /* Transform the tree to update absolute positions and
       orientations. */

    for (root = [Root nodes] ; root ; root = (Root *)root->right) {
        t_transform_subtree (root);
    }
}

-(int) _get_stepsize
{
    lua_pushnumber(_L, stepsize);

    return 1;
}

-(int) _get_time
{
    lua_pushnumber (_L, now);

    return 1;
}

-(int) _get_ceiling
{
    lua_pushnumber(_L, ceiling);

    return 1;
}

-(int) _get_timescale
{
    lua_pushnumber(_L, timescale);

    return 1;
}

-(int) _get_iterations
{
    lua_pushnumber(_L, iterations);

    return 1;
}

-(int) _get_collision
{
    lua_rawgeti(_L, LUA_REGISTRYINDEX, collision);

    return 1;
}

-(int) _get_tolerance
{
    lua_newtable (_L);
    lua_pushnumber (_L, 0);
    lua_rawseti (_L, -2, 1);
    lua_pushnumber (_L, 0);
    lua_rawseti (_L, -2, 2);

    return 1;
}

-(int) _get_popvelocity
{
    lua_pushnumber(_L, 0);

    return 1;
}

-(int) _get_surfacelayer
{
    lua_pushnumber(_L, 0);

    return 1;
}

-(int) _get_gravity
{
    double g[3];

    array_createarray (_L, ARRAY_TDOUBLE, g, 1, 3);

    return 1;
}

-(int) _get_interval
{
    if (interval < 0) {
        lua_pushnil (_L);
    } else {
        lua_pushnumber (_L, interval);
    }

    return 1;
}

-(void) _set_interval
{
    if (lua_isnumber (_L, 3)) {
        interval = lua_tonumber (_L, 3);
    } else {
        interval = -1;
    }
}

-(void) _set_time
{
    T_WARN_READONLY;
}

-(void) _set_stepsize
{
    stepsize = lua_tonumber(_L, 3);
}

-(void) _set_ceiling
{
    ceiling = lua_tonumber(_L, 3);
}

-(void) _set_timescale
{
    timescale = lua_tonumber(_L, 3);
}

-(void) _set_iterations
{
    iterations = lua_tonumber(_L, 3);
}

-(void) _set_collision
{
    luaL_unref(_L, LUA_REGISTRYINDEX, collision);
    collision = luaL_ref(_L, LUA_REGISTRYINDEX);
}

-(void) _set_tolerance
{
}

-(void) _set_popvelocity
{
}

-(void) _set_surfacelayer
{
}

-(void) _set_gravity
{
}

@end

int luaopen_dynamics (lua_State *L)
{
    [[Dynamics alloc] init];

    return 1;
}
