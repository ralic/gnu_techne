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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <errno.h>
#include <assert.h>
#include <limits.h>

#include <pgm.h>

static gray *elevation;
static unsigned short *bounds;
static int order;
static int *mins, *maxes;
static int overflown;

static int log2i(int n)
{
    int i;

    for(i = 0 ; n != 0 && (n & 1) == 0 ; i += 1, n >>= 1);

    return i;
}

static unsigned int calculate_error_bounds(int v0,
                                           int v1,
                                           int v2,
                                           unsigned int i0,
                                           unsigned int i1,
                                           unsigned int i2,
                                           unsigned int l)
{
    int e, e0, e1, vc;
    unsigned ic;

    ic = (i0 + i1) >> 1;

    /* Errors for leaf triangles are identically zero so there's no need
     * to actually store them. */

    if(l < 2 * order) {
        /* Calculate the center vertex's actual height. */

        vc = elevation[ic];

        e0 = calculate_error_bounds(v2, v0, vc, i2, i0, ic, l + 1);
        e1 = calculate_error_bounds(v1, v2, vc, i1, i2, ic, l + 1);

        /* The triangle's error is equal to the max of its childrens' errors
         * plus the variance. */

        if(e0 >= e1) {
            e = e0 + abs(2 * vc - (v0 + v1));
        } else {
            e = e1 + abs(2 * vc - (v0 + v1));
        }

        /* fprintf (stderr, "%d\n", e >> 1); */
        assert (e >= 0);
        assert (e >= e0 && e >= e1);

        /* assert ((e >> 1) <= 65535); */

        if(bounds[ic] < (e >> 1)) {
            if (mins[l] > (e >> 1)) {
                mins[l] = (e >> 1);
            }

            if (maxes[l] < (e >> 1)) {
                maxes[l] = (e >> 1);
            }

            if ((e >> 1) > 65535) {
                /* fprintf (stderr, "Error bound overflow at level %d: %d\n", l, e >> 1); */
                bounds[ic] = 65535;
                overflown += 1;
            } else {
                bounds[ic] = (e >> 1);
            }
        }
    } else {
        e = 0;
    }

    return e;
}

static void dump_samples (char *filename)
{
    FILE *fp;
    int i, j, l, rows, cols, format;
    gray max;

    if (filename) {
        fp = fopen(filename, "r");
    } else {
        fp = stdin;
    }

    if(fp == NULL) {
        pm_error("%s.", strerror(errno));
    }

    pgm_readpgminit(fp, &rows, &cols, &max, &format);

    order = log2i(rows - 1);

    if(rows != cols || !order) {
        pm_error("elevation data not a power-of-two-plus-one sided square.");
    }

    fprintf (stderr,
             "%s:"
             "  Given elevation data of order %d.\n"
             "  Calculating error bounds.\n",
             filename ? filename : "stdin", order);

    elevation = (gray *)calloc(rows * cols, sizeof(gray));

    for (i = 0 ; i < rows ; i += 1) {
        pgm_readpgmrow(fp, elevation + i * cols, cols, max, format);
    }

    if (max < 65535) {
        double s = (double)65535 / max;

        fprintf (stderr,
                 "  Maximum height is %d, rescaling height values by %f.\n", max, s);

        for(i = 0 ; i < rows * cols ; i += 1) {
            elevation[i] = (int)(elevation[i] * s);
        }
    }

    overflown = 0;
    mins = (int *)malloc (2 * order * sizeof(int));
    maxes = (int *)malloc (2 * order * sizeof(int));

    for (i = 0 ; i < 2 * order ; i += 1) {
        mins[i] = INT_MAX;
        maxes[i] = 0;
    }

    bounds = (unsigned short *)calloc(rows * cols, sizeof(unsigned short));

    l = cols - 1;

    bounds[0] = 65535;
    bounds[l] = 65535;
    bounds[l * (l + 1)] = 65535;
    bounds[l * (l + 2)] = 65535;

    calculate_error_bounds(elevation[0],
                           elevation[l * (l + 2)],
                           elevation[l * (l + 1)],
                           0, l * (l + 2), l * (l + 1),
                           0);

    calculate_error_bounds(elevation[l * (l + 2)],
                           elevation[0],
                           elevation[l],
                           l * (l + 2), 0, l,
                           0);

    fprintf(stdout, "array.ushorts (%d, %d, \"", rows, cols);

    for(i = 0 ; i < rows * cols ; i += 1) {
        fprintf(stdout, "\\%03d\\%03d",
                elevation[i] & 0xff, elevation[i] >> 8);
    }

    fprintf(stdout, "\"),\n"
            "array.ushorts (%d, %d, \"", rows, cols);

    for(i = 0 ; i < rows * cols ; i += 1) {
        fprintf(stdout, "\\%03d\\%03d", bounds[i] & 0xff, bounds[i] >> 8);
    }

    fprintf(stderr, "\n  Bound ranges:\n  Level\tmin\tmax\n");
    fprintf(stderr, "  ======================\n");
    for(i = 0 ; i < 2 * order ; i += 1) {
        fprintf(stderr, "  %d\t%d\t%d\n", i, mins[i], maxes[i]);
    }

    fprintf(stderr, "\n  Overflown bounds: %d\n", overflown);

    free(mins);
    free(maxes);

    fprintf(stdout, "\"),\n");

    if (filename) {
        fclose (fp);
    }
}

int main (int argc, char **argv)
{
    int i;

    pgm_init(&argc, argv);

    fprintf(stdout, "local array = require \"array\"\n\n"
            "return {\n");

    if (argc > 1) {
        for (i = 1 ; i < argc ; i += 1) {
            dump_samples(argv[i]);
        }
    } else {
        dump_samples(NULL);
    }

    fprintf(stdout, "}\n");

    exit (0);
}
