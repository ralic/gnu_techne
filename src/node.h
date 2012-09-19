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

#ifndef _NODE_H_
#define _NODE_H_

#include <lua.h>
#include <objc/runtime.h>

#include "object.h"

#define GET_PREFIX "_get_"
#define SET_PREFIX "_set_"

#define GET_PREFIX_LENGTH (sizeof (GET_PREFIX) - 1)
#define SET_PREFIX_LENGTH (sizeof (SET_PREFIX) - 1)

#define GET_ELEMENT "_get_element"
#define SET_ELEMENT "_set_element"

#define GET_ELEMENT_LENGTH (sizeof (GET_ELEMENT) - 1)
#define SET_ELEMENT_LENGTH (sizeof (SET_ELEMENT) - 1)

#define T_WARN_READONLY							\
	t_print_warning ("Property '%s' of '%s' nodes cannot be set.\n", \
			 sel_getName(_cmd) + SET_PREFIX_LENGTH,		\
			 [self name]);

struct protocol {
    Class class;
    int size;
    struct {
	const char *name;
	SEL selector;
    } *properties[2];
};
 
typedef enum {
    T_BEGIN_PHASE,
    T_INPUT_PHASE,
    T_STEP_PHASE,
    T_TRANSFORM_PHASE,
    T_PREPARE_PHASE,
    T_TRAVERSE_PHASE,
    T_FINISH_PHASE,

    T_PHASE_COUNT
} tprof_Phase;
    
@interface Node: Object {
@public
    Node *left, *right, *up, *down;
    const struct protocol *protocol;

    struct {
	long long int beginning[2], intervals[T_PHASE_COUNT][2];
    } profile;
    
    struct {
	lua_Number number;
	const char *string;
	int reference;
    } key;
    
    struct {
	const char *string;
	int reference;
    } tag;

    int length, linked, rawaccess;
    int children, link, unlink, step, get, set;
    int traverse, prepare, finish, begin;
}

+(const struct protocol *) introspect;

-(void) meetSibling: (Node *)sibling;
-(void) missSibling: (Node *)sibling;
-(void) meetParent: (Node *)parent;
-(void) missParent: (Node *)parent;
-(void) adopt: (Node *)child;
-(void) renounce: (Node *)child;
-(void) toggle;

-(void) begin;
-(void) prepare;
-(void) stepBy: (double) h at: (double) t;
-(void) traverse;
-(void) finish;

-(int) call;

-(int) _get_tag;
-(void) _set_tag;
-(int) _get_profile;
-(void) _set_profile;

-(int) _get_;
-(int) _set_;

-(int) _get_link;
-(int) _get_unlink;
-(int) _get_step;
-(int) _get_traverse;
-(int) _get_prepare;
-(int) _get_finish;
-(int) _get_begin;
-(int) _get_get;
-(int) _get_set;
-(int) _get_parent;
-(int) _get_ancestors;

-(void) _set_parent;
-(void) _set_ancestors;
-(void) _set_link;
-(void) _set_unlink;
-(void) _set_step;
-(void) _set_traverse;
-(void) _set_prepare;
-(void) _set_finish;
-(void) _set_begin;
-(void) _set_get;
-(void) _set_set;

@end

void t_begin_interval (Node *, tprof_Phase reading);
void t_end_interval (Node *, tprof_Phase reading);

int t_is_node(lua_State *L, int index);
id t_test_node (lua_State *L, int index, Class class);
id t_check_node(lua_State *L, int index, Class class);
void t_configure_node (lua_State *L, int index);
void t_export_nodes(lua_State *L, Class *classes);
void t_push_userdata(lua_State *L, int n, ...);
int t_call_hook (lua_State *L, int reference, int n, int m);

#endif
