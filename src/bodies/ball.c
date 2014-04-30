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

#include <ode/ode.h>
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include "techne.h"
#include "ball.h"

@implementation Ball

-(void) init
{
    self->geom = dCreateSphere (NULL, 1);
    self->radius = 1;

    dGeomSetData (self->geom, self);

    [super init];
}

-(int) _get_radius
{
    lua_pushnumber (_L, self->radius);

    return 1;
}

-(void) _set_radius
{
    self->radius = lua_tonumber (_L, 3);

    dGeomSphereSetRadius (self->geom, self->radius);
}

@end
