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

#include <gdk/gdk.h>

#include "techne.h"
#include "event.h"
#include "input.h"

static unsigned int *keys;
static int keys_n;

static void recurse (Node *root, GdkEvent *event)
{
    Node *child;
    
    if ([root respondsTo: @selector(inputWithEvent:)]) {
	[(id)root inputWithEvent: event];
    }
    
    for (child = root->down ; child ; child = child->right) {
	t_begin_interval (child, T_INPUT_PHASE);
	recurse (child, event);
	t_end_interval (child, T_INPUT_PHASE);
    }
}

@implementation Input

-(void) iterate: (Transform *)root
{
    GdkEvent *event;
    
    t_begin_interval (root, T_INPUT_PHASE);    

    while ((event = gdk_event_get()) != NULL) {
	int quit = 0;
	
	/* _TRACE ("%d\n", event->type); */

	assert(event);

	/* Ignore consecutive keypresses for the same key.  These are
	 * always a result of key autorepeat. */
    
	if (event->type == GDK_KEY_PRESS) {
	    unsigned int k;
	    int i, j = -1;

	    k = ((GdkEventKey *)event)->keyval;
	
	    for (i = 0 ; i < keys_n ; i += 1) {
		if (keys[i] == 0) {
		    j = i;
		}
	    
		if (keys[i] == k) {
		    return;
		}
	    }

	    if (j >= 0) {
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
	    recurse(root, event);
	    break;
	default:
	    gdk_event_put(event);
	    quit = 1;
	}
		
	gdk_event_free (event);

	if (quit) {
	    break;
	}
    }

    t_end_interval (root, T_INPUT_PHASE);    
}

@end
