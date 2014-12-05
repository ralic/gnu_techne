/* Copyright (C) 2014 Papavasileiou Dimitris
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
#include <stdlib.h>

#include "libmeteorology.h"

static double *values[3];
static int lengths[3];

static double lookup (double x, double *values, int length)
{
    double *a, *b;
    int k;

    if (length > 0) {
        for(k = 0, a = values, b = a + 2;
            k < 2 * length - 4 && b[0] <= x ;
            k += 2, a = b, b += 2);

        return a[1] + (b[1] - a[1]) / (b[0] - a[0]) * (x - a[0]);
    } else {
        return 0;
    }
}

static int compare (const void *a, const void *b)
{
    if (((double *)a)[1] == ((double *)b)[1]) {
        return 0;
    } else if (((double *)a)[1] < ((double *)b)[1]) {
        return 1;
    } else {
        return -1;
    }
}

double get_sample_at (meteorology_SampleType type, double h)
{
    return lookup(h, values[type], lengths[type]);
}

void get_samples (meteorology_SampleType type,
                  const double **samples, int *length)
{
    *samples = (const double *)values[type];
    *length = lengths[type];
}

void set_samples (meteorology_SampleType type,
                  const double *samples, const int length)
{
    lengths[type] = length;

    if (samples) {
        values[type] = (double *)realloc(values[type],
                                         2 * length * sizeof(double));
        memcpy(values[type], samples, 2 * length * sizeof(double));
        qsort (values[type], length, 2 * sizeof(double), compare);
    }
}
