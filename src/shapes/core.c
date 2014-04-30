/* Copyright (C) 2012 Papavasileiou Dimitris
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
#include <stdlib.h>

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include "shape.h"

#define DEFINE_SHAPE(shape, mode)				\
    static int export_##shape (lua_State *L)			\
    {								\
        [[Shape alloc] initWithMode: mode];			\
                                                                \
        /* ...and initialize it. */				\
                                                                \
        if(lua_istable(L, 1)) {					\
            lua_pushnil(L);					\
                                                                \
            while(lua_next(L, 1)) {				\
                lua_pushvalue(L, -2);				\
                lua_insert(L, -2);				\
                lua_settable(L, 2);				\
            }							\
        }							\
                                                                \
        return 1;						\
    }

DEFINE_SHAPE(points, GL_POINTS)
DEFINE_SHAPE(line, GL_LINE_STRIP)
DEFINE_SHAPE(loop, GL_LINE_LOOP)
DEFINE_SHAPE(lines, GL_LINES)
DEFINE_SHAPE(triangles, GL_TRIANGLES)
DEFINE_SHAPE(strip, GL_TRIANGLE_STRIP)
DEFINE_SHAPE(fan, GL_TRIANGLE_FAN)

int luaopen_shapes_core (lua_State *L)
{
    const luaL_Reg shapes[] = {
        {"points", export_points},
        {"line", export_line},
        {"loop", export_loop},
        {"lines", export_lines},
        {"triangles", export_triangles},
        {"strip", export_strip},
        {"fan", export_fan},
        {NULL, NULL}
    };

    lua_newtable (L);
    luaL_setfuncs (L, shapes, 0);

    return 1;
}
