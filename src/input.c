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
#include <lua.h>
#include <lauxlib.h>

#include "techne.h"
#include "input.h"
#include "cursor.h"
#include "root.h"

static unsigned int *keys;
static int keys_n;

static GdkEvent **events;
static int events_n;

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
    [super init];
    self->index = 1;
    
    lua_pushstring (_L, "input");
    lua_setfield (_L, -2, "tag");

    events_n = 1;
    events = malloc (events_n * sizeof(GdkEvent *));
    events[0] = NULL;

    instance = self;
}

+(Builtin *)instance
{
    return instance;
}

+(GdkEvent **)events
{
    return events;
}

-(void) iterate
{
    Root *root;
    GdkEvent *event;
    int n;
    
    t_begin_interval(self);

    for (n = 0, event = gdk_event_get();
         event;
         n += 1, event = gdk_event_get()) {
	/* _TRACE ("%d\n", event->type); */

	assert(event);

        /* Ignore double/triple-click events for now.  Individual
         * press/release event sets are generated as expected. */
        
        if (event->type == GDK_2BUTTON_PRESS ||
            event->type == GDK_3BUTTON_PRESS) {
            gdk_event_free (event);
            continue;
        }

	/* Ignore consecutive keypresses for the same key.  These are
	 * always a result of key autorepeat. */
    
	if (event->type == GDK_KEY_PRESS) {
	    unsigned int k;
	    int i, j = -1, skip = 0;

	    k = ((GdkEventKey *)event)->keyval;
	
	    for (i = 0 ; i < keys_n ; i += 1) {
                /* Find an empty spot in case we need to store the
                 * key. */
                
		if (keys[i] == 0) {
		    j = i;
		}

                /* Check if it's been pressed already. */
                
		if (keys[i] == k) {
                    skip = 1;
                    break;
		}
	    }

            /* Store the key making space if necesarry. */

            if (skip) {
                gdk_event_free (event);
                continue;
            } else if (j >= 0) {
		keys[j] = k;
	    } else {
		if (i == keys_n) {
		    keys_n += 1;
		    keys = realloc (keys, keys_n * sizeof(unsigned int));
		}

		keys[i] = k;
	    }
	} else if (event->type == GDK_KEY_RELEASE) {
	    unsigned int k;
	    int i;

	    k = ((GdkEventKey *)event)->keyval;

            /* Remove the key from the list. */
            
	    for (i = 0 ; i < keys_n ; i += 1) {
		if (keys[i] == k) {
		    keys[i] = 0;
		    break;
		}
	    }
	}
	    
	/* Pass it on to the node tree. */

	switch (event->type) {
	case GDK_NOTHING:
	    break;
	case GDK_KEY_PRESS: case GDK_KEY_RELEASE:
	case GDK_BUTTON_PRESS: case GDK_BUTTON_RELEASE:
	case GDK_SCROLL: case GDK_MOTION_NOTIFY:
            if (n == events_n - 1) {
                events_n += 1;
                events = realloc (events, events_n * sizeof(GdkEvent *));
                events[n + 1] = NULL;
            }

            assert(events[n] == NULL);
            events[n] = event;
            
	    break;
	default:
	    gdk_event_put(event);
            gdk_event_free (event);
	    break;
	}
    }
    
    t_end_interval(self);

    for (root = [Root nodes] ; root ; root = (Root *)root->right) {
        recurse(root);
    }

    t_begin_interval(self);

    for (n = 0 ; n < events_n && events[n] ; n += 1) {
        gdk_event_free(events[n]);
        events[n] = NULL;
    }
    
    t_end_interval(self);
}

@end

int luaopen_input (lua_State *L)
{
    [[Input alloc] init];

    return 1;
}
