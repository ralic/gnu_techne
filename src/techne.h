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

#include "node.h"

typedef enum {
    T_LOAD,
    T_MULTIPLY,
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

void *t_build_pool(int factor, size_t size);
void *t_allocate_from_pool (void *p);
void t_reset_pool (void *p);

void t_load_projection (float *matrix);
void t_push_projection (float *matrix);
void t_pop_projection ();

void t_load_modelview (float *matrix, t_Enumerated mode);
void t_push_modelview (float *matrix, t_Enumerated mode);
void t_pop_modelview ();

int luaopen_moremath (lua_State *L);

@interface Techne: Node {
}

-(id) initWithArgc: (int)argc andArgv: (char **)argv;

-(void) iterate;

-(int) _get_iterate;
-(int) _get_iterations;	
-(void) _set_iterate;
-(void) _set_iterations;

@end

#endif
