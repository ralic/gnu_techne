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

/* Debugging convenience macros. */

#define _TRACE t_print_message ("\033[22;31m%s: %d: \033[22;37m", __FILE__, __LINE__), t_print_message
#define _TOP _TRACE ("@%d\n", lua_gettop(_L))
#define _TYPE(i) _TRACE ("%s@%d\n", lua_typename(_L, lua_type(_L, (i))), i)
#define _STACK {int i; _TRACE ("\033[22;31m\n  >>>>>>>>>>\033[22;37m\n"); for (i = 1 ; i <= lua_gettop(_L) ; i += 1) { t_print_message ("\033[22;31m  %d: %s\033[22;37m\n", i, lua_typename(_L, lua_type(_L, (i)))); } t_print_message ("\033[22;31m  <<<<<<<<<<\n\033[22;37m\n");}

#include <lua.h>
#include <assert.h>

#include "node.h"

lua_State *_L;

typedef enum {
    TPROF_BEGIN,
    TPROF_INPUT,
    TPROF_COLLIDE,
    TPROF_STEP,
    TPROF_TRANSFORM,
    TPROF_PREPARE,
    TPROF_TRAVERSE,
    TPROF_FINISH,
} tprof_Frame;

int t_call (lua_State *L, int nargs, int nresults);
void t_print_message (const char *format, ...);
void t_print_warning (const char *format, ...);
void t_print_error (const char *format, ...);

void tprof_new_row ();
void tprof_begin (tprof_Frame reading);
void tprof_end (tprof_Frame reading);
void tprof_report ();

void t_print_timing_resolution();
long long int t_get_real_time ();
long long int t_get_cpu_time ();

void *t_build_pool(int factor, size_t size);
void *t_allocate_from_pool (void *p);
void t_reset_pool (void *p);

int luaopen_moremath (lua_State *L);

@interface Techne: Node {
}

-(id) initWithArgc: (int)argc andArgv: (char **)argv;

-(void) iterate;

-(int) _get_iterate;
-(void) _set_iterate;
@end

#endif
