/* Copyright (C) 2012 Papavasileiou Dimitris                             
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
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <assert.h>
#include <unistd.h>
#include <linux/input.h>

#include <lua.h>
#include <lauxlib.h>

#include "techne.h"
#include "pointer.h"
#include "keyboard.h"
#include "controller.h"

#define BITS_PER_WORD (sizeof(unsigned int) * 8)
#define MAX_EVENT_WORDS ((EV_MAX / BITS_PER_WORD) + 1)
#define test(b, i) (b[i / BITS_PER_WORD] & (1 << (i % BITS_PER_WORD)))

static int constructkeyboard(lua_State *L)
{
    [[Keyboard alloc] init];
    t_configurenode (L, 1);

    return 1;    
}

static int constructpointer(lua_State *L)
{
    [[Pointer alloc] init];
    t_configurenode (L, 1);

    return 1;    
}

static int constructcontroller(lua_State *L)
{
    [[Controller alloc] initWithDevice: lua_tostring(L, lua_upvalueindex (1))];
    t_configurenode (L, 1);

    return 1;    
}

int luaopen_controllers_core (lua_State *L)
{
    luaL_Reg api[] = {
	{NULL, NULL}
    };
    
    int fd, i, j, n, h;
    char name[256] = "Unknown device";
    unsigned int bits[MAX_EVENT_WORDS];

    lua_newtable(L);

    lua_pushstring (L, "Core pointer");
    lua_pushcfunction (L, constructpointer);
    lua_settable (L, -3);

    lua_pushstring (L, "Core keyboard");
    lua_pushcfunction (L, constructkeyboard);
    lua_settable (L, -3);
    
    h = lua_gettop(L);
        
    for (i = 0, j = 2 ; i < 32 ; i += 1) {
        lua_pushfstring(L, "/dev/input/event%d", i);
        fd = open(lua_tostring(L, -1), O_NONBLOCK | O_RDONLY);

        if (fd < 0) {
            goto next;
        }

        if(ioctl(fd, EVIOCGNAME(sizeof(name)), name) < 0) {
            t_print_warning("Could not get input device name");
            goto next;
        }

        lua_pushstring (L, name);
        lua_insert (L, -2);

        memset(bits, 0, sizeof(bits));
        n = ioctl(fd, EVIOCGBIT(0, sizeof(bits)), bits);
            
        if (n < 0) {
            t_print_warning("Could not get input device event set");
            goto next;
        }
            
        if(!(test(bits, EV_ABS) || test(bits, EV_REL) || test(bits, EV_KEY))) {
            goto next;
        }
        
        t_print_message ("Adding input device %s at %s.\n",
                         lua_tostring(L, -2),
                         lua_tostring(L, -1));
        
        lua_pushcclosure(L, constructcontroller, 1);
        lua_settable (L, -3);
        j += 1;
        
    next:
        lua_settop (L, h);

        if (fd > 0) {
            close(fd);
        }
    }

    t_print_message ("Added %d input devices.\n", j);

    luaL_setfuncs(L, api, 0);

    return 1;
}

