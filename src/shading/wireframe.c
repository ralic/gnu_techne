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

#include "gl.h"

#include "algebra.h"
#include "techne.h"
#include "wireframe.h"

@implementation Wireframe

-(void) init
{
    [super init];

    self->enabled = 1;
}

-(void) draw: (int)frame
{
    if (self->enabled) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    }

    [super draw: frame];

    if (self->enabled) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }
}

-(int) _get_enabled
{
    lua_pushboolean (_L, self->enabled);

    return 1;
}

-(void) _set_enabled
{
    self->enabled = lua_toboolean (_L, 3);
}

@end
