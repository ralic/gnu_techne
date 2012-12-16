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
#include "cursor.h"

@implementation Cursor

-(void) init
{
    [super init];

    self->buttonpress = LUA_REFNIL;
    self->buttonrelease = LUA_REFNIL;
    self->keypress = LUA_REFNIL;
    self->keyrelease = LUA_REFNIL;
    self->motion = LUA_REFNIL;
    self->scroll = LUA_REFNIL;
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
                lua_pushstring (_L, "up");
            } else if (event->scroll.direction == GDK_SCROLL_DOWN) {
                lua_pushstring (_L, "down");
            } else if (event->scroll.direction == GDK_SCROLL_LEFT) {
                lua_pushstring (_L, "left");
            } else if (event->scroll.direction == GDK_SCROLL_RIGHT) {
                lua_pushstring (_L, "right");
            }
	
            t_callhook (_L, self->scroll, 2, 0);
        } else if (event->type == GDK_MOTION_NOTIFY) {
            t_pushuserdata (_L, 1, self);
		
            lua_pushnumber (_L, event->motion.x);
            lua_pushnumber (_L, event->motion.y);

            t_callhook (_L, self->motion, 3, 0);
        } else if (event->type == GDK_KEY_PRESS ||
                   event->type == GDK_KEY_RELEASE) {
            char *name;
            unsigned int k;

            t_pushuserdata (_L, 1, self);

            k = event->key.keyval;
            name = gdk_keyval_name (k);

            if (k > 255 || !isalnum(k)) {
                char *new, *c;

                /* Make a temporary copy of the name and down-case it. */
	    
                new = strcpy(alloca(strlen(name) + 1), name);

                for (c = new ; *c ; c += 1) {
                    if (isupper(*c)) {
                        *c = tolower(*c);
                    }
                }
	    
                lua_pushstring (_L, new);
            } else {
                lua_pushstring (_L, name);
            }

            if (event->type == GDK_KEY_PRESS) {
                t_callhook (_L, self->keypress, 2, 0);
            } else {
                t_callhook (_L, self->keyrelease, 2, 0);
            }	
        }
    }

    [super input];
}

-(void) free
{
    luaL_unref (_L, LUA_REGISTRYINDEX, self->buttonpress);
    luaL_unref (_L, LUA_REGISTRYINDEX, self->buttonrelease);
    luaL_unref (_L, LUA_REGISTRYINDEX, self->keypress);
    luaL_unref (_L, LUA_REGISTRYINDEX, self->keyrelease);
    luaL_unref (_L, LUA_REGISTRYINDEX, motion);
    luaL_unref (_L, LUA_REGISTRYINDEX, scroll);

    [super free];
}

-(int) _get_buttonpress
{
    lua_rawgeti(_L, LUA_REGISTRYINDEX, self->buttonpress);

    return 1;
}

-(int) _get_buttonrelease
{
    lua_rawgeti(_L, LUA_REGISTRYINDEX, self->buttonrelease);

    return 1;
}

-(int) _get_keypress
{
    lua_rawgeti(_L, LUA_REGISTRYINDEX, self->keypress);

    return 1;
}

-(int) _get_keyrelease
{
    lua_rawgeti(_L, LUA_REGISTRYINDEX, self->keyrelease);

    return 1;
}

-(int) _get_motion
{
    lua_rawgeti(_L, LUA_REGISTRYINDEX, motion);

    return 1;
}

-(int) _get_scroll
{
    lua_rawgeti(_L, LUA_REGISTRYINDEX, scroll);

    return 1;
}

-(void) _set_buttonpress
{
    luaL_unref (_L, LUA_REGISTRYINDEX, self->buttonpress);
    self->buttonpress = luaL_ref (_L, LUA_REGISTRYINDEX);
}

-(void) _set_buttonrelease
{
    luaL_unref (_L, LUA_REGISTRYINDEX, self->buttonrelease);
    self->buttonrelease = luaL_ref (_L, LUA_REGISTRYINDEX);
}

-(void) _set_keypress
{
    luaL_unref (_L, LUA_REGISTRYINDEX, self->keypress);
    self->keypress = luaL_ref (_L, LUA_REGISTRYINDEX);
}

-(void) _set_keyrelease
{
    luaL_unref (_L, LUA_REGISTRYINDEX, self->keyrelease);
    self->keyrelease = luaL_ref (_L, LUA_REGISTRYINDEX);
}

-(void) _set_motion
{
    luaL_unref (_L, LUA_REGISTRYINDEX, motion);
    self->motion = luaL_ref (_L, LUA_REGISTRYINDEX);
}

-(void) _set_scroll
{
    luaL_unref (_L, LUA_REGISTRYINDEX, scroll);
    self->scroll = luaL_ref (_L, LUA_REGISTRYINDEX);
}

@end
