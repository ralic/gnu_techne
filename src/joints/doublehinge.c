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

#include "array/array.h"
#include "techne.h"
#include "dynamics.h"
#include "doublehinge.h"
#include "body.h"

@implementation Doublehinge

-(void) init
{
    self->joint = dJointCreateDHinge (_WORLD, NULL);

    self->anchors[0][0] = 0;
    self->anchors[0][1] = 0;
    self->anchors[0][2] = 0;

    self->anchors[1][0] = 0;
    self->anchors[1][1] = 0;
    self->anchors[1][2] = 0;

    self->axis[0] = 0;
    self->axis[1] = 0;
    self->axis[2] = 1;

    [super init];
}

-(void) update
{
    if (dJointGetBody (self->joint, 0) ||
        dJointGetBody (self->joint, 1)) {
        dJointGetDHingeAxis (self->joint, self->axis);
        dJointGetDHingeAnchor1 (self->joint, self->anchors[0]);
        dJointGetDHingeAnchor2 (self->joint, self->anchors[1]);
    }

    [super update];

    /* Axis and anchor should be set after the
       joint has been attached. */

    if (self->linked) {
        dJointSetDHingeAxis (self->joint,
                             self->axis[0],
                             self->axis[1],
                             self->axis[2]);

        dJointSetDHingeAnchor1 (self->joint,
                                self->anchors[0][0],
                                self->anchors[0][1],
                                self->anchors[0][2]);

        dJointSetDHingeAnchor2 (self->joint,
                                self->anchors[1][0],
                                self->anchors[1][1],
                                self->anchors[1][2]);
    }
}

-(int) _get_anchors
{
    int j;

    dJointGetDHingeAnchor1 (self->joint, self->anchors[0]);
    dJointGetDHingeAnchor2 (self->joint, self->anchors[1]);

    lua_newtable (_L);

    for(j = 0 ; j < 2 ; j += 1) {
        array_createarray (_L, ARRAY_TDOUBLE, self->anchors[j], 1, 3);
        lua_rawseti (_L, -2, j + 1);
    }

    return 1;
}

-(int) _get_axis
{
    dJointGetDHingeAxis (self->joint, self->axis);
    array_createarray (_L, ARRAY_TDOUBLE, self->axis, 1, 3);

    return 1;
}

-(int) _get_tolerance
{
    lua_pushnumber (_L, self->tolerance);

    return 1;
}

-(void) _set_axis
{
    array_Array *array;
    int i;

    array = array_checkcompatible (_L, 3,
                                   ARRAY_TYPE | ARRAY_RANK | ARRAY_SIZE,
                                   ARRAY_TDOUBLE, 1, 3);

    dSafeNormalize3 (array->values.doubles);

    for (i = 0 ; i < 3 ; i += 1) {
        self->axis[i] = array->values.doubles[i];
    }

    if (self->bodies[0] || self->bodies[1]) {
        dJointSetDHingeAxis (self->joint,
                             array->values.doubles[0],
                             array->values.doubles[1],
                             array->values.doubles[2]);
    }
}

-(void) _set_anchors
{
    array_Array *array;
    int i;

    if(!lua_isnil (_L, 3)) {
        /* Set the first anchor. */

        lua_pushinteger (_L, 1);
        lua_gettable (_L, 3);

        array = array_checkcompatible (_L, -1,
                                       ARRAY_TYPE | ARRAY_RANK | ARRAY_SIZE,
                                       ARRAY_TDOUBLE, 1, 3);

        dJointSetDHingeAnchor1 (self->joint,
                               array->values.doubles[0],
                               array->values.doubles[1],
                               array->values.doubles[2]);

        for (i = 0 ; i < 3 ; i += 1) {
            self->anchors[0][i] = array->values.doubles[i];
        }

        /* Set the second anchor. */

        lua_pushinteger (_L, 2);
        lua_gettable (_L, 3);

        array = array_checkcompatible (_L, -1,
                                       ARRAY_TYPE | ARRAY_RANK | ARRAY_SIZE,
                                       ARRAY_TDOUBLE, 1, 3);

        dJointSetDHingeAnchor2 (self->joint,
                                array->values.doubles[0],
                                array->values.doubles[1],
                                array->values.doubles[2]);

        for (i = 0 ; i < 3 ; i += 1) {
            self->anchors[1][i] = array->values.doubles[i];
        }

        lua_pop (_L, 2);
    }
}

-(void) _set_tolerance
{
    self->tolerance = lua_tonumber (_L, 3);

    dJointSetDHingeParam (self->joint, dParamCFM, self->tolerance);
}

@end
