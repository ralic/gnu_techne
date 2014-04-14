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

#include <lua.h>
#include <lauxlib.h>

#include "gl.h"

#include "techne.h"
#include "algebra.h"
#include "observer.h"

static Observer *instance;

@implementation Observer

+(Observer *) instance
{
    return instance;
}

-(void) toggle
{
    [super toggle];

    if (self->linked) {
        if (instance) {
            t_print_error ("Only one Observer node should be linked to "
                           "the scene.\n");
            abort();
        }

        instance = self;
    } else {
        assert (instance == self);
        instance = NULL;
    }
}

-(void) transform
{
    Transform *root;

    [super transform];

    for (root = self ; root->up ; root = (Transform *)root->up);

    {
        double *R, *r;
        double f[3], s[3], u[3];
        double M[16];

        R = self->rotation;
        r = self->translation;

        /* Compute a matrix as in:

            gluLookAt (r[0], r[1], r[2],
                       r[0] - R[2], r[1] - R[5], r[2] - R[8],
                       -R[0], -R[3], -R[6]);

           and leave it on the bottom of the matrix stack. */

        s[0] = R[0];
        s[1] = R[3];
        s[2] = R[6];

        u[0] = -R[1];
        u[1] = -R[4];
        u[2] = -R[7];

        f[0] = -R[2];
        f[1] = -R[5];
        f[2] = -R[8];

        M[0] = R[0];
        M[1] = -R[1];
        M[2] = -R[2];
        M[12] = -t_dot_3(s, r);

        M[4] = R[3];
        M[5] = -R[4];
        M[6] = -R[5];
        M[13] = -t_dot_3(u, r);

        M[8] = R[6];
        M[9] = -R[7];
        M[10] = -R[8];
        M[14] = -t_dot_3(f, r);

        M[3] = M[7] = M[11] = 0;
        M[15] = 1;

        t_load_modelview(M, T_LOAD);
    }
}

@end
