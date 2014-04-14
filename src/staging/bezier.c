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
#include <math.h>

#include <lua.h>
#include <lauxlib.h>

#include "gl.h"

#include "techne.h"
#include "algebra.h"
#include "array/array.h"
#include "bezier.h"

@implementation Bezier

-(void) init
{
    [super init];

    self->reference = LUA_REFNIL;
    self->segments_n = 0;
    self->time = 0;
    self->speed = 1;
}

-(void) free
{
     luaL_unref (_L, LUA_REGISTRYINDEX, self->reference);

     [super free];
}

-(void) toggle
{
    [super toggle];
}

-(int) _get_speed
{
    lua_pushnumber(_L, self->speed);

    return 1;
}

-(void) _set_speed
{
    self->speed = lua_tonumber(_L, 3);
}

-(int) _get_vertices
{
    array_createarray (_L, ARRAY_TDOUBLE, self->vertices, 2, 3);

    return 1;
}

-(void) _set_vertices
{
    array_Array *array;

    luaL_unref (_L, LUA_REGISTRYINDEX, self->reference);

    if (!lua_isnoneornil(_L, 3)) {
        array = array_checkcompatible (_L, 3,
                                       ARRAY_TYPE | ARRAY_RANK,
                                       ARRAY_TDOUBLE, 2);

        if (array->size[1] != 3) {
            lua_pushstring (_L, "Curve vertices must be three-dimensional.");
            lua_error (_L);
        }

        /* Check vertex count. */

        if ((double)((array->size[0] - 1) / 3) !=
            (double)(array->size[0] - 1) / 3) {
            lua_pushstring (_L,
                            "Number of vertices for a cubic curve must be "
                            "multiple-of-three plus 1.");
            lua_error (_L);
        }

        self->vertices = array->values.doubles;
        self->reference = luaL_ref(_L, LUA_REGISTRYINDEX);
        self->segments_n = (array->size[0] - 1) / 3;
    }
}

-(void) stepBy: (double) h at: (double) t
{
    int n;

    n = (int)self->time;

    if (n < self->segments_n) {
        double (*P)[3], u, u_1, v[3];
        int i;

        u = self->time - floor(self->time);
        u_1 = 1 - u;
        P = ((double (*)[3])self->vertices) + 3 * n;

        for (i = 0 ; i < 3 ; i += 1) {
            v[i] = 3 * u_1 * u_1 * (P[1][i] - P[0][i]) +
                6 * u_1 * u * (P[2][i] - P[1][i]) +
                3 * u * u * (P[3][i] - P[2][i]);
        }

        self->time += self->speed * h / t_length_3(v);
    }

    [super stepBy: h at: t];
}

-(void) transform
{
    int n;

    n = (int)self->time;

    if (n < self->segments_n) {
        double (*P)[3], u, u_1, t[3];
        double s[3], U[3] = {0, 0, 1};
        double *R, *r;

        int i;

        R = self->orientation;
        r = self->position;

        u = self->time - floor(self->time);
        u_1 = 1 - u;
        P = ((double (*)[3])self->vertices) + 3 * n;

        for (i = 0 ; i < 3 ; i += 1) {
            r[i] =
                u_1 * u_1 * u_1 * P[0][i] +
                3 * u_1 * u_1 * u * P[1][i] +
                3 * u_1 * u * u * P[2][i] +
                u * u * u * P[3][i];

            t[i] =
                3 * u_1 * u_1 * (P[1][i] - P[0][i]) +
                6 * u_1 * u * (P[2][i] - P[1][i]) +
                3 * u * u * (P[3][i] - P[2][i]);
        }

        t_normalize_3(t);
        t_cross(s, t, U);
        t_cross(U, t, s);

        R[0] = s[0];
        R[3] = s[1];
        R[6] = s[2];

        R[1] = U[0];
        R[4] = U[1];
        R[7] = U[2];

        R[2] = t[0];
        R[5] = t[1];
        R[8] = t[2];
    }

    [super transform];
}

@end
