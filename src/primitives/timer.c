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

#include <math.h>
#include <string.h>

#include <lua.h>
#include <lauxlib.h>

#include "techne.h"
#include "timer.h"

#define TECHNE 1
#define DYNAMICS 2
#define ITERATIONS 3

#define current(self) (self->source == TECHNE ? t_get_real_time() * 1e-9 : (self->source == DYNAMICS ? t_get_dynamics_time() * 1e-9 : t_get_iterations()))

@implementation Timer

-(void) init
{
    [super init];

    self->tick = LUA_REFNIL;    
    self->shortcircuit = 0;
    self->period = 0.0 / 0.0;
    self->source = TECHNE;
}

-(void) toggle
{
    /* If we're about to get unlinked and our period is infinity
     * tick. */
        
    if (self->linked && isinf(self->period) > 0) {
        self->shortcircuit = 1;
        [self tick];
        self->shortcircuit = 0;
    }

    [super toggle];

    /* Reset the timer if it's just been linked. */
    
    if (self->linked) {
        self->previous = current(self);
        self->elapsed = 0;
        self->delta = 0;
        self->count = 0;
    }
}

-(void) free
{
     luaL_unref (_L, LUA_REGISTRYINDEX, self->tick);

     [super free];
}

-(void) tick
{
    double now;

    now = current(self);
    self->delta = now - self->previous;

    if (self->shortcircuit || self->delta >= self->period) {
	self->elapsed += self->delta;
	self->count += 1;

	t_pushuserdata (_L, 1, self);
	lua_pushnumber (_L, self->count);
	lua_pushnumber (_L, self->delta);
	lua_pushnumber (_L, self->elapsed);

	t_callhook (_L, self->tick, 4, 0);

	self->previous = now;
    }
}

-(void) stepBy: (double) h at: (double) t
{
    if (self->source == TECHNE ||
        self->source == DYNAMICS) {
        [self tick];
    }
    
    [super stepBy: h at: t];
}

-(void) transform
{
    [self tick];
    [super transform];
}

-(int) _get_period
{
    if (!isnan(self->period)) {
        lua_pushnumber (_L, self->period);
    } else {
        lua_pushnil (_L);
    }

    return 1;
}

-(int) _get_tick
{
    lua_rawgeti (_L, LUA_REGISTRYINDEX, self->tick);

    return 1;
}

-(int) _get_count
{
    lua_pushnumber (_L, self->count);

    return 1;
}

-(int) _get_source
{
    switch (self->source) {
    case TECHNE: lua_pushliteral(_L, "techne"); break;
    case DYNAMICS: lua_pushliteral(_L, "dynamics"); break;
    case ITERATIONS: lua_pushliteral(_L, "iterations"); break;
    }

    return 1;
}

-(int) _get_elapsed
{
    lua_pushnumber (_L, self->elapsed);

    return 1;
}

-(void) _set_period
{
    if (lua_isnil (_L, 3)) {
        self->period = 0.0 / 0.0;
    } else {
        self->period = lua_tonumber (_L, 3);
    }
}

-(void) _set_tick
{
    luaL_unref (_L, LUA_REGISTRYINDEX, self->tick);
    self->tick = luaL_ref (_L, LUA_REGISTRYINDEX);
}

-(void) _set_count
{
    T_WARN_READONLY;
}

-(void) _set_source
{
    const char *new;

    new = lua_tostring (_L, 3);
    
    if (!strcmp(new, "techne")) {
        self->source = TECHNE;
    } else if (!strcmp(new, "dynamics")) {
        self->source = DYNAMICS;
    } else if (!strcmp(new, "iterations")) {
        self->source = ITERATIONS;
    } else {
        t_print_warning("'%s' is not a valid timer source.\n", new);
    }
}

-(void) _set_elapsed
{
    T_WARN_READONLY;
}

@end
