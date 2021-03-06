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
#include <lualib.h>
#include <lauxlib.h>

#include <ode/ode.h>
#include "gl.h"

#include "array/array.h"
#include "techne.h"
#include "dynamics.h"
#include "universal.h"

@implementation Universal

-(void) init
{
    self->joint = dJointCreateUniversal (_WORLD, NULL);

    self->anchor[0] = 0;
    self->anchor[1] = 0;
    self->anchor[2] = 0;

    self->axes[0][0] = 1;
    self->axes[0][1] = 0;
    self->axes[0][2] = 0;

    self->axes[1][0] = 0;
    self->axes[1][1] = 1;
    self->axes[1][2] = 0;

    self->stops[0][0] = -dInfinity;
    self->stops[0][1] = dInfinity;

    self->stops[1][0] = -dInfinity;
    self->stops[1][1] = dInfinity;

    [super init];
}

-(void) update
{
    [super update];

    dJointSetUniversalAnchor (self->joint,
                              self->anchor[0],
                              self->anchor[1],
                              self->anchor[2]);

    dJointSetUniversalAxis1 (self->joint,
                             self->axes[0][0],
                             self->axes[0][1],
                             self->axes[0][2]);

    dJointSetUniversalAxis2 (self->joint,
                             self->axes[1][0],
                             self->axes[1][1],
                             self->axes[1][2]);
}

-(int) _get_anchor
{
    dJointGetUniversalAnchor (self->joint, self->anchor);
    array_createarray (_L, ARRAY_TDOUBLE, self->anchor, 1, 3);

    return 1;
}

-(int) _get_axes
{
    int j;

    dJointGetUniversalAxis1 (self->joint, self->axes[0]);
    dJointGetUniversalAxis2 (self->joint, self->axes[1]);

    lua_newtable (_L);

    for(j = 0 ; j < 2 ; j += 1) {
        array_createarray (_L, ARRAY_TDOUBLE, self->axes[j], 1, 3);

        lua_rawseti (_L, -2, j + 1);
    }

    return 1;
}

-(int) _get_motor
{
    int i, j;

    lua_newtable (_L);

    for(j = 0 ; j < 2 ; j += 1) {
        lua_newtable (_L);

        for(i = 0; i < 2; i += 1) {
            lua_pushnumber (_L, self->motor[j][i]);
            lua_rawseti (_L, -2, i + 1);
        }

        lua_rawseti (_L, -2, j + 1);
    }

    return 1;
}

-(int) _get_stops
{
    int i, j;

    lua_newtable (_L);

    for(j = 0 ; j < 2 ; j += 1) {
        lua_newtable (_L);

        lua_newtable (_L);
        for(i = 0; i < 2; i += 1) {
            lua_pushnumber (_L, self->stops[j][i]);
            lua_rawseti (_L, -2, i + 1);
        }
        lua_rawseti (_L, -2, 1);

        lua_newtable (_L);
        for(i = 0; i < 2; i += 1) {
            lua_pushnumber (_L, self->hardness[j][i]);
            lua_rawseti (_L, -2, i + 1);
        }
        lua_rawseti (_L, -2, 2);

        lua_pushnumber (_L, self->bounce[j]);
        lua_rawseti (_L, -2, 3);

        lua_rawseti (_L, -2, j + 1);
    }

    return 1;
}

-(int) _get_tolerance
{
    int j;

    lua_newtable (_L);

    for(j = 0 ; j < 2 ; j += 1) {
        lua_pushnumber (_L, self->tolerance[j]);

        lua_rawseti (_L, -2, j + 1);
    }

    return 1;
}

-(int) _get_positions
{
    dReal state[2];
    int j;

    if (self->joint) {
        state[0] = dJointGetUniversalAngle1 (self->joint);
        state[1] = dJointGetUniversalAngle2 (self->joint);

        lua_newtable (_L);

        for(j = 0 ; j < 2 ; j += 1) {
            lua_pushnumber (_L, state[j]);

            lua_rawseti (_L, -2, j + 1);
        }
    } else {
        lua_pushnil (_L);
    }

    return 1;
}

-(int) _get_rates
{
    dReal state[2];
    int j;

    if (self->joint) {
        state[0] = dJointGetUniversalAngle1Rate (self->joint);
        state[1] = dJointGetUniversalAngle2Rate (self->joint);

        lua_newtable (_L);

        for(j = 0 ; j < 2 ; j += 1) {
            lua_pushnumber (_L, state[j]);

            lua_rawseti (_L, -2, j + 1);
        }
    } else {
        lua_pushnil (_L);
    }

    return 1;
}

-(void) _set_anchor
{
    array_Array *array;
    int i;

    if(!lua_isnil (_L, 3)) {
        array = array_checkcompatible (_L, 3,
                                       ARRAY_TYPE | ARRAY_RANK | ARRAY_SIZE,
                                       ARRAY_TDOUBLE, 1, 3);

        dJointSetUniversalAnchor (self->joint,
                                  array->values.doubles[0],
                                  array->values.doubles[1],
                                  array->values.doubles[2]);

        for (i = 0 ; i < 3 ; i += 1) {
            self->anchor[i] = array->values.doubles[i];
        }
    }
}

