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

#ifndef _TECHNE_H_
#define _TECHNE_H_

#include <lua.h>
#include <assert.h>

#include "builtin.h"

typedef enum {
    T_FLUSH_ONLY = 0,
    T_FREEABLE,
} t_PoolMode;

typedef enum {
    T_LOAD = 0,
    T_MULTIPLY,
} t_MatrixLoadMode;

typedef enum {
    T_VERTEX_STAGE = 0,
    T_GEOMETRY_STAGE,
    T_FRAGMENT_STAGE,
    T_TESSELATION_CONTROL_STAGE,
    T_TESSELATION_EVALUATION_STAGE,
    T_STAGES_N,
} t_ProcessingStage;

int _PROFILING;
lua_State *_L;

/* Console output. */

const char *t_ansi_color (int i, int j);
void t_print_message (const char *format, ...);
void t_print_warning (const char *format, ...);
void t_print_error (const char *format, ...);

/* Timing. */

void t_initialize_timing ();
long long unsigned int t_get_dynamics_time ();
long long unsigned int t_get_real_time ();
long long unsigned int t_get_raw_cpu_time ();
long long unsigned int t_get_cpu_time ();

/* Memory pools. */

void *t_build_pool(int factor, size_t size, t_PoolMode mode);
void *t_allocate_pooled (void *p);
void t_free_pooled (void *p, void *block);
void t_reset_pool (void *p);
void t_flush_pool (void *p);
void t_free_pool (void *p);

/* Transform stack manipulation. */

void t_load_projection (double *matrix);
void t_push_projection (double *matrix);
void t_pop_projection ();
void t_copy_projection(double *matrix);

void t_load_modelview (double *matrix, t_MatrixLoadMode mode);
void t_push_modelview (double *matrix, t_MatrixLoadMode mode);
void t_pop_modelview ();
void t_copy_modelview(double *matrix);
void t_copy_transform(double *matrix);

/* Window-related stuff. */

void t_get_pointer (int *x, int *y);
void t_warp_pointer (int x, int y);

/* Tree traversal. */

void t_draw_subtree (Node *root, int frame);
void t_transform_subtree (Node *root);
void t_step_subtree (Node *root, double h, double t);
void t_input_subtree (Node *root);

/* Misc. */

int t_get_iterations ();

void t_convert_from_spring(double k_s, double k_d, double *erp, double *cfm);
void t_convert_to_spring(double erp, double cfm, double *k_s, double *k_d);

int t_call (lua_State *L, int nargs, int nresults);

int t_compiletemplate(lua_State *L, const char *source);
int t_rendertemplate(lua_State *L, const char *source);

int luaopen_moremath (lua_State *L);
int luaopen_morebase (lua_State *L);
int luaopen_profiling (lua_State *L);

@interface Techne: Builtin {
    t_CPUProfilingInterval latency;
}

-(int) _get_profiling;
-(void) _set_profiling;
-(int) _get_iterate;
-(void) _set_iterate;
-(int) _get_iterations;
-(void) _set_iterations;
-(int) _get_interactive;
-(void) _set_interactive;
-(int) _get_time;
-(void) _set_time;
-(int) _get_latency;
-(void) _set_latency;

@end

int luaopen_techne (lua_State *L);

#endif
