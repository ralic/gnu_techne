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

#include <string.h>

#include <lua.h>
#include <lauxlib.h>

#include "techne.h"
#include "meteorology.h"
#include "libmeteorology.h"

static void getsamples(lua_State *L, meteorology_SampleType type)
{
    const double *samples;
    int i, n;

    get_samples(type, &samples, &n);

    lua_newtable (_L);

    for (i = 0 ; i < n ; i += 1) {
        lua_pushnumber (_L, samples[2 * i]);
        lua_pushnumber (_L, samples[2 * i + 1]);
        lua_rawset (_L, -3);
    }
}

static void setsamples(lua_State *L, meteorology_SampleType type)
{
    double *samples;
    int n;

    if (lua_istable (_L, 3)) {
        /* Get count of samples. */

        lua_pushnil (_L);
        for (n = 0;
             lua_next(_L, 3) != 0;
             n += lua_type(_L, -1) == LUA_TNUMBER) {
            lua_pop(_L, 1);
        }

        samples = (double *)malloc(2 * n * sizeof(double));

        /* Now get the samples. */

        lua_pushnil (_L);
        for (n = 0;
             lua_next(_L, 3) != 0;
             n += lua_type(_L, -1) == LUA_TNUMBER) {
            samples[2 * n] = lua_tonumber(_L, -2);
            samples[2 * n + 1] = lua_tonumber(_L, -1);
            lua_pop(_L, 1);
        }

        set_samples(type, samples, n);
    } else {
        set_samples(type, NULL, 0);
    }
}

@implementation Meteorology

-(int) _get_temperature
{
    getsamples(_L, METEOROLOGY_TEMPERATURE);

    return 1;
}

-(int) _get_pressure
{
    getsamples(_L, METEOROLOGY_PRESSURE);

    return 1;
}

-(int) _get_density
{
    getsamples(_L, METEOROLOGY_DENSITY);

    return 1;
}

-(void) _set_temperature
{
    setsamples(_L, METEOROLOGY_TEMPERATURE);
}

-(void) _set_pressure
{
    setsamples(_L, METEOROLOGY_PRESSURE);
}

-(void) _set_density
{
    setsamples(_L, METEOROLOGY_DENSITY);
}

@end

int luaopen_meteorology (lua_State *L)
{
    [[Meteorology alloc] init];

    return 1;
}
