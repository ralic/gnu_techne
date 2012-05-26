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

#include <lua.h>
#include <lauxlib.h>

#include <gvc.h>
#include <graph.h>

#include "node.h"

static Agnode_t *draw (Agraph_t *G, Node *root)
{
    Agnode_t *node, *other;
    Node *child;
    char name[16];

    snprintf (name, 16, "%p", root);
    
    node = agnode (G, name);
    
    if (root->tag) {
	agsafeset (node, "label", (char *)root->tag, "");
    } else {
	agsafeset (node, "label", (char *)[root name], "");
    }

    agsafeset (node, "URL", "http://foo.org", "");

    for (child = root->down ; child ; child = child->right) {
	other = draw (G, child);
	agedge (G, node, other);
    }

    return node;
}

static int foo (lua_State *L)
{
    Node *object;

    Agraph_t *G;
    GVC_t *gvc;

    gvc = gvContext();
    G = agopen ("G", AGRAPHSTRICT);

    object = *(Node **)lua_touserdata(L, 1);
    draw (G, object);
    
    gvLayout (gvc, G, "dot");

    gvRenderFilename (gvc, G, "svg", "/tmp/foo.svg");
    
    gvFreeLayout (gvc, G);
    agclose (G);
    gvFreeContext (gvc);

    return 0;
}

int luaopen_console_core (lua_State *L)
{
    luaL_Reg core[] = {
	{"foo", foo},
	{NULL, NULL},
    };
    
    lua_newtable (L);
    luaL_newlib (L, core);

    return 1;
}
