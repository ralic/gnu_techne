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
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/input.h>

#include "techne.h"
#include "controller.h"

@implementation Controller

-(void) initWithDevice: (const char *)name
{
    [super init];

    self->buttonpress = LUA_REFNIL;
    self->buttonrelease = LUA_REFNIL;
    self->motion = LUA_REFNIL;

    self->device = open(name, O_NONBLOCK | O_RDONLY);
}

-(void) input
{
    struct input_event events[64];
    int i, n;
    
    n = read(self->device, events, 64 * sizeof(struct input_event));

    if (n > 0) {
        assert (n % sizeof(struct input_event) == 0);

        /* Fire the bindings based on the event type. */
    
        for (i = 0 ; i < n / sizeof(struct input_event) ; i += 1) {
            switch(events[i].type) {
            case EV_KEY:
                t_pushuserdata (_L, 1, self);
                lua_pushnumber (_L, events[i].code);
        
                if (events[i].value == 1) {
                    t_callhook (_L, self->buttonpress, 2, 0);
                } else {
                    t_callhook (_L, self->buttonrelease, 2, 0);
                }

                break;                  
            case EV_REL:
            case EV_ABS:
                t_pushuserdata (_L, 1, self);
		
                lua_pushnumber (_L, events[i].code);
                lua_pushnumber (_L, events[i].value);

                t_callhook (_L, self->motion, 3, 0);

                break;
            }
        }
    }

    [super input];
}

-(void) free
{
    luaL_unref (_L, LUA_REGISTRYINDEX, self->buttonpress);
    luaL_unref (_L, LUA_REGISTRYINDEX, self->buttonrelease);
    luaL_unref (_L, LUA_REGISTRYINDEX, motion);

    close(self->device);

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

-(int) _get_motion
{
    lua_rawgeti(_L, LUA_REGISTRYINDEX, motion);

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

-(void) _set_motion
{
    luaL_unref (_L, LUA_REGISTRYINDEX, motion);
    self->motion = luaL_ref (_L, LUA_REGISTRYINDEX);
}

@end
