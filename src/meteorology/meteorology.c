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
#include <ctype.h>
#include <lua.h>
#include <lauxlib.h>

#include "techne.h"
#include "meteorology.h"

static double *values[3];
static int lengths[3];

static double lookup (double x, double *values, int length)
{
    double *a, *b;
    int k;
    
    if (length > 0) {
	for(k = 0, a = values, b = a + 2;
	    k < 2 * length - 4 && b[0] <= x ;
	    k += 2, a = b, b += 2);
	
	return a[1] + (b[1] - a[1]) / (b[0] - a[0]) * (x - a[0]);
    } else {
	return 0;
    }
}

static int compare (const void *a, const void *b)
{
    if (((double *)a)[1] == ((double *)b)[1]) {
	return 0;
    } else if (((double *)a)[1] < ((double *)b)[1]) {
	return 1;
    } else {
	return -1;
    }
}

double get_temperature_at (double h)
{
    return lookup(h, values[0], lengths[0]);
}

double get_pressure_at (double h)
{
    return lookup(h, values[1], lengths[1]);
}

double get_density_at (double h)
{
    return lookup(h, values[2], lengths[2]);
}

@implementation Meteorology

-(int) _get_temperature
{
    int i;
    
    lua_newtable (_L);
	
    for (i = 0 ; i < lengths[0] ; i += 1) {
        lua_pushnumber (_L, values[0][2 * i]);
        lua_pushnumber (_L, values[0][2 * i + 1]);
        lua_rawset (_L, -3);
    }

    return 1;
}

-(int) _get_pressure
{
    int i;
    
    lua_newtable (_L);
	
    for (i = 0 ; i < lengths[1] ; i += 1) {
        lua_pushnumber (_L, values[1][2 * i]);
        lua_pushnumber (_L, values[1][2 * i + 1]);
        lua_rawset (_L, -3);
    }

    return 1;
}

-(int) _get_density
{
    int i;
    
    lua_newtable (_L);
	
    for (i = 0 ; i < lengths[2] ; i += 1) {
        lua_pushnumber (_L, values[2][2 * i]);
        lua_pushnumber (_L, values[2][2 * i + 1]);
        lua_rawset (_L, -3);
    }

    return 1;
}

-(void) _set_temperature
{
    int n;
    
    if (lua_istable (_L, 3)) {
        /* Get count of samples. */

        lua_pushnil (_L);
        for (n = 0;
             lua_next(_L, 3) != 0;
             n += lua_type(_L, -1) == LUA_TNUMBER) {
            lua_pop(_L, 1);
        }

        lengths[0] = n;
        values[0] = (double *)realloc(values[0], 2 * n * sizeof(double));

        /* Now get the samples. */
	    
        lua_pushnil (_L);
        for (n = 0;
             lua_next(_L, 3) != 0;
             n += lua_type(_L, -1) == LUA_TNUMBER) {
            values[0][2 * n] = lua_tonumber(_L, -2);
            values[0][2 * n + 1] = lua_tonumber(_L, -1);
            lua_pop(_L, 1);
        }

        /* And sort. */
	    
        qsort (values[0], lengths[0], 2 * sizeof(double), compare);
    } else {
        lengths[0] = 0;
    }
}

-(void) _set_pressure
{
    int n;
    
    if (lua_istable (_L, 3)) {
        /* Get count of samples. */

        lua_pushnil (_L);
        for (n = 0;
             lua_next(_L, 3) != 0;
             n += lua_type(_L, -1) == LUA_TNUMBER) {
            lua_pop(_L, 1);
        }

        lengths[1] = n;
        values[1] = (double *)realloc(values[1], 2 * n * sizeof(double));

        /* Now get the samples. */
	    
        lua_pushnil (_L);
        for (n = 0;
             lua_next(_L, 3) != 0;
             n += lua_type(_L, -1) == LUA_TNUMBER) {
            values[1][2 * n] = lua_tonumber(_L, -2);
            values[1][2 * n + 1] = lua_tonumber(_L, -1);
            lua_pop(_L, 1);
        }

        /* And sort. */
	    
        qsort (values[1], lengths[1], 2 * sizeof(double), compare);
    } else {
        lengths[1] = 0;
    }
}

-(void) _set_density
{
    int n;
    
    if (lua_istable (_L, 3)) {
        /* Get count of samples. */

        lua_pushnil (_L);
        for (n = 0;
             lua_next(_L, 3) != 0;
             n += lua_type(_L, -1) == LUA_TNUMBER) {
            lua_pop(_L, 1);
        }

        lengths[2] = n;
        values[2] = (double *)realloc(values[2], 2 * n * sizeof(double));

        /* Now get the samples. */
	    
        lua_pushnil (_L);
        for (n = 0;
             lua_next(_L, 3) != 0;
             n += lua_type(_L, -1) == LUA_TNUMBER) {
            values[2][2 * n] = lua_tonumber(_L, -2);
            values[2][2 * n + 1] = lua_tonumber(_L, -1);
            lua_pop(_L, 1);
        }

        /* And sort. */
	    
        qsort (values[2], lengths[2], 2 * sizeof(double), compare);
    } else {
        lengths[2] = 0;
    }
}

@end

int luaopen_meteorology (lua_State *L)
{
    [[Meteorology alloc] init];

    return 1;
}
