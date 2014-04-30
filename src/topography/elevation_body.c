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

#include <lua.h>
#include <lauxlib.h>

#include "techne.h"
#include "roam.h"
#include "elevation.h"

static dReal heightfield_data_callback (void *data, int x, int z)
{
    roam_Tileset *tiles;
    ElevationBody *self = data;
    double h;

    /* The sign of the x coordinate needs to be flipped.
       See comment about heightfield frames in ODE and
       Techne below. */

    tiles = self->tileset;
    look_up_sample (tiles, (1 << tiles->depth) * tiles->size[0] - x, z, &h, NULL);

    /* printf ("%d, %d => %f\n", x, z, h); */

    return h;
}

@implementation ElevationBody

-(void) init
{
    roam_Tileset *tiles;
    Elevation *elevation;

    /* Make a reference to the elevation to make sure it's not
     * collected. */

    elevation = t_tonode (_L, -1);
    self->reference = luaL_ref (_L, LUA_REGISTRYINDEX);

    [super init];

    tiles = self->tileset = &elevation->context.tileset;
    self->data = dGeomHeightfieldDataCreate();

    dGeomHeightfieldDataBuildCallback (self->data,
                                       self,
                                       heightfield_data_callback,
                                       (1 << tiles->depth) *
                                       tiles->size[0] * tiles->resolution[0],
                                       (1 << tiles->depth) *
                                       tiles->size[1] * tiles->resolution[1],
                                       (1 << tiles->depth) *
                                       tiles->size[0] + 1,
                                       (1 << tiles->depth) *
                                       tiles->size[1] + 1,
                                       1, 0, 100, 0);

    dGeomHeightfieldDataSetBounds (self->data, 0, dInfinity);

    /* Create the geom itself. */

    self->geom = dCreateHeightfield(NULL, self->data, 1);

    dGeomSetData (self->geom, self);

    {
        /* ODE assumes that heighfield are oriented with the
           y-axis pointing upwards, while Techne thinks the
           z-axis does.  In order to align the two frames we
           need to apply a transform to the geom but in the
           process the x-axis has to be negated to keep the
           handedness of both frames the same. */

        dMatrix3 R = {-self->orientation[0],
                      self->orientation[2],
                      self->orientation[1],
                      0,

                      -self->orientation[3],
                      self->orientation[5],
                      self->orientation[4],
                      0,

                      -self->orientation[6],
                      self->orientation[8],
                      self->orientation[7],
                      0};

        dGeomSetRotation (self->geom, R);
        dGeomSetPosition (self->geom,
                          self->position[0],
                          self->position[1],
                          self->position[2]);
    }
}

-(void) free
{
    luaL_unref (_L, LUA_REGISTRYINDEX, self->reference);
    dGeomHeightfieldDataDestroy(self->data);

    [super free];
}

-(void) _set_orientation
{
    dMatrix3 R;

    [super _set_orientation];

    /* See comment about heightfield frames
       in ODE and Techne above. */

    R[0] = -self->orientation[0];
    R[1] = self->orientation[2];
    R[2] = self->orientation[1];
    R[3] = 0;

    R[4] = -self->orientation[3];
    R[5] = self->orientation[5];
    R[6] = self->orientation[4];
    R[7] = 0;

    R[8] = -self->orientation[6];
    R[9] = self->orientation[8];
    R[10] = self->orientation[7];
    R[11] = 0;

    dGeomSetRotation (self->geom, R);
}

@end
