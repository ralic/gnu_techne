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
#include <ctype.h>
#include <string.h>

#include "techne.h"
#include "input.h"
#include "pointer.h"

@implementation Pointer

-(void) init
{
    [super init];

    self->axes[0] = 0;
    self->axes[1] = 0;
}

-(void) input
{
    GdkEvent *event;

    /* Fire the bindings based on the event type. */
    
    for (event = [Input first] ; event ; event = [Input next]) {    
        assert (event);
    
        if (event->type == GDK_BUTTON_PRESS ||
            event->type == GDK_BUTTON_RELEASE) {
            t_pushuserdata (_L, 1, self);
            lua_pushnumber (_L, event->button.button);
        
            if (event->type == GDK_BUTTON_PRESS) {
                t_callhook (_L, self->buttonpress, 2, 0);
            } else {
                t_callhook (_L, self->buttonrelease, 2, 0);
            }
        } else if (event->type == GDK_SCROLL) {
            t_pushuserdata (_L, 1, self);

            if (event->scroll.direction == GDK_SCROLL_UP) {
                lua_pushinteger (_L, 0);
                lua_pushinteger (_L, 1);
            } else if (event->scroll.direction == GDK_SCROLL_DOWN) {
                lua_pushinteger (_L, 0);
                lua_pushinteger (_L, -1);
            } else if (event->scroll.direction == GDK_SCROLL_LEFT) {
                lua_pushinteger (_L, 1);
                lua_pushinteger (_L, -1);
            } else if (event->scroll.direction == GDK_SCROLL_RIGHT) {
                lua_pushinteger (_L, 1);
                lua_pushinteger (_L, 1);
            }
	
            t_callhook (_L, self->relative, 3, 0);
        } else if (event->type == GDK_MOTION_NOTIFY) {
            if (event->motion.x != self->axes[0]) {
                t_pushuserdata (_L, 1, self);
                lua_pushinteger (_L, 0);
                lua_pushinteger (_L, event->motion.x);

                t_callhook (_L, self->absolute, 3, 0);
            }
            
            if (event->motion.y != self->axes[1]) {
                t_pushuserdata (_L, 1, self);
                lua_pushinteger (_L, 1);
                lua_pushinteger (_L, event->motion.y);

                t_callhook (_L, self->absolute, 3, 0);
            }

            self->axes[0] = event->motion.x;
            self->axes[1] = event->motion.y;
        }
    }

    [super input];
}

@end
