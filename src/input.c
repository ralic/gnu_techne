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

#include <stdlib.h>
#include <string.h>
#include <lua.h>
#include <lauxlib.h>

#include "techne.h"
#include "input.h"
#include "event.h"
#include "root.h"

static GdkEvent **events, **cursor;
static int events_max, events_n;

static Input* instance;

static void recurse (Node *root)
{
    Node *child;
    
    t_begin_interval (root);

    if ([root isKindOf: [Event class]]) {
	[(id)root input];
    } else {
        for (child = root->down ; child ; child = child->right) {
            recurse (child);
        }
    }
    
    t_end_interval (root);
}

@implementation Input

-(void) init
{
    self->index = 1;

    [super init];
    
    lua_pushstring (_L, "input");
    lua_setfield (_L, -2, "tag");

    events_max = 1;
    events = malloc (events_max * sizeof(GdkEvent *));
    events[0] = NULL;

    instance = self;
}

+(Builtin *)instance
{
    return instance;
}

+(GdkEvent *)first
{
    if (events_n > 0) {
        return *(cursor = events);
    } else {
        return NULL;
    }
}

+(GdkEvent *)next
{
    assert (cursor);
    
    if (cursor - events < events_n - 1) {
        return *(cursor += 1);
    } else {
        return NULL;
    }
}

+(void)addEvent: (GdkEvent *)event
{
    assert(event);
    
    if (events_n == events_max) {
        events_max += 1;
        events = realloc (events, events_max * sizeof(GdkEvent *));
    }

    events[events_n] = event;
    events_n += 1;
}

-(void) iterate
{
    Root *root;
    int n;

    for (root = [Root nodes] ; root ; root = (Root *)root->right) {
        recurse(root);
    }

    t_begin_interval(self);

    for (n = 0 ; n < events_n ; n += 1) {
        gdk_event_free(events[n]);
    }

    memset (events, 0, events_n * sizeof (GdkEvent *));
    events_n = 0;
    
    t_end_interval(self);
}

@end

int luaopen_input (lua_State *L)
{
    [[Input alloc] init];

    return 1;
}
