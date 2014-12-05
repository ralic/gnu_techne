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

#ifndef _LIBMETEOROLOGY_H_
#define _LIBMETEOROLOGY_H_

typedef enum {
    METEOROLOGY_TEMPERATURE = 0,
    METEOROLOGY_PRESSURE,
    METEOROLOGY_DENSITY,
} meteorology_SampleType;

double get_sample_at (meteorology_SampleType type, double h);
void get_samples (meteorology_SampleType type,
                  const double **samples, int *length);
void set_samples (meteorology_SampleType type,
                  const double *samples, const int length);

#endif
