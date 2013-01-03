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
#include <limits.h>
#include <math.h>

#include "techne.h"
#include "controller.h"

@implementation Controller

-(void) initWithDevice: (const char *)name
{
    struct input_event ie;
    
    [super init];

    self->device = open(name, O_NONBLOCK | O_RDWR);
        
    self->force[0] = 0;
    self->force[1] = 0;

    memset (&self->effect, 0, sizeof(self->effect));
        
    ie.type = EV_FF;
    ie.code = FF_AUTOCENTER;
    ie.value = 0;
            
    if (write(self->device, &ie, sizeof(ie)) == -1)
        perror("set auto-center");
            
    ie.type = EV_FF;
    ie.code = FF_GAIN;
    ie.value = 0xFFFFUL;

    if (write(self->device, &ie, sizeof(ie)) == -1)
        perror("set gain");

    self->effect.type = FF_CONSTANT;
    self->effect.id = -1;

    ioctl(self->device, EVIOCSFF, &self->effect);

    ie.type = EV_FF;
    ie.code = effect.id;
    ie.value = 1;

    if (write(self->device, (const void*) &ie, sizeof(ie)) == -1) {
        perror("Play effect");
    }
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
        
                if (events[i].value == 1 || events[i].value == 2) {
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
    ioctl(self->device, EVIOCRMFF, self->effect.id);
    close(self->device);

    [super free];
}

-(int) _get_force
{
    int j;
    
    lua_newtable (_L);
        
    for(j = 0 ; j < 2 ; j += 1) {
	lua_pushnumber (_L, self->force[j]);

	lua_rawseti (_L, -2, j + 1);
    }

    return 1;
}

-(void) _set_force
{
    int j;
    
    if(lua_istable (_L, 3)) {
	for(j = 0 ; j < 2 ; j += 1) {
	    lua_rawgeti (_L, 3, j + 1);
	    self->force[j] = lua_tonumber (_L, -1);
	    lua_pop (_L, 1);
	}

        if (self->force[0] > 1) {
            self->force[0] = 1;
        } else if (self->force[0] < -1) {
            self->force[0] = -1;
        }
        
        if (self->force[1] >= 0) {
            self->force[1] = fmod(self->force[1], 2 * M_PI);
        } else {
            self->force[1] = fmod(self->force[1], 2 * M_PI) + 2 * M_PI;
        }
        
        self->effect.u.constant.level = self->force > 0 ? self->force[0] * SHRT_MAX : -self->force[0] * SHRT_MIN;
        self->effect.direction = self->force[1] / (2 * M_PI) * USHRT_MAX;

        ioctl(self->device, EVIOCSFF, &self->effect);
    }
}

@end