-(void) _set_axes
{
    array_Array *arrays[3];
    int i, j;

    if(!lua_isnil (_L, 3)) {
        for(j = 0 ; j < 2 ; j += 1) {
            lua_pushinteger (_L, j + 1);
            lua_gettable (_L, 3);
            arrays[j] = array_checkcompatible (_L, -1,
                                               ARRAY_TYPE | ARRAY_RANK |
                                               ARRAY_SIZE,
                                               ARRAY_TDOUBLE, 1, 3);
            dSafeNormalize3 (arrays[j]->values.doubles);

            lua_pop (_L, 1);

            for (i = 0 ; i < 3 ; i += 1) {
                self->axes[j][i] = arrays[j]->values.doubles[i];
            }
        }

        dJointSetUniversalAxis1 (self->joint,
                                 self->axes[0][0],
                                 self->axes[0][1],
                                 self->axes[0][2]);

        dJointSetUniversalAxis2 (self->joint,
                                 self->axes[1][0],
                                 self->axes[1][1],
                                 self->axes[1][2]);
    }
}

-(void) _set_motor
{
    int i, j;

    if(lua_istable (_L, 3)) {
        for(j = 0 ; j < 2 ; j += 1) {
            lua_rawgeti (_L, 3, j + 1);

            if(lua_istable (_L, -1)) {
                for(i = 0 ; i < 2 ; i += 1) {
                    lua_rawgeti (_L, -1, i + 1);
                    self->motor[j][i] = lua_tonumber (_L, -1);

                    lua_pop (_L, 1);
                }

                dJointSetUniversalParam (self->joint,
                                         dParamVel + dParamGroup * j,
                                         self->motor[j][0]);

                dJointSetUniversalParam (self->joint,
                                         dParamFMax + dParamGroup * j,
                                         self->motor[j][1]);
            }

            lua_pop (_L, 1);
        }
    }
}

-(void) _set_stops
{
    int i, j;

   /* Resetting the self->stops makes sure that lo remains
       smaller than hi between calls. */

    for(j = 0 ; j < 2 ; j += 1) {
        dJointSetUniversalParam (self->joint,
                                 dParamLoStop + dParamGroup * j,
                                 -dInfinity);

        dJointSetUniversalParam (self->joint,
                                 dParamHiStop + dParamGroup * j,
                                 dInfinity);
    }

    if(lua_istable (_L, 3)) {
        for(j = 0 ; j < 2 ; j += 1) {
            lua_rawgeti (_L, 3, j + 1);

            if(lua_istable (_L, -1)) {
                double erp, cfm;

                /* The limits. */

                lua_rawgeti (_L, -1, 1);
                for(i = 0 ; i < 2 ; i += 1) {
                    lua_rawgeti (_L, -1, i + 1);

                    self->stops[j][i] = lua_tonumber (_L, -1);

                    lua_pop (_L, 1);
                }
                lua_pop (_L, 1);

                /* The hardness. */

                lua_rawgeti (_L, -1, 2);

                if (!lua_isnil(_L, -1)) {
                    for(i = 0 ; i < 2 ; i += 1) {
                        lua_rawgeti (_L, -1, i + 1);

                        self->hardness[j][i] = lua_tonumber (_L, -1);

                        lua_pop (_L, 1);
                    }

                    t_convert_from_spring(self->hardness[j][0],
                                          self->hardness[j][1],
                                          &erp, &cfm);
                } else {
                    erp = dWorldGetERP(_WORLD);
                    cfm = dWorldGetCFM(_WORLD);

                    t_convert_to_spring(erp, cfm,
                                        &self->hardness[j][0],
                                        &self->hardness[j][1]);
                }

                lua_pop (_L, 1);

                /* The bounce. */

                lua_rawgeti (_L, -1, 3);
                self->bounce[j] = lua_tonumber (_L, -1);
                lua_pop (_L, 1);

                dJointSetUniversalParam (self->joint,
                                         dParamLoStop + dParamGroup * j,
                                         self->stops[j][0]);

                dJointSetUniversalParam (self->joint,
                                         dParamHiStop + dParamGroup * j,
                                         self->stops[j][1]);

                dJointSetUniversalParam (self->joint,
                                         dParamStopCFM + dParamGroup * j,
                                         cfm);

                dJointSetUniversalParam (self->joint,
                                         dParamStopERP + dParamGroup * j,
                                         erp);

                dJointSetUniversalParam (self->joint,
                                         dParamBounce + dParamGroup * j,
                                         self->bounce[j]);
            }

            lua_pop (_L, 1);
        }
    }
}

-(void) _set_tolerance
{
    int j;

    if(lua_istable (_L, 3)) {
        for(j = 0 ; j < 2 ; j += 1) {
            lua_rawgeti (_L, 3, j + 1);
            self->tolerance[j] = lua_tonumber (_L, -1);

            lua_pop (_L, 1);

            dJointSetUniversalParam (self->joint,
                                     dParamCFM + dParamGroup * j,
                                     self->tolerance[j]);
        }
    }
}

-(void) _set_positions
{
    T_WARN_READONLY;
}

-(void) _set_rates
{
    T_WARN_READONLY;
}

@end
