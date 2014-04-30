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

#ifndef _PROFILING_H_
#define _PROFILING_H_

struct queryset {
    unsigned int queries[2];
    struct queryset *left, *right;
};

typedef struct {
    struct queryset *sets;
    long long unsigned int interval;
    int frames;
} t_GPUProfilingInterval;

typedef struct {
    long long unsigned int total[2], lua[2];
    int frames;
} t_CPUProfilingInterval;

typedef struct {
    long long unsigned int count;
    int frames;
} t_ProfilingCount;

void t_begin_gpu_interval (t_GPUProfilingInterval *profile);
void t_end_gpu_interval (t_GPUProfilingInterval *profile);
void t_free_profiling_queries(t_GPUProfilingInterval *profile);

void t_begin_cpu_interval (t_CPUProfilingInterval *profile);
void t_pause_cpu_interval (t_CPUProfilingInterval *profile);
void t_end_cpu_interval (t_CPUProfilingInterval *profile);

void t_add_count_sample(t_ProfilingCount *profile,
                        unsigned long int count);

void t_pushcoreinterval (lua_State *L, t_CPUProfilingInterval *profile);
void t_pushuserinterval (lua_State *L, t_CPUProfilingInterval *profile);
void t_pushgraphicsinterval (lua_State *L, t_GPUProfilingInterval *profile);
void t_pushcount (lua_State *L, t_ProfilingCount *profile);

void t_enable_profiling ();
void t_advance_profiling_frame ();

#endif
