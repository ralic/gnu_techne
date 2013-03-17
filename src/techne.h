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

/* Workarounds for buggy platforms. */

#ifndef HAVE_ASPRINTF
#include <compat/asprintf.h>
#endif

#include <lua.h>
#include <assert.h>

#include "builtin.h"

typedef enum {
    T_FLUSH_ONLY = 0,
    T_FREEABLE = 1,
    
    T_LOAD = 0,
    T_MULTIPLY = 1,
    
    T_VERTEX_STAGE = 0,
    T_GEOMETRY_STAGE = 1,
    T_FRAGMENT_STAGE = 2,
    T_TESSELATION_CONTROL_STAGE = 3,
    T_TESSELATION_EVALUATION_STAGE = 4,
} t_Enumerated;

lua_State *_L;

const char *t_ansi_color (int i, int j);
int t_call (lua_State *L, int nargs, int nresults);
void t_print_message (const char *format, ...);
void t_print_warning (const char *format, ...);
void t_print_error (const char *format, ...);

void t_print_timing_resolution();
long long int t_get_real_time ();
long long int t_get_cpu_time ();

void *t_build_pool(int factor, size_t size, t_Enumerated mode);
void *t_allocate_pooled (void *p);
void t_free_pooled (void *p, void *block);
void t_reset_pool (void *p);
void t_flush_pool (void *p);
void t_free_pool (void *p);

void t_load_projection (float *matrix);
void t_push_projection (float *matrix);
void t_pop_projection ();
void t_copy_projection(float *matrix);

void t_load_modelview (float *matrix, t_Enumerated mode);
void t_push_modelview (float *matrix, t_Enumerated mode);
void t_pop_modelview ();
void t_copy_modelview(float *matrix);

void t_get_pointer (int *x, int *y);
void t_warp_pointer (int x, int y);

void t_convert_spring(double k_s, double k_d, double *erp, double *cfm);

int luaopen_moremath (lua_State *L);
int luaopen_morebase (lua_State *L);

@interface Techne: Builtin {
}

-(int) _get_iterate;
-(int) _get_iterations;	
-(void) _set_iterate;
-(void) _set_iterations;

@end

int luaopen_techne (lua_State *L);

#endif
