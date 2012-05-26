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

#include "object.h"

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
    Node *left, *right, *up, *down;
    const struct protocol *protocol;
    char *tag;
    int length, linked, rawaccess;
    int key, children, link, unlink, step, get, set;
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

-(int) _get_tag;
-(void) _set_tag;

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

int t_is_node(lua_State *L, int index);
id t_check_node(lua_State *L, int index, Class class);
void t_export_nodes(lua_State *L, Class *classes);
void t_push_userdata(lua_State *L, int n, ...);
void t_push_hook(lua_State *L, int reference);
int t_call_hook (lua_State *L, int reference, int n, int m);

#endif
