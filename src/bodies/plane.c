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

#include <ode/ode.h>
#include "gl.h"

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include "techne.h"
#include "plane.h"

@implementation Plane

-(void) init
{
    self->geom = dCreatePlane (NULL, 0, 0, 1, 0);
    dGeomSetData (self->geom, self);

    [super init];
}

-(void) fasten
{
    /* We're unplacable so do nothing here. */
}

-(void) release
{
    /* We're unplacable so do nothing here. */
}

-(void) recalculate
{
    double *r, *R;

    r = self->position;
    R = self->orientation;

/*      printf ("%f, %f, %f\n", R[2], R[5], R[8]); */
/*      printf ("%f, %f, %f\n", r[0], r[1], r[2]); */
    dGeomPlaneSetParams (self->geom,
                         R[2], R[5], R[8],
                         r[0] * R[2] +
                         r[1] * R[5] +
                         r[2] * R[8]);
}

-(void) _set_position
{
    [super _set_position];
    [self recalculate];
}

-(void) _set_orientation
{
    [super _set_orientation];
    [self recalculate];
}

@end
