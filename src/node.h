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

#include "profiling.h"
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
    t_print_warning ("Property '%s' of '%s' nodes cannot be set.\n",    \
                     sel_getName(_cmd) + SET_PREFIX_LENGTH,		\
                     [self name]);

#define T_WARN_WRITEONLY                                                \
    t_print_warning ("Property '%s' of '%s' nodes cannot be queried.\n", \
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
    
@interface Node: Object {
@public
    Node *left, *right, *up, *down, **orphans;

    const struct protocol *protocol;

    const char **prerequisites;
    int prerequisites_n;
    
    t_CPUProfilingInterval core;
    
    struct {
	lua_Number number;
	const char *string;
	int reference;
    } key;
    
    struct {
	const char *string;
	int reference;
    } alias;
    
    struct {
	const char *string;
	int reference;
    } tag;

    double index;
    int length, linked, rawaccess, children;
    int link, unlink, get, set, adopt, abandon, reparent;
}

+(const struct protocol *) introspect;

-(const char *) name;

-(void) setOrphansList: (Node **)list;
-(void) set: (int) n prerequisites: (const char **)list;

-(void) meetSibling: (Node *)sibling;
-(void) missSibling: (Node *)sibling;
-(void) meetParent: (Node *)parent;
-(void) missParent: (Node *)parent;
-(void) adopt: (Node *)child;
-(void) abandon: (Node *)child;
-(void) toggle;

-(int) call;

-(int) _get_tag;
-(void) _set_tag;
-(int) _get_core;
-(void) _set_core;
-(int) _get_user;
-(void) _set_user;

-(int) _get_;
-(int) _set_;

-(int) _get_link;
-(int) _get_unlink;
-(int) _get_get;
-(int) _get_set;
-(int) _get_parent;
-(int) _get_ancestors;
-(int) _get_siblings;
-(int) _get_children;
-(int) _get_annotations;
-(int) _get_attributes;
-(int) _get_index;
-(int) _get_adopt;
-(int) _get_abandon;

-(void) _set_parent;
-(void) _set_ancestors;
-(void) _set_siblings;
-(void) _set_children;
-(void) _set_annotations;
-(void) _set_attributes;
-(void) _set_link;
-(void) _set_unlink;
-(void) _set_get;
-(void) _set_set;
-(void) _set_index;
-(void) _set_adopt;
-(void) _set_abandon;

-(int) _get_type;
-(void) _set_type;

-(int) _get_pedigree;
-(void) _set_pedigree;

@end

int t_isnode(lua_State *L, int index);
id t_tonode (lua_State *L, int index);
id t_testnode (lua_State *L, int index, Class class);
id t_checknode(lua_State *L, int index, Class class);
void t_configurenode (lua_State *L, int index);
void t_exportnodes(lua_State *L, Class *classes);
void t_pushuserdata(lua_State *L, int n, ...);
int t_callhook (lua_State *L, int reference, int n, int m);

#endif
