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

#define _TRACE t_print_message ("\033[0;31m%s: %d: \033[0m", __FILE__, __LINE__), t_print_message

#define _PRINTV(_N, _F, _V)						\
    {									\
	int i;								\
									\
	t_print_message ("(");						\
									\
	for (i = 0 ; i < _N ; i += 1) {					\
	    if (i != _N - 1) {						\
		t_print_message ("%"_F", ", _V[i]);			\
	    } else {							\
		t_print_message ("%"_F, _V[i]);				\
	    }								\
	}								\
									\
	t_print_message (")\n");					\
    }

#define _TRACEV(_N, _F, _V)						\
    {									\
	t_print_message ("\033[0;31m%s: %d: \033[0m",			\
			 __FILE__, __LINE__);				\
	_PRINTV(_N, _F, _V);						\
    }									\

#define _TRACEM(_N_0, _N_1, _F, _V)					\
    {									\
	int j;								\
									\
	t_print_message ("\033[0;31m%s: %d: \033[0m(\n",		\
			 __FILE__, __LINE__);				\
									\
	for (j = 0 ; j < _N_0 ; j += 1) {				\
	    t_print_message ("  ");					\
	    _PRINTV (_N_1, _F, (_V + j * _N_1));			\
	}								\
									\
	t_print_message (")\n");					\
    }

#define _TOP _TRACE ("@%d\n", lua_gettop(_L))
#define _TYPE(i) _TRACE ("%s@%d\n", lua_typename(_L, lua_type(_L, (i))), i)
#define _STACK {int i; _TRACE ("\033[0;31m\n  >>>>>>>>>>\033[0m\n"); for (i = 1 ; i <= lua_gettop(_L) ; i += 1) { if (lua_type(_L, (i)) == LUA_TNUMBER) { t_print_message ("\033[0;31m  %d: %s \033[0;32m[%f]\033[0m\n", i, lua_typename(_L, lua_type(_L, (i))), lua_tonumber (_L, (i))); } else if (lua_type(_L, (i)) == LUA_TSTRING) { t_print_message ("\033[0;31m  %d: %s \033[0;32m[%s]\033[0m\n", i, lua_typename(_L, lua_type(_L, (i))), lua_tostring (_L, (i))); } else { t_print_message ("\033[0;31m  %d: %s\033[0m\n", i, lua_typename(_L, lua_type(_L, (i)))); }} t_print_message ("\033[0;31m  <<<<<<<<<<\n\033[0m\n");}

#include <lua.h>
#include <assert.h>

#include "node.h"

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

void t_set_projection (float *matrix);
void t_set_modelview (float *matrix);

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
