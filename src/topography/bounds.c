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
#include <string.h>
#include <assert.h>

#include "roam.h"

static int order, overflown;
static unsigned short *H, *B;

static int log2i(int n)
{
    int i;

    for(i = 0 ; n != 0 && (n & 1) == 0 ; i += 1, n >>= 1);

    return i;
}

static unsigned int calculate_subtree_bounds(int v_0, int v_1, int v_2,
                                             unsigned int i_0,
                                             unsigned int i_1,
                                             unsigned int i_2,
                                             unsigned int l)
{
    int e, e_0, e_1, v_c;
    unsigned i_c;

    i_c = (i_0 + i_1) >> 1;

    /* Errors for leaf triangles are identically zero so there's no need
     * to actually store them. */

    if(l < 2 * order) {
        /* Calculate the center vertex's actual height. */

        v_c = H[i_c];

        e_0 = calculate_subtree_bounds(v_2, v_0, v_c, i_2, i_0, i_c, l + 1);
        e_1 = calculate_subtree_bounds(v_1, v_2, v_c, i_1, i_2, i_c, l + 1);

        /* The triangle's error is equal to the max of its childrens' errors
         * plus the variance. */

        if(e_0 >= e_1) {
            e = e_0 + abs(2 * v_c - (v_0 + v_1));
        } else {
            e = e_1 + abs(2 * v_c - (v_0 + v_1));
        }

        /* fprintf (stderr, "%d\n", e >> 1); */
        assert (e >= 0);
        assert (e >= e_0 && e >= e_1);

        /* assert ((e >> 1) <= 65535); */

        if(B[i_c] < (e >> 1)) {
            if ((e >> 1) > 65535) {
                /* fprintf (stderr, "Error bound overflow at level %d: %d\n", l, e >> 1); */
                B[i_c] = 65535;
                overflown += 1;
            } else {
                B[i_c] = (e >> 1);
            }
        }
    } else {
        e = 0;
    }

    return e;
}

void calculate_tile_bounds(unsigned short *heights, unsigned short *bounds,
                           int size)
{
    int l;

    overflown = 0;
    order = log2i(size - 1);
    l = size - 1;
    H = heights;
    B = bounds;

    memset (B, 0, size * size * sizeof(unsigned short));
    B[0] = 65535;
    B[l] = 65535;
    B[l * (l + 1)] = 65535;
    B[l * (l + 2)] = 65535;

    calculate_subtree_bounds(H[0], H[l * (l + 2)], H[l * (l + 1)],
                             0, l * (l + 2), l * (l + 1),
                             0);

    calculate_subtree_bounds(H[l * (l + 2)], H[0], H[l],
                             l * (l + 2), 0, l,
                             0);
}
