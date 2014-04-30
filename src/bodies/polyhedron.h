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

#ifndef _POLYHEDRON_H_
#define _POLYHEDRON_H_

#include <lua.h>
#include <ode/ode.h>
#include "../array/array.h"
#include "body.h"

@interface Polyhedron: Body {
    dTriMeshDataID data;

    array_Array *vertices;
    array_Array *indices;

    int references[2];
}

-(void) update;

@end

#endif
